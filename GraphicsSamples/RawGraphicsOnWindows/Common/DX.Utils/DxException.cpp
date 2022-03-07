#include "DxException.h"

namespace dx
{
    DxException::DxException(const std::string& msg, HRESULT hr)
        : std::runtime_error(msg)
        , m_hr(hr)
    {
        const _com_error err(hr);
        _comErrorMsg = err.ErrorMessage();
    }

    std::string HResultToString(HRESULT hr)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
        return std::string(s_str);
    }

    void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            const std::string errMsg = HResultToString(hr);

#if defined( DEBUG ) || defined( _DEBUG )
            OutputDebugStringA(errMsg.c_str());
#endif

            throw DxException(errMsg, hr);
        }
    }

    void ThrowIfFailed(HRESULT hr, ID3DBlob* errorBlob)
    {
        if (FAILED(hr))
        {
            std::string errMsg = HResultToString(hr);
            std::string errBlobMsg = errorBlob
                ? (char*)errorBlob->GetBufferPointer()
                : "";

#if defined( DEBUG ) || defined( _DEBUG )
            OutputDebugStringA(errMsg.c_str());
            if (errorBlob)
                OutputDebugStringA(errBlobMsg.c_str());
#endif

            throw DxException(errMsg + errBlobMsg, hr);
        }
    }

    void ThrowIfFailed(HRESULT hr, ComPtr<ID3DBlob>& errorBlob)
    {
        ThrowIfFailed(hr, errorBlob.Get());
    }
}