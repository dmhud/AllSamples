#pragma once

#include <cstdint>
#include <string>
#include <functional>

#include <Windows.h>

namespace win
{
    using SizeChangedCallback = std::function<void(uint32_t width, uint32_t height)>;

    class Window
    {
    public:
        /**
         * windowClassName - must be global unique for each registered window in WinAPI
         */
        Window(uint32_t windowWidth, uint32_t windowHeight,
            const std::wstring& windowTitle     = L"WindowTitle",
            const std::wstring& windowClassName = L"WindowClassName");
        ~Window();

        HWND HWnd()             const { return _hWnd; }
        uint32_t WindowWidth()  const { return _windowWidth; }
        uint32_t WindowHeight() const { return _windowHeight; }

        void Show() const;
        void SetWindowTitle(const std::wstring& str) const;

        static bool HandleMessage(MSG& msg);

    public:
        void SetSizeChangedCallback(SizeChangedCallback sizeChangedCallback);
        void OnWindowSizeChanged(uint32_t width, uint32_t height);

    private:
        HWND                _hWnd;
        HINSTANCE           _hInstance;
        std::wstring        _windowClassName;

        uint32_t            _windowWidth;
        uint32_t            _windowHeight;
        SizeChangedCallback _sizeChangedCallback;
    };
    
}