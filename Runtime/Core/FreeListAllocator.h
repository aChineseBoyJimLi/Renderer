#pragma once

#include <list>

class FreeListAllocator
{
public:
    FreeListAllocator() = default;
    FreeListAllocator(uint32_t inCount) { SetTotalCount(inCount); }
    void SetTotalCount(uint32_t inCount);
    uint32_t GetTotalCount() const { return m_TotalCount; }
    bool TryAllocate(uint32_t inCount, uint32_t& outOffset);
    void Free(uint32_t inOffset, uint32_t inCount);
    void Reset();
    bool IsEmpty() const;

private:
    struct AllocatorRange
    {
        uint32_t First;
        uint32_t Last;

        AllocatorRange(uint32_t inFirst, uint32_t inLast)
            : First(inFirst), Last(inLast) {}
    };

    std::list<AllocatorRange> m_FreeList;
    uint32_t m_TotalCount = 0;
};