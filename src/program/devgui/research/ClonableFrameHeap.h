#pragma once

#include "heap/seadFrameHeap.h"

namespace sead {

class ClonableFrameHeap : public FrameHeap {
public:
    static ClonableFrameHeap* tryCreate(size_t size_, const SafeString& name, Heap* parent, s32 alignment, HeapDirection direction, bool enableLock);

    static ClonableFrameHeap* refCreate(size_t size, const SafeString& name, Heap* parent, s32 alignment,
                           HeapDirection direction, bool flag) {
        return tryCreate(size, name, parent, alignment, direction, true);//flag);
    }
    
    ClonableFrameHeap(const SafeString& name, Heap* parent, void* address, size_t size,
            HeapDirection direction, bool flag) : FrameHeap(name, parent, address, size, direction, flag) {}

    void* tryAlloc(size_t size, s32 alignment) override;

    ClonableFrameHeap* clone() const;
};

}
