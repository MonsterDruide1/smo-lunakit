#pragma once

#include "imgui.h"

#include "al/scene/Scene.h"

#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/System/GameSystem.h"

#include "container/seadPtrArray.h"
#include "heap/seadDisposer.h"
#include "heap/seadHeap.h"

#include "program/SequenceUtil.h"

#include "program/devgui/windows/WindowBase.h"
#include "program/devgui/windows/WindowEditor.h"
#include "program/devgui/windows/WindowInfo.h"
#include "program/devgui/windows/WindowFPS.h"

#include "program/devgui/homemenu/HomeMenuBase.h"
#include "program/devgui/homemenu/HomeMenuDebugger.h"

class DevGuiManager {
    SEAD_SINGLETON_DISPOSER(DevGuiManager);
    DevGuiManager();
    ~DevGuiManager();

public:
    void init(sead::Heap* heap);

    void update();
    void updateDisplay();

    bool isMenuActive() { return mIsActive; };
    bool isFirstStep() { return mIsFirstStep; };

    sead::Heap* getHeap() { return mDevGuiHeap; }
    int getWindowCount() { return mWindows.size(); }

private:
    sead::Heap* mDevGuiHeap = nullptr; // Uses the game system heap

    bool mIsActive = false;
    bool mIsFirstStep = false;

    sead::PtrArray<WindowBase> mWindows;
    sead::PtrArray<HomeMenuBase> mHomeMenuTabs;
};