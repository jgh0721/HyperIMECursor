#pragma once

#include <string>

namespace nsCmn
{
    std::string                         Format( const char* fmt, ... );
    std::wstring                        Format( const wchar_t* fmt, ... );
    void                                PrintDebugString( const std::wstring& Str );
}
