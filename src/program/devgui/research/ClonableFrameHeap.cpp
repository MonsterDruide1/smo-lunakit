#include "ClonableFrameHeap.h"
#include <cstdlib>
#include <utility>
#include "basis/seadRawPrint.h"
#include "basis/seadTypes.h"
#include "container/seadBuffer.h"
#include "heap/seadHeapMgr.h"
#include "heap/seadMemBlock.h"
#include "math/seadMathCalcCommon.h"
#include "logger/Logger.hpp"

namespace sead {

const int cMinAlignment = alignof(void*);

ClonableFrameHeap* ClonableFrameHeap::tryCreate(size_t size_, const SafeString& name, Heap* parent, s32 alignment, HeapDirection direction, bool enableLock)
{
    // FIXME alignment ignored!
    
    if (!parent)
    {
        parent = HeapMgr::instance()->getCurrentHeap();
        if (!parent)
        {
            SEAD_ASSERT_MSG(false, "current heap is null");
            return nullptr;
        }
    }

    size_t size;
    if (size_ == 0)
    {
        size = parent->getMaxAllocatableSize(alignment);
        SEAD_ASSERT_MSG(MathSizeT::isMultiplePow2(size, cMinAlignment),
                        "[bug] getMaxAllocatableSize must return multiple of cMinAlignment");
        size = MathSizeT::roundDownPow2(size, cMinAlignment);
    }
    else
    {
        size = MathSizeT::roundUpPow2(size_, cMinAlignment);
    }

    if (size < sizeof(ClonableFrameHeap))
    {
        SEAD_ASSERT_MSG(size_ == 0, "size must be able to include manage area: size=%zu", size);
        return nullptr;
    }

    void* heapStart;

    {
#ifdef SEAD_DEBUG
        ScopedDebugFillSystemDisabler disabler(parent);
#endif // SEAD_DEBUG
        heapStart = parent->tryAlloc(size, static_cast<s32>(direction) * cMinAlignment);
    }

    if (!heapStart)
        return nullptr;

    if (parent->mDirection == HeapDirection::cHeapDirection_Reverse)
        direction = static_cast<HeapDirection>(-static_cast<s32>(direction));

    ClonableFrameHeap* result;
    if (direction == HeapDirection::cHeapDirection_Forward)
    {
        result = new(heapStart) ClonableFrameHeap(name, parent, heapStart, size, direction, enableLock);
    }
    else
    {
        void* ptr = PtrUtil::addOffset(heapStart, size - sizeof(ClonableFrameHeap));
        result = new(ptr) ClonableFrameHeap(name, parent, heapStart, size, direction, enableLock);
    }

    result->initialize_();

    parent->pushBackChild_(result);

#ifdef SEAD_DEBUG
    {
        HeapMgr* mgr = HeapMgr::instance();
        if (mgr)
            mgr->callCreateCallback_(result);
    }
#endif // SEAD_DEBUG

    return result;
}

struct MemoryArea {
    u64 start;
    u64 end;
};

void* ClonableFrameHeap::tryAlloc(size_t size, s32 alignment) {
    //Logger::log("Trying to allocate %zu bytes with alignment %d on %p\n", size, alignment, this);
    //Logger::log("Remaining size: %zu / %zu\n", getFreeSize(), mSize);
    void* ptr = FrameHeap::tryAlloc(size, alignment);
    //Logger::log("After-Remaining size: %zu / %zu\n", getFreeSize(), mSize);
    //MemBlock* block = MemBlock::FindManageArea(ptr);
    //memset(ptr, 0, block->mSize);
    return ptr;
}

int comparePairSortFrame(const std::pair<MemoryArea, u64>* a, const std::pair<MemoryArea, u64>* b) {
    return a->first.start - b->first.start;
}

int comparePairFindFrame(const std::pair<MemoryArea, u64>* checkArea, const std::pair<MemoryArea, u64>* comparisonPointer) {
    if(checkArea->first.start > comparisonPointer->first.start)
        return 1;
    if(checkArea->first.end < comparisonPointer->first.start)
        return -1;
    return 0;  // fulfills start <= pointer < end
}

ClonableFrameHeap* ClonableFrameHeap::clone() const {
    /*u64 origRangeStart = (u64)mStart;
    u64 origRangeEnd = (u64)PtrUtil::addOffset(mStart, mSize);

    ClonableFrameHeap* copy = create(mSize, getName(), mParent, 16, mDirection, false /* isEnableLock *-/);
    u64 copyRangeStart = (u64)copy->mStart;

    // use malloc to avoid placing on heap itself
    s32 numAllocs = mUseList.size();
    std::pair<MemoryArea, u64>* allocMappingsBuffer = (std::pair<MemoryArea, u64>*)malloc(numAllocs * sizeof(std::pair<MemoryArea, u64>));
    sead::Buffer<std::pair<MemoryArea, u64>> allocationMappings{numAllocs, allocMappingsBuffer};

    MemBlock* current = mUseList.front();
    int i=0;
    do {
        u64 startOfCopyBlock = (u64)copy->tryAlloc(current->mSize, 8);

        allocationMappings[i++] = std::make_pair(MemoryArea{(u64)current->memory(), (u64)current->memory()+current->mSize}, startOfCopyBlock);
    } while((current = mUseList.next(current)) != nullptr);

    allocationMappings.heapSort(0, numAllocs-1, &comparePairSortFrame);

    for(int i=0; i<allocationMappings.size(); i++) {
        std::pair<MemoryArea, u64> pair = allocationMappings[i];
        MemoryArea& block = pair.first;
        u64 startOfCopyBlock = pair.second;
        u64 startOfOrigBlock = block.start;

        for(u64 i=0; i+7<block.end-block.start; i+=8) {
            u64 value = *((u64*)(startOfOrigBlock+i));
            if(value >= origRangeStart && value < origRangeEnd) {
                MemoryArea searchBlock = {value, (u64)-1};
                s32 mappingIndex = allocationMappings.binarySearch(std::make_pair(searchBlock, -1), &comparePairFindFrame);
                if(mappingIndex == -1)
                    SEAD_ASSERT_MSG(false, "could not find mapping for %p, required at %p (base=%p)\n", (void*)value, (void*)(startOfOrigBlock+i), (void*)startOfOrigBlock);
                u64 offset = value - allocationMappings[mappingIndex].first.start;
                value = allocMappingsBuffer[mappingIndex].second + offset;
            }
            *((u64*)(startOfCopyBlock+i)) = value;
        }
        // copy remaining bytes
        for(u64 i=(block.end-block.start)/8*8; i<block.end-block.start; i++) {
            *((u8*)(startOfCopyBlock+i)) = *((u8*)(startOfOrigBlock+i));
        }
    }

    ::free(allocMappingsBuffer);
    return copy;*/
}

}
