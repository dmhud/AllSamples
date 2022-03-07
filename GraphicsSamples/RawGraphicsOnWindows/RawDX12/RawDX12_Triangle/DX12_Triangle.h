#pragma once

#include <cstdint>

#include <Windows.h>

class DX12_Triangle
{
public:
    void DrawTriangle(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight);
};