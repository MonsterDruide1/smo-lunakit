#include "program/devgui/DevGuiManager.h"

SEAD_SINGLETON_DISPOSER_IMPL(DevGuiManager)
DevGuiManager::DevGuiManager() = default;
DevGuiManager::~DevGuiManager() = default;

void DevGuiManager::init(sead::Heap* heap)
{
    mWindows.allocBuffer(0x10, heap);
    mHomeMenuTabs.allocBuffer(0x10, heap);
    mDevGuiHeap = heap;
    mIsActive = false;
    
    // Create all display windows
    WindowEditor* editorWindow = new (heap) WindowEditor("LunaKit Param Editor", heap);
    mWindows.pushBack(editorWindow);

    WindowInfo* infoWindow = new (heap) WindowInfo("LunaKit Info Viewer", heap);
    mWindows.pushBack(infoWindow);

    WindowFPS* fpsWindow = new (heap) WindowFPS("FPS Window", heap);
    mWindows.pushBack(fpsWindow);

    // Create all home menu tabs
    HomeMenuDebugger* homeDebug = new (heap) HomeMenuDebugger("Debug", heap);
    mHomeMenuTabs.pushBack(homeDebug);
}

void DevGuiManager::update()
{
    // Check for enabling and disabling the window
    if (al::isPadHoldR(-1) && al::isPadHoldZR(-1) && al::isPadTriggerL(-1)) {
        mIsActive = !mIsActive;
        if (mIsActive)
            mIsFirstStep = true;
    }

    // Note: Each window's update function runs even with the menu closed/inactive!
    for (int i = 0; i < mWindows.size(); i++) {
        auto* entry = mWindows.at(i);
        entry->updateWin();
    }
}

void DevGuiManager::updateDisplay()
{
    // Setup mouse cursor state
    if (!mIsActive) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        return;
    }

    if (mIsFirstStep)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

    // Load and draw all windows
    for (int i = 0; i < mWindows.size(); i++) {
        auto* entry = mWindows.at(i);
        entry->updateWinDisplay();
    }
    
    // Load and draw all home menu tabs
    if (ImGui::BeginMainMenuBar()) {
        for (int i = 0; i < mHomeMenuTabs.size(); i++) {
            auto* entry = mHomeMenuTabs.at(i);
            if (ImGui::BeginMenu(entry->getMenuName())) {
                entry->updateMenu();

                ImGui::EndMenu();
            }
        }

        ImGui::EndMainMenuBar();
    }

    // Draw the demo window if the settings class has it enabled
    if(DevGuiSettings::instance()->mIsDisplayImGuiDemo)
        ImGui::ShowDemoWindow();

    // Reset the first step flag when complete!
    if (mIsFirstStep)
        mIsFirstStep = false;
}