#include "FreeListAllocator.h"

void FreeListAllocator::SetTotalCount(uint32_t inCount)
{
    m_TotalCount = inCount;
    Reset();
}

bool FreeListAllocator::TryAllocate(uint32_t inCount, uint32_t& outOffset)
{
    if(inCount == 0)
    {
        outOffset = UINT_MAX;
        return false;
    }
    
    const auto numRanges = m_FreeList.size();
    if ( numRanges > 0)
    {
        for(auto it = m_FreeList.begin(); it != m_FreeList.end(); ++it)
        {
            AllocatorRange& currentRange = *it;
            const uint32_t Size = 1 + currentRange.Last - currentRange.First;
            if (inCount <= Size)
            {
                uint32_t first = currentRange.First;
                if (inCount == Size && std::next(it) != m_FreeList.end())
                {
                    // Range is full and a new range exists, remove it.
                    m_FreeList.erase(it);
                }
                else
                {
                    // Range is larger than required, split it.
                    currentRange.First += inCount;
                }
                outOffset = first;
                return true;
            }
        }
    }
    outOffset = UINT_MAX;
    return false;
}

void FreeListAllocator::Free(uint32_t inOffset, uint32_t inCount)
{
    if (inOffset == UINT_MAX || inCount == 0)
    {
        return;
    }
    const uint32_t End = inOffset + inCount;
    auto it = m_FreeList.begin();
    while (it != m_FreeList.end() && it->First < End)
    {
        ++it;
    }
    
    if (it != m_FreeList.begin() && std::prev(it)->Last + 1 == inOffset)
    {
        // Merge with previous range.
        --it;
        it->Last += inCount;
    }
    else
    {
        // Insert a new range before the current one.
        m_FreeList.insert(it, AllocatorRange(inOffset, End - 1));
    }
}

void FreeListAllocator::Reset()
{
    m_FreeList.clear();
    m_FreeList.emplace_back(0, m_TotalCount - 1);
}

bool FreeListAllocator::IsEmpty() const
{
    if(m_FreeList.size() == 1)
    {
        auto iter = m_FreeList.begin();
        const size_t size = 1 + iter->Last - iter->First;
        if(size == m_TotalCount)
        {
            return true;
        }
    }
    return false;
}