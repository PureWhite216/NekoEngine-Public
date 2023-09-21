#include "JobSystem.h"
#define _AMD64_
#include "processthreadsapi.h"
#include <atomic>
#include <thread>
#include <condition_variable>
#include <deque>

#define NOMINMAX
#include <Windows.h>
#include <comdef.h>
#include <mutex>
#include <sstream>

#include "Math/Maths.h"

namespace NekoEngine
{
    class SpinLock
    {
        std::atomic_flag locked = ATOMIC_FLAG_INIT;

    public:
        inline void lock()
        {
            int spin = 0;
            while(!TryLock())
            {
            }
        }

        inline bool TryLock()
        {
            return !locked.test_and_set(std::memory_order_acquire);
        }

        inline void unlock()
        {
            locked.clear(std::memory_order_release);
        }
    };
    namespace JobSystem
    {
        struct Job
        {
            Context* ctx;
            std::function<void(JobDispatchArgs)> task;
            uint32_t groupID;
            uint32_t groupJobOffset;
            uint32_t groupJobEnd;
            uint32_t sharedmemory_size;
        };

        struct JobQueue
        {
            std::deque<Job> queue;
            std::mutex locker;

            inline void push_back(const Job& item)
            {
                std::scoped_lock lock(locker);
                queue.push_back(item);
            }

            inline bool pop_front(Job& item)
            {
                std::scoped_lock lock(locker);
                if(queue.empty())
                {
                    return false;
                }
                item = std::move(queue.front());
                queue.pop_front();
                return true;
            }
        };

        struct InternalState
        {
            uint32_t numCores   = 0;
            uint32_t numThreads = 0;
            std::unique_ptr<JobQueue[]> jobQueuePerThread;
            std::atomic_bool alive { true };
            std::condition_variable wakeCondition;
            std::mutex wakeMutex;
            std::atomic<uint32_t> nextQueue { 0 };
            ArrayList<std::thread> threads;

            ~InternalState()
            {
                alive.store(false); // indicate that new jobs cannot be started from this point
                bool wake_loop = true;
                std::thread waker([&]
                                  {
                                      while (wake_loop)
                                      {
                                          wakeCondition.notify_all(); // wakes up sleeping worker threads
                                      } });
                for(auto& thread : threads)
                {
                    if(thread.joinable())
                        thread.join();
                }
                wake_loop = false;
                if(waker.joinable())
                    waker.join();
            }
        };
        static InternalState* internal_state = nullptr;

        inline void work(uint32_t startingQueue)
        {
            Job job;
            for(uint32_t i = 0; i < internal_state->numThreads; ++i)
            {
                JobQueue& job_queue = internal_state->jobQueuePerThread[startingQueue % internal_state->numThreads];
                while(job_queue.pop_front(job))
                {
                    JobDispatchArgs args;
                    args.groupID = job.groupID;
                    if(job.sharedmemory_size > 0)
                    {
                        thread_local static ArrayList<uint8_t> shared_allocation_data;
                        shared_allocation_data.reserve(job.sharedmemory_size);
                        args.sharedmemory = shared_allocation_data.data();
                    }
                    else
                    {
                        args.sharedmemory = nullptr;
                    }

                    for(uint32_t j = job.groupJobOffset; j < job.groupJobEnd; ++j)
                    {
                        args.jobIndex          = j;
                        args.groupIndex        = j - job.groupJobOffset;
                        args.isFirstJobInGroup = (j == job.groupJobOffset);
                        args.isLastJobInGroup  = (j == job.groupJobEnd - 1);
                        job.task(args);
                    }

                    job.ctx->counter.fetch_sub(1);
                }
                startingQueue++; // go to next queue
            }
        }

