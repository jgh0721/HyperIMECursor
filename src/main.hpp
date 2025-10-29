#pragma once

#include <string>

namespace nsCmn
{
    std::string                         Format( const char* fmt, ... );
    std::wstring                        Format( const wchar_t* fmt, ... );
    void                                PrintDebugString( const std::wstring& Str );
}

HRESULT                                 InitializeTSF();
void                                    CloseTSF();

HRESULT                                 InitializeUIAutomation();
void                                    CloseUIAutomation();

bool                                    IsProcessInAppContainor( HWND hWnd );

bool                                    CreateMainWnd( HINSTANCE hInstance );
bool                                    CreateIndicatorWnd( HINSTANCE hInstance, HWND& hWnd );
void                                    CreateNotificationIcon( HWND hWnd );
void                                    DestroyNotificationIcon();

LRESULT CALLBACK                        MainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK                        HookWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK                        LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK                        LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam );

/*!
 * TSF 서비스를 이용하여 현재 입력기의 입력 모드 반환 
 * @return 1 = Korean, 0 = English, -1 = Unknown
 * IsKoreanIMELayout
 */
int                                     IsKoreanIMEUsingTSF();
// CheckIMEViaIMM32
int                                     IsKoreanIMEUsingIMM32( HWND hWnd );
int                                     IsKoreanIMEUsingKeyboardLayout( HWND hWnd );
 
/*!
 * 다양한 방법을 사용하여 IME 를 사용 중이고, 현재 한글 모드인지 확인
 * @return true = 한글 모드, false = 그외
 * IsKoreanIMEActive
 */
bool                                    IsKoreanModeInIME();

void                                    UpdateIMEStatus();
void                                    UpdateIndicatorPosition();
SIZE                                    GetCursorSize( HCURSOR Cursor );
void                                    DrawIndicator( HDC hdc, bool IsKoreanMode );