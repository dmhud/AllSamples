#include "Common/Win.Utils/Window.h"
#include "DX12_Triangle.h"

int main(int argc, char *argv[])
{
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
    win::Window window(windowWidth, windowHeight, L"RawDX12_Triangle", L"RawDX12_Triangle_WindowClassName");
    window.Show();

    // Sample:
    HWND hwnd = window.HWnd();
    DX12_Triangle dx;
    dx.DrawTriangle(hwnd, windowWidth, windowHeight);

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