        void OnInit(uint32_t maxThreadCount)
        {
            if(!internal_state)
                internal_state = new InternalState();

            if(internal_state->numThreads > 0)
                return;

            maxThreadCount = Maths::Max(1u, maxThreadCount);

            // Retrieve the number of hardware threads in this System:
            internal_state->numCores = std::thread::hardware_concurrency();

            // Calculate the actual number of worker threads we want:
            // Reserve a couple of threads
            internal_state->numThreads = Maths::Min(maxThreadCount, Maths::Max(1u, internal_state->numCores - 1));

            // Keep one for update thread
            internal_state->numThreads -= 1;
            internal_state->jobQueuePerThread.reset(new JobQueue[internal_state->numThreads]);
            internal_state->threads.reserve(internal_state->numThreads);

            for(uint32_t threadID = 0; threadID < internal_state->numThreads; ++threadID)
            {
                std::thread& worker = internal_state->threads.emplace_back([threadID]
                                                                          {
                                                                              while (internal_state->alive.load())
                                                                              {
                                                                                  work(threadID);

                                                                                  std::stringstream ss;
                                                                                  ss << "JobSystem_" << threadID;

                                                                                  // finished with jobs, put to sleep
                                                                                  std::unique_lock<std::mutex> lock(internal_state->wakeMutex);
                                                                                  internal_state->wakeCondition.wait(lock);
                                                                              } });


                // Do Windows-specific thread setup:
                    HANDLE handle = (HANDLE)worker.native_handle();

                    // Put each thread on to dedicated core
                    DWORD_PTR affinityMask    = 1ull << threadID;
                    DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
                    ASSERT(affinity_result <= 0, "Failed to set thread affinity");

                    // Increase thread priority:
                    // BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
                    // NekoEngine_ASSERT(priority_result != 0, "");

                    // Name the thread:
                    std::wstring wthreadname = L"JobSystem_" + std::to_wstring(threadID);
                    HRESULT hr               = SetThreadDescription(handle, wthreadname.c_str());

                    ASSERT(!SUCCEEDED(hr), "Failed to set thread name");



                worker.detach();
            }

            LOG_FORMAT("Initialised JobSystem with [%d cores] [%d threads]", internal_state->numCores, internal_state->numThreads);
        }

        void Release()
        {
            delete internal_state;
            internal_state = nullptr;
        }

        uint32_t GetThreadCount()
        {
            return internal_state->numThreads;
        }

        void Execute(Context& ctx, const std::function<void(JobDispatchArgs)>& task)
        {
            // Context state is updated:
            ctx.counter.fetch_add(1);

            Job job;
            job.ctx               = &ctx;
            job.task              = task;
            job.groupID           = 0;
            job.groupJobOffset    = 0;
            job.groupJobEnd       = 1;
            job.sharedmemory_size = 0;

            internal_state->jobQueuePerThread[internal_state->nextQueue.fetch_add(1) % internal_state->numThreads].push_back(job);
            internal_state->wakeCondition.notify_one();
        }

        void Dispatch(Context& ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& task, size_t sharedmemory_size)
        {
            if(jobCount == 0 || groupSize == 0)
            {
                return;
            }

            const uint32_t groupCount = DispatchGroupCount(jobCount, groupSize);

            // Context state is updated:
            ctx.counter.fetch_add(groupCount);

            Job job;
            job.ctx               = &ctx;
            job.task              = task;
            job.sharedmemory_size = (uint32_t)sharedmemory_size;

            for(uint32_t groupID = 0; groupID < groupCount; ++groupID)
            {
                // For each group, generate one real job:
                job.groupID        = groupID;
                job.groupJobOffset = groupID * groupSize;
                job.groupJobEnd    = Maths::Min(job.groupJobOffset + groupSize, jobCount);

                internal_state->jobQueuePerThread[internal_state->nextQueue.fetch_add(1) % internal_state->numThreads].push_back(job);
            }

            internal_state->wakeCondition.notify_one();
        }

        uint32_t DispatchGroupCount(uint32_t jobCount, uint32_t groupSize)
        {
            // Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
            return (jobCount + groupSize - 1) / groupSize;
        }

        bool IsBusy(const Context& ctx)
        {
            // Whenever the main thread label is not reached by the workers, it indicates that some worker is still alive
            return ctx.counter.load() > 0;
        }

        void Wait(const Context& ctx)
        {
            if(IsBusy(ctx))
            {
                // Wake any threads that might be sleeping:
                internal_state->wakeCondition.notify_all();

                // work() will pick up any jobs that are on stand by and execute them on this thread:
                work(internal_state->nextQueue.fetch_add(1) % internal_state->numThreads);

                while(IsBusy(ctx))
                {
                    // If we are here, then there are still remaining jobs that work() couldn't pick up.
                    //    In this case those jobs are not standing by on a queue but currently executing
                    //    on other threads, so they cannot be picked up by this thread.
                    //    Allow to swap out this thread by OS to not spin endlessly for nothing
                    std::this_thread::yield();
                }
            }
        }


    }
} // NekoEngine