#include "stdafx.h"
#include "cmnUtils.hpp"

///////////////////////////////////////////////////////////////////////////////

namespace nsCmn
{
    __inline std::string format_arg_list( const char* fmt, va_list args )
    {
        if( !fmt ) return "";
        int   result = -1, length = 512;
        char* buffer = 0;
        while( result == -1 )
        {
            if( buffer )
                delete[] buffer;
            buffer = new char[ length + 1 ];
            memset( buffer, 0, ( length + 1 ) * sizeof( char ) );
            result = _vsnprintf_s( buffer, length, _TRUNCATE, fmt, args );
            length *= 2;
        }
        std::string s( buffer );
        delete[] buffer;
        return s;
    }

    __inline std::wstring format_arg_list( const wchar_t* fmt, va_list args )
    {
        if( !fmt ) return L"";
        int   result = -1, length = 512;
        wchar_t* buffer = 0;
        while( result == -1 )
        {
            if( buffer )
                delete[] buffer;
            buffer = new wchar_t[ length + 1 ];
            memset( buffer, 0, ( length + 1 ) * sizeof( wchar_t ) );
            result = _vsnwprintf_s( buffer, length, _TRUNCATE, fmt, args );
            length *= 2;
        }
        std::wstring s( buffer );
        delete[] buffer;
        return s;
    }

    std::string Format( const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        auto s = format_arg_list( fmt, args );
        va_end( args );
        return s;
    }

    std::wstring Format( const wchar_t* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        auto s = format_arg_list( fmt, args );
        va_end( args );
        return s;
    }

    void PrintDebugString( const std::wstring& Str )
    {
#if defined(_DEBUG)
        OutputDebugStringW( Str.c_str() );
        OutputDebugStringW( L"\n" );
#endif
    }
}
