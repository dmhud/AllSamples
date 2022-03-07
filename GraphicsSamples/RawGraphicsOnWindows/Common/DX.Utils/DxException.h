#pragma once

#include <string>

#include <d3d11.h>

#include <wrl/client.h>
#include <comdef.h>

#include <Windows.h>

namespace dx
{
    using namespace Microsoft::WRL;

    class DxException : public std::runtime_error
    {
    public:
        DxException(const std::string& msg, HRESULT hr);
        HRESULT Error() const { return m_hr; }

    private:
        HRESULT m_hr;
        std::wstring _comErrorMsg;
    };

    std::string HResultToString(HRESULT hr);
    void ThrowIfFailed(HRESULT hr);
    void ThrowIfFailed(HRESULT hr, ID3DBlob* errorBlob);
    void ThrowIfFailed(HRESULT hr, ComPtr<ID3DBlob>& errorBlob);
}