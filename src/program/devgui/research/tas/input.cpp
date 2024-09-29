#include "math/seadVector.h"
#include "input.h"
#include "tas.h"
#include "nn/hid.h"
#include "lib.hpp"
#include "logger/Logger.hpp"


#include <heap/seadHeap.h>
#include <heap/seadHeapMgr.h>
// 0 = FrameHeap, 1 = UnitHeap, 2 = SeparateHeap, 3 = ExpHeap
s32 getHeapType(sead::Heap* heap) {
    if(!heap->isFreeable())
        return 0;
    if(!heap->isResizable())
        return 1;
    if(!heap->isAdjustable())
        return 2;
    return 3;
}

void getTypes(sead::Heap* heap, s32* arr) {
    arr[getHeapType(heap)]++;
    for (sead::Heap& childRef : heap->mChildren) {
        sead::Heap* child = &childRef;
        if (child)
            getTypes(child, arr);
    }
}

static int frameCount = 0;
void logInputs(nn::hid::NpadBaseState* state) {
    nn::hid::NpadButtonSet buttons = state->mButtons;
    Logger::log("Buttons: ");
    if (buttons.Test((s32)nn::hid::NpadButton::A)) Logger::log("KEY_A;");
    if (buttons.Test((s32)nn::hid::NpadButton::B)) Logger::log("KEY_B;");
    if (buttons.Test((s32)nn::hid::NpadButton::X)) Logger::log("KEY_X;");
    if (buttons.Test((s32)nn::hid::NpadButton::Y)) Logger::log("KEY_Y;");
    if (buttons.Test((s32)nn::hid::NpadButton::L)) Logger::log("KEY_L;");
    if (buttons.Test((s32)nn::hid::NpadButton::R)) Logger::log("KEY_R;");
    if (buttons.Test((s32)nn::hid::NpadButton::ZL)) Logger::log("KEY_ZL;");
    if (buttons.Test((s32)nn::hid::NpadButton::ZR)) Logger::log("KEY_ZR;");
    if (buttons.Test((s32)nn::hid::NpadButton::Plus)) Logger::log("KEY_PLUS;");
    if (buttons.Test((s32)nn::hid::NpadButton::Minus)) Logger::log("KEY_MINUS;");
    if (buttons.Test((s32)nn::hid::NpadButton::Left)) Logger::log("KEY_LEFT;");
    if (buttons.Test((s32)nn::hid::NpadButton::Up)) Logger::log("KEY_UP;");
    if (buttons.Test((s32)nn::hid::NpadButton::Right)) Logger::log("KEY_RIGHT;");
    if (buttons.Test((s32)nn::hid::NpadButton::Down)) Logger::log("KEY_DOWN;");
    if (buttons.Test((s32)nn::hid::NpadButton::StickL)) Logger::log("KEY_LEFT_STICK;");
    if (buttons.Test((s32)nn::hid::NpadButton::StickR)) Logger::log("KEY_RIGHT_STICK;");

    Logger::log(" - AnalogStickL: %d, %d", state->mAnalogStickL.X, state->mAnalogStickL.Y);
    Logger::log(" - AnalogStickR: %d, %d", state->mAnalogStickR.X, state->mAnalogStickR.Y);
    Logger::log("\n");

    s32 types[4] = {0, 0, 0, 0};
    getTypes(sead::HeapMgr::instance()->getRootHeap(0), types);
    Logger::log("Heap types: FrameHeap: %d, UnitHeap: %d, SeparateHeap: %d, ExpHeap: %d\n", types[0], types[1], types[2], types[3]);
}

long raw_input;
long raw_input_prev;

bool isPressed(int button) {
    return (raw_input & (1 << button)) != 0;
}
bool isPressedPrev(int button) {
    return (raw_input_prev & (1 << button)) != 0;
}
bool isTriggerLeft() {
    return isPressed(12) && !isPressedPrev(12);
}
bool isTriggerUp() {
    return isPressed(13) && !isPressedPrev(13);
}
bool isTriggerRight() {
    return isPressed(14) && !isPressedPrev(14);
}
bool isTriggerDown() {
    return isPressed(15) && !isPressedPrev(15);
}
bool isL() {
    return isPressed(6);
}
bool isTriggerPressLeftStick() {
    return isPressed(4) && !isPressedPrev(4);
}

