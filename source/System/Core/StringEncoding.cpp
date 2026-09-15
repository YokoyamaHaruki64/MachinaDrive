#include "pch.h"

#include "System/Core/StringEncoding.h"

namespace StringEncoding
{
    std::wstring Utf8ToWide(std::string_view text)
    {
        if (text.empty())
        {
            return {};
        }

        const int requiredLength = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            nullptr,
            0);
        if (requiredLength <= 0)
        {
            return {};
        }

        std::wstring result(static_cast<size_t>(requiredLength), L'\0');
        if (MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                text.data(),
                static_cast<int>(text.size()),
                result.data(),
                requiredLength) <= 0)
        {
            return {};
        }

        return result;
    }
}
