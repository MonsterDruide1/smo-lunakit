#pragma once

#include "heap/seadExpHeap.h"

namespace sead {

class ClonableExpHeap : public ExpHeap {
public:
    static ClonableExpHeap* create(size_t size, const SafeString& name, Heap* parent, s32 alignment,
                           HeapDirection direction, bool);
    static ClonableExpHeap* create(void* address, size_t size, const SafeString& name, bool);
    static ClonableExpHeap* create(void* address, size_t size, const SafeString& name, Heap* parent, bool);

    static ClonableExpHeap* refCreate(size_t size, const SafeString& name, Heap* parent, s32 alignment,
                           HeapDirection direction, bool flag) {
        return create(size, name, parent, alignment, direction, true);//flag);
    }

    static ClonableExpHeap* tryCreate(size_t size, const SafeString& name, Heap* parent, s32 alignment,
                              HeapDirection direction, bool);
    static ClonableExpHeap* tryCreate(void* address, size_t size, const SafeString& name, bool);
    static ClonableExpHeap* tryCreate(void* address, size_t size, const SafeString& name, Heap* parent, bool);
    
    ClonableExpHeap(const SafeString& name, Heap* parent, void* address, size_t size,
            HeapDirection direction, bool flag) : ExpHeap(name, parent, address, size, direction, flag) {}

    void* tryAlloc(size_t size, s32 alignment) override;
    void free(void* ptr) override;

    // emulate FrameHeap
    bool isFreeable() const override { return false; }

    ClonableExpHeap* clone() const;
};

}
