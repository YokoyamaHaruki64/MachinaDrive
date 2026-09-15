#pragma once

#include <string>
#include <string_view>

namespace StringEncoding
{
    std::wstring Utf8ToWide(std::string_view text);
}
