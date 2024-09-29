#include "tas.h"
#include "nn/mem.h"
#include "nn/init.h"

#define alloc(SIZE) nn::init::GetAllocator()->Allocate(SIZE)
#define dealloc(PTR) nn::init::GetAllocator()->Free(PTR)
#define realloc(PTR, SIZE) nn::init::GetAllocator()->Reallocate(PTR, SIZE);

void fl::TasHolder::update()
{
    if (startPending)
    {
        isRunning = true;
        curFrame = 0;
        startPending = false;
        return;
    }
    if(!isRunning) return;
    
    if (curFrame + 1 >= frameCount) stop();
    curFrame++;
}

void fl::TasHolder::start()
{
    isRunning = true;
}

void fl::TasHolder::stop()
{
    curFrame = 0;
    isRunning = false;
}

void fl::TasHolder::setScriptName(char* name)
{
    if (scriptName) dealloc(scriptName);
    scriptName = name;
}
