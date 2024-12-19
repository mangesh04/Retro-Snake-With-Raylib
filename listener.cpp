#include <windows.h>
#include <iostream>
using namespace std;
HHOOK hHook;

HWND hwnd;

// Hook procedure for capturing keyboard events
bool isMin()
{
    return IsIconic(hwnd);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{

    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *pKeyStruct = (KBDLLHOOKSTRUCT *)lParam;

        // if (wParam == WM_KEYDOWN)
        // {
        //     int keyCode = pKeyStruct->vkCode;
        // }

        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_SPACE) & 0x8000) && (GetAsyncKeyState(VK_MENU) & 0x8000))
        {
            // shortCutPressed = true;
            if (IsIconic(hwnd))
            {
                ShowWindow(hwnd, SW_RESTORE);
            }
            else
            {
                ShowWindow(hwnd, SW_MINIMIZE);
            }
        }

        // if (pKeyStruct->vkCode == VK_ESCAPE)
        // {
        //     PostQuitMessage(0);
        // }
    }

    // Call next hook in the chain
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}
void installHook()
{

    // Install the hook
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);

    if (hHook == NULL)
    {
        std::cerr << "Failed to install hook!" << std::endl;
    }
}
void uninstallHook()
{
    // Uninstall the hook before exiting
    UnhookWindowsHookEx(hHook);
}

void extra_features()
{
    hwnd = GetForegroundWindow();
    // hwnd = GetActiveWindow();
    if (hwnd)
    {
        // Remove the window from the taskbar and Alt+Tab

        // LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);

        // exStyle &= ~WS_EX_APPWINDOW; // Remove "application" window flag

        // exStyle |= WS_EX_TOOLWINDOW; // Add "tool window" flag

        // SetWindowLongA(hwnd, GWL_EXSTYLE, exStyle);

        // // Change the window style to WS_POPUP to avoid taskbar
        // LONG style = GetWindowLongA(hwnd, GWL_STYLE);
        // style &= ~WS_POPUP; // Ensure WS_POPUP is set
        // SetWindowLongA(hwnd, GWL_STYLE, style);
    }
}

void wait()
{
    MSG msg;
    GetMessage(&msg, NULL, 0, 0);
}

void startListeningKeys()
{
    installHook();
    wait();
    uninstallHook();
}

void SetRoundedWindow(int cornerRadius)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);

    // Calculate width and height of the window
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Create a rounded region
    HRGN region = CreateRoundRectRgn(0, 0, width + 1, height + 1, cornerRadius, cornerRadius);

    // Apply the region to the window
    SetWindowRgn(hwnd, region, TRUE);
}