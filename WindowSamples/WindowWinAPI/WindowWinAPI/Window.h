#pragma once

#include <cstdint>
#include <string>
#include <functional>

#include <Windows.h>

using SizeChangedCallback = std::function<void(uint32_t width, uint32_t height)>;

class Window
{
public:
    Window(uint32_t windowWidth, uint32_t windowHeight);
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
    const char*         _windowClassName;

    uint32_t            _windowWidth;
    uint32_t            _windowHeight;
    SizeChangedCallback _sizeChangedCallback;
};

