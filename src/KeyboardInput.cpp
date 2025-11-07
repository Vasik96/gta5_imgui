#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_styleGTA5/Include.hpp"
#include "KeyboardInput.h"
#include "globals.h"
#include "ProcessHandler.h"


std::unordered_map<int, bool> keyState;
HWND gtaWindow = nullptr;
DWORD gtaProcessId = 0;

// Define blocked keys for GTA/FiveM
const std::unordered_set<int> blockedKeys = {
    VK_RETURN, VK_ESCAPE, VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_TAB,
    0x54, 0x50, 0x51, 0x56, 0x52, 0x59, 0x46, 0x4D  // T, P, Q, V, R, Y, F, M
};

bool KeyboardInput::insertKeyPressed = false;
bool KeyboardInput::wasInsertKeyDown = false;


bool KeyboardInput::IsInGTA() {
    HWND foregroundWindow = GetForegroundWindow();
    DWORD processId;
    GetWindowThreadProcessId(foregroundWindow, &processId);

    if (!gtaWindow || !IsWindow(gtaWindow)) {
        gtaWindow = FindWindow(NULL, L"Grand Theft Auto V");
        if (!gtaWindow) {
            gtaWindow = FindWindow(NULL, L"FiveM");
        }
        if (gtaWindow) {
            GetWindowThreadProcessId(gtaWindow, &gtaProcessId);
        }
    }

    return (processId == gtaProcessId);
}


void KeyboardInput::ProcessRawInput(LPARAM lParam) {
    UINT dwSize;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

    std::vector<BYTE> lpb(dwSize);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
        return;
    }

    RAWINPUT* raw = (RAWINPUT*)lpb.data();
    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        RAWKEYBOARD& kb = raw->data.keyboard;
        int key = kb.VKey;
        bool keyDown = (kb.Flags & RI_KEY_BREAK) == 0;

        // Handle Insert key
        if (key == VK_INSERT) {
            insertKeyPressed = keyDown;
        }
        if (visibilityStatus != VISIBLE) {
            return;
        }

       

        // Block keys if in GTA/FiveM
        if (IsInGTA() && blockedKeys.count(key)) {
            std::cout << "Blocking key for GTA/FiveM: " << key << std::endl;
            return;  // Do not process blocked keys further
        }

        // Handle ImGui input
        ImGuiIO& io = ImGui::GetIO();
        if (keyDown && !keyState[key]) {  // Prevent duplicate key presses
            BYTE keyboardState[256];
            GetKeyboardState(keyboardState);
            WCHAR buffer[2];
            if (ToUnicodeEx((UINT)key, kb.MakeCode, keyboardState, buffer, 2, 1, GetKeyboardLayout(0)) > 0) {
                io.AddInputCharacter(buffer[0]);
            }
        }

        //special actions that imgui normally does not capture from this, so they HAVE to be done like this!
        if (key == VK_BACK && keyDown) {
            //cant think of any other better ways, io.AddInputCharacter does not work for backspace (backspace id: 127)
            io.AddKeyEvent(ImGuiKey_Backspace, true);
            io.AddKeyEvent(ImGuiKey_Backspace, false);
        }
        else if (key == VK_UP && keyDown) {
            io.AddKeyEvent(ImGuiKey_UpArrow, true);
            io.AddKeyEvent(ImGuiKey_UpArrow, false);
        }
        else if (key == VK_DOWN && keyDown) {
            io.AddKeyEvent(ImGuiKey_DownArrow, true);
            io.AddKeyEvent(ImGuiKey_DownArrow, false);
        }
        else if (key == VK_LEFT && keyDown) {
            io.AddKeyEvent(ImGuiKey_LeftArrow, true);
            io.AddKeyEvent(ImGuiKey_LeftArrow, false);
        }
        else if (key == VK_RIGHT && keyDown) {
            io.AddKeyEvent(ImGuiKey_RightArrow, true);
            io.AddKeyEvent(ImGuiKey_RightArrow, false);
        }
        else if (key == VK_RETURN && keyDown) {
            io.AddKeyEvent(ImGuiKey_Enter, true);
            io.AddKeyEvent(ImGuiKey_Enter, false);
        }

        // 0x<something> keys website: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
        /*else if (key == 0x50 && keyDown) { // this doesnt work, raw input does not capture something that the game uses :(
            io.AddKeyEvent(ImGuiKey_P, true);
            io.AddKeyEvent(ImGuiKey_P, false);
        }*/
        

        keyState[key] = keyDown;
    }
}


void KeyboardInput::RegisterRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;   // Generic Desktop Controls
    rid.usUsage = 0x06;       // Keyboard
    rid.dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY;  // Receive input even when not focused
    rid.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        std::cerr << "Failed to register Raw Input!" << std::endl;
    }
}