#pragma once

#include <string>
#include <filesystem>

#include <Windows.h>

namespace fs = std::filesystem;

namespace win
{
    template <typename TChar, typename TStringGetterFunc>
    std::basic_string<TChar> GetStringFromWindowsApi(TStringGetterFunc stringGetter, int initialSize = 0)
    {
        if (initialSize <= 0)
        {
            initialSize = MAX_PATH;
        }

        std::basic_string<TChar> result(initialSize, 0);
        for (;;)
        {
            auto length = stringGetter(&result[0], result.length());
            if (length == 0)
            {
                return std::basic_string<TChar>();
            }

            if (length < result.length() - 1)
            {
                result.resize(length);
                result.shrink_to_fit();
                return result;
            }

            result.resize(result.length() * 2);
        }
    }

    inline auto get_module_file_name()
    {
        auto moduleName = GetStringFromWindowsApi<TCHAR>([](TCHAR* buffer, int size)
            {
                const auto hInstance = GetModuleHandle(nullptr);
                return GetModuleFileName(hInstance, buffer, size);
            });
        return moduleName;
    }

    inline fs::path exe_path()
    {
        fs::path exeFileName = get_module_file_name();
        fs::path exePath = exeFileName.parent_path();
        return exePath;
    }

} // namespace win
