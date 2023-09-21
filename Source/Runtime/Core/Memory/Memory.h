#pragma once

#include "Core.h"
#include "Memory/Allocators/Allocator.h"

#define ArenaTempBlock(arena, name) \
    ArenaTemp name = { 0 };         \
    DeferLoop(name = ArenaTempBegin(arena), ArenaTempEnd(name))


namespace NekoEngine
{
    struct Arena
    {
        uint64_t Position;
        uint64_t CommitPosition;
        uint64_t Align;
        uint64_t Size;
        Arena* Ptr;
        uint64_t _unused_[3];
    };

    struct ArenaTemp
    {
        Arena* arena;
        uint64_t pos;
    };

    Arena* ArenaAlloc(uint64_t size);
    Arena* ArenaAllocDefault();
    void ArenaRelease(Arena* arena);
    void* ArenaPushNoZero(Arena* arena, uint64_t size);
    void* ArenaPushAligner(Arena* arena, uint64_t alignment);
    void* ArenaPush(Arena* arena, uint64_t size);
    void ArenaPopTo(Arena* arena, uint64_t pos);
    void ArenaSetAutoAlign(Arena* arena, uint64_t align);
    void ArenaPop(Arena* arena, uint64_t size);
    void ArenaClear(Arena* arena);
    uint64_t ArenaPos(Arena* arena);
    ArenaTemp ArenaTempBegin(Arena* arena);
    void ArenaTempEnd(ArenaTemp temp);


    class Memory
    {
    public:
        static void* AlignedAlloc(size_t size, size_t alignment);
        static void AlignedFree(void* data);

        static void* NewFunc(std::size_t size, const char* file, int line);
        static void DeleteFunc(void* p);
        static void LogMemoryInformation();

        static Allocator* const MemoryAllocator;
    };

} // NekoEngine

