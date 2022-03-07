#include "Window.h"

#include <string>
#include <iostream>

namespace win
{
    LRESULT CALLBACK MessageProc(HWND, UINT, WPARAM, LPARAM);
    std::string GetLastErrorAsString();

    Window::Window(uint32_t windowWidth, uint32_t windowHeight, const std::wstring& windowTitle, const std::wstring& windowClassName)
        : _hWnd(nullptr)
        , _windowClassName(windowClassName)
        , _windowWidth(windowWidth)
        , _windowHeight(windowHeight)
    {
        // Get HINSTANCE
        _hInstance = (HINSTANCE)GetModuleHandle(nullptr);

        // Register window class
        WNDCLASSEX wndClassEx;
        wndClassEx.cbSize = sizeof(WNDCLASSEX);
        wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
        wndClassEx.lpfnWndProc = MessageProc;
        wndClassEx.cbClsExtra = 0L;
        wndClassEx.cbWndExtra = 0L;
        wndClassEx.hInstance = _hInstance;
        wndClassEx.hIcon = nullptr;
        wndClassEx.hCursor = nullptr;
        wndClassEx.hbrBackground = nullptr;
        wndClassEx.lpszMenuName = nullptr;
        wndClassEx.lpszClassName = _windowClassName.c_str();
        wndClassEx.hIconSm = nullptr;
        if (RegisterClassEx(&wndClassEx) == 0)
        {
            std::string str = "RegisterClassEx Failed.";
            str += "GetLastError: " + GetLastErrorAsString();
            throw std::exception(str.c_str());
        }

        // Create a window
        RECT rc = { 0, 0, windowWidth, windowHeight };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        _hWnd = CreateWindowEx(
            0,                                          // Optional window styles.
            _windowClassName.c_str(),                   // Window class
            windowTitle.c_str(),                        // Window text
            WS_OVERLAPPEDWINDOW,                        // Window style        
            CW_USEDEFAULT,                              // Size and position - X
            CW_USEDEFAULT,                              // Size and position - Y
            rc.right - rc.left,                         // Size and position - Width
            rc.bottom - rc.top,                         // Size and position - Height        
            NULL,                                       // Parent window    
            NULL,                                       // Menu
            _hInstance,                                 // Instance handle
            NULL                                        // Additional application data
        );
        if (!_hWnd)
        {
            std::string str = "CreateWindowW Failed.";
            str += "GetLastError: " + GetLastErrorAsString();
            throw std::exception(str.c_str());
        }

        SetPropW(_hWnd, L"WindowPtr", (void*)this);
    }

    Window::~Window()
    {
        // Cleanup
        ::DestroyWindow(_hWnd);
        ::UnregisterClass(_windowClassName.c_str(), _hInstance);
    }

    void Window::Show() const
    {
        ShowWindow(_hWnd, SW_SHOW);
        UpdateWindow(_hWnd);
    }

    void Window::SetWindowTitle(const std::wstring& str) const
    {
        SetWindowTextW(_hWnd, str.c_str());
    }

    bool Window::HandleMessage(MSG& msg)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            return true;
        }

        return false;
    }

    void Window::SetSizeChangedCallback(SizeChangedCallback sizeChangedCallback)
    {
        _sizeChangedCallback = sizeChangedCallback;
    }

    void Window::OnWindowSizeChanged(uint32_t width, uint32_t height)
    {
        //std::cout << "Widnow size: " << width << " x " << height << std::endl;
        _windowWidth = width;
        _windowHeight = height;

        const auto callback = _sizeChangedCallback;
        if (callback) callback(width, height);
    }

    // Called every time the App receives a message
    LRESULT CALLBACK MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Window* wnd = (Window*)GetPropW(hWnd, L"WindowPtr");

        switch (message)
        {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_SIZE: // Window size has been changed
        {
            uint32_t windowWidth = LOWORD(lParam);
            uint32_t windowHeight = HIWORD(lParam);
            if (wnd)
                wnd->OnWindowSizeChanged(windowWidth, windowHeight);
            return 0;
        }
        //case WM_CHAR:
        //    if (wParam == VK_ESCAPE)
        //        PostQuitMessage(0);
        //    return 0;
        //
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            lpMMI->ptMinTrackSize.x = 320;
            lpMMI->ptMinTrackSize.y = 240;
            return 0;
        }

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }

    //Returns the last Win32 error, in string format. Returns an empty string if there is no error.
    std::string GetLastErrorAsString()
    {
        //Get the error message ID, if any.
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0) {
            return std::string(); //No error message has been recorded
        }

        LPSTR messageBuffer = nullptr;

        //Ask Win32 to give us the string version of that message ID.
        //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        //Copy the error message into a std::string.
        std::string message(messageBuffer, size);

        //Free the Win32's string's buffer.
        LocalFree(messageBuffer);

        return message;
    }
} // namespace win