#pragma once

#include <tchar.h>
#include <windows.h>
#include <msctf.h>
#include <ctffunc.h>
#include <atlbase.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <UIAutomation.h>

#include <QtCore>
#include <QtWidgets>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "shell32.lib")

#define IF_FALSE_BREAK( var, expr ) if( ( (var) = (expr) ) == false ) break
#define IF_FALSE_BREAK_TO_DEBUG( var, expr, format, ... ) if( ( (var) = (expr) ) == false ) { nsCmn::PrintDebugString( nsCmn::Format( format, ## __VA_ARGS__ ) ); break; }
#define IF_SUCCESS_BREAK( var, expr ) if( ( (var) = (expr) ) == true ) break
#define IF_FAILED_BREAK( var, expr ) if( FAILED( ((var) = (expr)) ) ) break;
#define IF_FAILED_BREAK_TO_DEBUG( var, expr, format, ... ) if( FAILED( (var) = (expr) ) ) { nsCmn::PrintDebugString( nsCmn::Format( format, ## __VA_ARGS__ ) ); break; }
