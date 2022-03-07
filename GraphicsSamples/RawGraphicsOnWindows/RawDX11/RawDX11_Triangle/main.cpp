
#include "Common/Win.Utils/Window.h"

int main(int argc, char *argv[])
{
    win::Window window(800, 600, L"EmptyWindow", L"RunEmptyWindow_WindowClassName");
    window.Show();

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (window.HandleMessage(msg))
        {
            // Message handled by WinAPI
        }
    }

    return 0;
}