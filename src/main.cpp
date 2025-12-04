#include "stdafx.h"
#include "main.hpp"
#include "app.hpp"
#include "settings.hpp"

///////////////////////////////////////////////////////////////////////////////


HINSTANCE                               hInstance = nullptr;

using namespace nsCmn;

int WINAPI wWinMain( HINSTANCE hCurrInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow )
{
    int Ret = 0;
    CoInitializeEx( 0, COINIT_APARTMENTTHREADED );
    HANDLE Guard = ::CreateMutexW( nullptr, TRUE, L"_HyperIMECursor_" );

    do
    {
        if( GetLastError() == ERROR_ALREADY_EXISTS )
            break;

        hInstance = hCurrInstance;
        QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );

        CIMECursorApp App( __argc, __argv );
        Q_INIT_RESOURCE( app );
        App.setQuitOnLastWindowClosed( false );
        QMetaObject::invokeMethod( &App, "Initialize", Qt::QueuedConnection );
        Ret = App.exec();

    } while( false );

    CloseHandle( Guard );
    CoUninitialize();
    return Ret;
}

bool IsProcessInAppContainor( HWND hWnd )
{
    bool IsInAppContainor = false;

    do
    {
        if( hWnd == nullptr )
            break;

        wchar_t ClassName[ 256 ] = {0,};
        if( GetClassNameW( hWnd, ClassName, 256 ) != 0 )
        {
            if( wcscmp( ClassName, L"ApplicationFrameWindow" ) == 0 )
            {
                IsInAppContainor = true;
                break;
            }

            if( wcscmp( ClassName, L"Windows.UI.Core.CoreWindow" ) == 0 )
            {
                IsInAppContainor = true;
                break;
            }
        }

    } while( false );

    return IsInAppContainor;
}

int IsKoreanIMEUsingIMM32( HWND hWnd )
{
    int Result = -1;
    HIMC hIMC = nullptr;

    constexpr uint32_t IMC_GETOPENSTATUS = 0x5;
    constexpr uint32_t IMC_GETCONVERSIONMODE = 0x1;

    do
    {
        if( hWnd == nullptr )
            break;

        // 방법 1 : ImmGetContext 이용,
        // 방법 2 : ImmGetDefaultIMEWnd 사용

        hIMC = ImmGetContext( hWnd );
        if( hIMC != nullptr )
        {
            DWORD dwConversion = 0;
            DWORD dwSentence = 0;

            if( ImmGetConversionStatus( hIMC, &dwConversion, &dwSentence ) == FALSE )
                break;

            Result = ( dwConversion & IME_CMODE_NATIVE ) != 0 ? TRUE : FALSE;
        }
        else
        {
            const HWND hIME = ImmGetDefaultIMEWnd( hWnd );
            if( hIME )
            {
                DWORD_PTR MessageResult = 0;
                Result = SendMessageTimeoutW( hIME, WM_IME_CONTROL, IMC_GETCONVERSIONMODE, 0, SMTO_NORMAL | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 1000, &MessageResult );
                if( Result == FALSE )
                {
                    Result = -1;
                    break;
                }

                Result = ( MessageResult & IME_CMODE_NATIVE ) != 0 ? TRUE : FALSE;
            }
        }

    } while( false );

    if( hIMC )
        ImmReleaseContext( hWnd, hIMC );

    //PrintDebugString( Format( L"%s : Result = %d", __FUNCTIONW__, Result ) );
    return Result;
}

int IsKoreanIMEUsingKeyboardLayout( HWND hWnd )
{
    int Result = -1;

    do
    {
        if( hWnd == nullptr )
            break;

        DWORD dwThreadId = GetWindowThreadProcessId( hWnd, NULL );
        HKL hkl = GetKeyboardLayout( dwThreadId );
        WORD langId = LOWORD( hkl );

        // 한글 레이아웃인지 확인
        if( langId == MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN ) )
        {
            // 한글 레이아웃이지만 영문 모드일 수 있으므로
            // IMM32로 다시 확인
            Result = IsKoreanIMEUsingIMM32( hWnd );
        }

    } while( false );

    PrintDebugString( Format( L"%s : Result = %d", __FUNCTIONW__, Result ) );
    return Result;
}

bool IsKoreanModeInIME()
{
    bool IsIMEMode = false;
    static const auto StSettings = GetSettings();
    constexpr uint64_t                      IMEActiveCheckPeriod = 500;

    do
    {
        const auto CurrentTime = GetTickCount64();
        if( CurrentTime - IMEActiveCheckTime < IMEActiveCheckPeriod )
        {
            IsIMEMode = IsKoreanMode;
            break;
        }

        IMEActiveCheckTime = CurrentTime;

        int Result = -1;
        const auto IsAttachThreadUI = GET_VALUE( OPTION_DETECT_ATTACH_THREAD_INPUT ).toBool();

        HWND hWnd = GetForegroundWindow();
        const auto TargetThreadId = GetWindowThreadProcessId( hWnd, nullptr );
        const auto CurrentThreadId = GetCurrentThreadId();
        bool IsAttached = false;

        if( IsAttachThreadUI == true && CurrentThreadId == TargetThreadId )
            IsAttached = AttachThreadInput( CurrentThreadId, TargetThreadId, FALSE );

        do
        {
            // MS TEAMS 등 몇몇 앱은 IME 메시지를 처리하는 창이 전경 창이 아니라 내부의 다른 창이므로 실제 포커스 된 창을 가져온다.
            GUITHREADINFO guiInfo = { 0 };
            guiInfo.cbSize        = sizeof( GUITHREADINFO );
            if( GetGUIThreadInfo( TargetThreadId, &guiInfo ) == TRUE )
            {
                if( guiInfo.hwndFocus != nullptr )
                    hWnd = guiInfo.hwndFocus;
            }

            Result = IsKoreanIMEUsingIMM32( hWnd );
            if( Result == TRUE )
                IsIMEMode = true;
            if( Result == FALSE )
                IsIMEMode = false;
            if( Result >= 0 )
                IsKoreanModeOnHook = IsIMEMode;
            if( Result >= 0 )
                break;

            Result = IsKoreanIMEUsingKeyboardLayout( hWnd );

            if( Result == TRUE )
                IsIMEMode = true;
            if( Result == FALSE )
                IsIMEMode = false;
            if( Result >= 0 )
                IsKoreanModeOnHook = IsIMEMode;

        } while( false );

        if( IsAttachThreadUI == true && IsAttached == true )
            AttachThreadInput( CurrentThreadId, TargetThreadId, FALSE );

        // 어떤 것도 성공하지 못하면 키보드 훅 기반의 값을 반환
        IsIMEMode = IsKoreanModeOnHook;

    } while( false );

    return IsIMEMode;
}