void controllerHook(nn::hid::NpadBaseState* state) {
    fl::TasHolder& h = fl::TasHolder::instance();
    logInputs(state);
    raw_input_prev = raw_input;
    raw_input = state->mButtonsRaw;
    if (!h.isRunning) {
        return;
    }

    state->mButtonsRaw = 0;
    if(h.frames[h.curFrame].A) 
        state->mButtonsRaw |= (1 << 0);
    if(h.frames[h.curFrame].B) 
        state->mButtonsRaw |= (1 << 1);
    if(h.frames[h.curFrame].X) 
        state->mButtonsRaw |= (1 << 2);
    if(h.frames[h.curFrame].Y) 
        state->mButtonsRaw |= (1 << 3);
    if(h.frames[h.curFrame].pressLeftStick) 
        state->mButtonsRaw |= (1 << 4);
    if(h.frames[h.curFrame].pressRightStick) 
        state->mButtonsRaw |= (1 << 5);
    if(h.frames[h.curFrame].L) 
        state->mButtonsRaw |= (1 << 6);
    if(h.frames[h.curFrame].R) 
        state->mButtonsRaw |= (1 << 7);
    if(h.frames[h.curFrame].ZL) 
        state->mButtonsRaw |= (1 << 8);
    if(h.frames[h.curFrame].ZR) 
        state->mButtonsRaw |= (1 << 9);
    if(h.frames[h.curFrame].plus) 
        state->mButtonsRaw |= (1 << 10);
    if(h.frames[h.curFrame].minus) 
        state->mButtonsRaw |= (1 << 11);
    if(h.frames[h.curFrame].dLeft) 
        state->mButtonsRaw |= (1 << 12);
    if(h.frames[h.curFrame].dUp) 
        state->mButtonsRaw |= (1 << 13);
    if(h.frames[h.curFrame].dRight) 
        state->mButtonsRaw |= (1 << 14);
    if(h.frames[h.curFrame].dDown) 
        state->mButtonsRaw |= (1 << 15);

    if(h.frames[h.curFrame].leftStick.x < -0.5f)
        state->mButtonsRaw |= (1 << 16);
    if(h.frames[h.curFrame].leftStick.y > 0.5f)
        state->mButtonsRaw |= (1 << 17);
    if(h.frames[h.curFrame].leftStick.x > 0.5f)
        state->mButtonsRaw |= (1 << 18);
    if(h.frames[h.curFrame].leftStick.y < -0.5f)
        state->mButtonsRaw |= (1 << 19);
        
    if(h.frames[h.curFrame].rightStick.x < -0.5f)
        state->mButtonsRaw |= (1 << 20);
    if(h.frames[h.curFrame].rightStick.y > 0.5f)
        state->mButtonsRaw |= (1 << 21);
    if(h.frames[h.curFrame].rightStick.x > 0.5f)
        state->mButtonsRaw |= (1 << 22);
    if(h.frames[h.curFrame].rightStick.y < -0.5f)
        state->mButtonsRaw |= (1 << 23);
    
    // all others unused

    //state->mAnalogStickL = (((long) h.frames[h.curFrame].leftStick.x) & 0xffffffff) | (((long) h.frames[h.curFrame].leftStick.y) << 32);
    //state->mAnalogStickR = (((long) h.frames[h.curFrame].rightStick.x) & 0xffffffff) | (((long) h.frames[h.curFrame].rightStick.y) << 32);
    state->mAnalogStickLRaw = (((long) (h.frames[h.curFrame].leftStick.x * 32767)) & 0xffffffff) | (((long) (h.frames[h.curFrame].leftStick.y * 32767)) << 32);
    state->mAnalogStickRRaw = (((long) (h.frames[h.curFrame].rightStick.x * 32767)) & 0xffffffff) | (((long) (h.frames[h.curFrame].rightStick.y * 32767)) << 32);
}


HOOK_DEFINE_TRAMPOLINE(NpadStatesHandheld) {
    static void Callback(nn::hid::NpadHandheldState* state, int unk1, const unsigned int& unk2) {
        Orig(state, unk1, unk2);
        controllerHook(state);
    }
};
HOOK_DEFINE_TRAMPOLINE(NpadStatesDual) {
    static void Callback(nn::hid::NpadJoyDualState* state, int unk1, const unsigned int& unk2) {
        Orig(state, unk1, unk2);
        controllerHook(state);
    }
};
HOOK_DEFINE_TRAMPOLINE(NpadStatesFullKey) {
    static void Callback(nn::hid::NpadFullKeyState* state, int unk1, const unsigned int& unk2) {
        Orig(state, unk1, unk2);
        controllerHook(state);
    }
};
HOOK_DEFINE_TRAMPOLINE(NpadStatesJoyLeft) {
    static void Callback(nn::hid::NpadJoyLeftState* state, int unk1, const unsigned int& unk2) {
        Orig(state, unk1, unk2);
        controllerHook(state);
    }
};
HOOK_DEFINE_TRAMPOLINE(NpadStatesJoyRight) {
    static void Callback(nn::hid::NpadJoyRightState* state, int unk1, const unsigned int& unk2) {
        Orig(state, unk1, unk2);
        controllerHook(state);
    }
};

void exlSetupPracticeTASHooks() {
    NpadStatesFullKey::InstallAtSymbol("_ZN2nn3hid13GetNpadStatesEPNS0_16NpadFullKeyStateEiRKj");
    NpadStatesHandheld::InstallAtSymbol("_ZN2nn3hid13GetNpadStatesEPNS0_17NpadHandheldStateEiRKj");
    NpadStatesDual::InstallAtSymbol("_ZN2nn3hid13GetNpadStatesEPNS0_16NpadJoyDualStateEiRKj");
    NpadStatesJoyLeft::InstallAtSymbol("_ZN2nn3hid13GetNpadStatesEPNS0_16NpadJoyLeftStateEiRKj");
    NpadStatesJoyRight::InstallAtSymbol("_ZN2nn3hid13GetNpadStatesEPNS0_17NpadJoyRightStateEiRKj");
}
