#include "Allocators/BinAllocator.h"
#include "Allocators/DefaultAllocator.h"
#include "Allocators/StbAllocator.h"
#include "Memory.h"

namespace NekoEngine
{
    Allocator* const Memory::MemoryAllocator = new DefaultAllocator();

    void* Memory::AlignedAlloc(size_t size, size_t alignment)
    {
        void* data;
        data = _aligned_malloc(size, alignment);

        return data;
    }

    void Memory::AlignedFree(void* data)
    {
        _aligned_free(data);

    }

    void* Memory::NewFunc(std::size_t size, const char* file, int line)
    {
        return MemoryAllocator->Malloc(size, file, line);
    }

    void Memory::DeleteFunc(void* p)
    {
        return MemoryAllocator->Free(p);
    }

    void Memory::LogMemoryInformation()
    {
        return MemoryAllocator->Print();
    }

    // Arenas
    Arena* ArenaAlloc(uint64_t size)
    {
        auto* arena          = (Arena*)malloc(sizeof(Arena) + size);
        arena->Position       = sizeof(Arena);
        arena->CommitPosition = sizeof(Arena);
        arena->Align          = alignof(std::max_align_t);
        arena->Size           = size;
        arena->Ptr            = arena;
        return arena;
    }

    Arena* ArenaAllocDefault()
    {
        return ArenaAlloc(4096);
    }

    void ArenaRelease(Arena* arena)
    {
        if(arena)
        {
            free(arena);
        }
    }

    void* ArenaPushNoZero(Arena* arena, uint64_t size)
    {
        assert(arena != nullptr);
        uint64_t alignedSize = (size + arena->Align - 1) & ~(arena->Align - 1);
        uint64_t newPos      = arena->Position + alignedSize;

        if(newPos > arena->Size)
        {
            return nullptr; // Not enough space in the arena
        }

        void* ptr       = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(arena->Ptr) + arena->Position);
        arena->Position = newPos;
        return ptr;
    }

    void* ArenaPushAligner(Arena* arena, uint64_t alignment)
    {
        assert(arena != nullptr);
        assert((alignment & (alignment - 1)) == 0); // Ensure alignment is a power of 2

        uint64_t currentAddr      = reinterpret_cast<uintptr_t>(arena->Ptr) + arena->Position;
        uint64_t alignedAddr      = (currentAddr + alignment - 1) & ~(alignment - 1);
        uint64_t alignmentPadding = alignedAddr - currentAddr;
        uint64_t newPos           = arena->Position + alignmentPadding;

        if(newPos > arena->Size)
        {
            return nullptr; // Not enough space in the arena
        }

        void* ptr       = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(arena->Ptr) + newPos);
        arena->Position = newPos;
        return ptr;
    }

    void* ArenaPush(Arena* arena, uint64_t size)
    {
        void* ptr = ArenaPushNoZero(arena, size);
        if(ptr)
        {
            memset(ptr, 0, size); // Zero initialize the memory
        }
        return ptr;
    }

    void ArenaPopTo(Arena* arena, uint64_t pos)
    {
        assert(arena != nullptr);
        assert(pos <= arena->Position);
        arena->Position = pos;
    }

    void ArenaSetAutoAlign(Arena* arena, uint64_t align)
    {
        assert(arena != nullptr);
        arena->Align = align;
    }

    void ArenaPop(Arena* arena, uint64_t size)
    {
        assert(arena != nullptr);
        assert(size <= arena->Position);
        arena->Position -= size;
    }

    void ArenaClear(Arena* arena)
    {
        assert(arena != nullptr);
        arena->Position = sizeof(Arena);
    }

    uint64_t ArenaPos(Arena* arena)
    {
        assert(arena != nullptr);
        return arena->Position;
    }

    ArenaTemp ArenaTempBegin(Arena* arena)
    {
        assert(arena != nullptr);
        return { arena, arena->Position };
    }

    void ArenaTempEnd(ArenaTemp temp)
    {
        assert(temp.arena != nullptr);
        ArenaPopTo(temp.arena, temp.pos);
    }
} // NekoEngine