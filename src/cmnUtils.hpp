#pragma once

#include <string>

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(Param) ((int)(short)LOWORD(Param))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(Param) ((int)(short)HIWORD(Param))
#endif

namespace nsCmn
{
    std::string                         Format( const char* fmt, ... );
    std::wstring                        Format( const wchar_t* fmt, ... );
    void                                PrintDebugString( const std::wstring& Str );
}
