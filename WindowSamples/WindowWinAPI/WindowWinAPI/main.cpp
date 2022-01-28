#include <iostream>

#include "Window.h"

int main(int argc, char* argv[])
{
    std::cout << "Hello WinAPI window!" << std::endl;

    Window window(800, 600);
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