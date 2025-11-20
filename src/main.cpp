#include "stdafx.h"
#include "main.hpp"
#include "imeborderindicator.hpp"

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

///////////////////////////////////////////////////////////////////////////////

// TSF 관련
CComPtr<ITfThreadMgr>                   TSF_ThreadMgr;
CComPtr<ITfInputProcessorProfiles>      TSF_Profiles;
CComPtr<ITfInputProcessorProfileMgr>    TSF_ProfileMgr;
TfClientId                              TSF_dwThreadMgrEventCookie = TF_INVALID_COOKIE;

// 키보드 / 마우스 훅
HHOOK                                   KeyboardHook = nullptr;
HHOOK                                   MouseHook = nullptr;

// 상태 변수
std::atomic_bool                        IsKoreanMode = false;
std::atomic_bool                        IsKoreanModeOnHook = false; // 키보드 훅을 통해 직접 획득
constexpr uint64_t                      IMEActiveCheckPeriod = 500;
uint64_t                                IMEActiveCheckTime = 0;

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

        CIMECursorApp App( __argc, __argv );
        Q_INIT_RESOURCE( app );
        App.setQuitOnLastWindowClosed( false );
        Ret = App.exec();

    } while( false );

    CloseHandle( Guard );
    CoUninitialize();
    return Ret;
}

///////////////////////////////////////////////////////////////////////////////
//
// //// 통지영역 아이콘
// //NOTIFYICONDATAW                         NotificationIcon = {0,};
// //constexpr uint32_t                      NotificationIconId = 1;
// //constexpr uint32_t                      NotificationIconMsg = WM_USER + 1;
// //constexpr uint32_t                      NotificationMenu_About = 4000;
// //constexpr uint32_t                      NotificationMenu_Reset = 8000;
// //constexpr uint32_t                      NotificationMenu_Exit = 5000;
// //
// //// 표시기 설정
// //HWND                                    INDICATOR_HWND = nullptr;
// //constexpr int                           INDICATOR_SIZE = 24;  // 표시기 크기
// //constexpr int                           OFFSET_X = 20;        // 커서로부터의 X 오프셋
// //constexpr int                           OFFSET_Y = 20;        // 커서로부터의 Y 오프셋
// //int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow )
// //{
// //    int Result = 0;
// //
// //    do
// //    {
// //        IF_FALSE_BREAK_TO_DEBUG( IsSuccess, CreateMainWnd( hInstance ), L"주 화면 창 생성 실패 : %d", ::GetLastError() );
// //        IF_FALSE_BREAK_TO_DEBUG( IsSuccess, CreateIndicatorWnd( hInstance, INDICATOR_HWND ), L"표시기 생성 실패 : %d", ::GetLastError() );
// //
// //        UpdateIndicatorPosition();
// //
// //    } while( false );
// //
// //    if( INDICATOR_HWND )
// //        DestroyWindow( INDICATOR_HWND );
// //
// //    return Result;
// //}

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

// //bool CreateMainWnd( HINSTANCE hInstance )
// //{
// //    bool IsSuccess = false;
// //    WNDCLASSEX wcex = { 0 };
// //    const wchar_t WNDNAME[] = L"HyperIME Indicator Main Window";
// //    const wchar_t CLASSNAME[] = L"HyperIME Indicator Main Class";
// //
// //    do
// //    {
// //        // 메인 윈도우 클래스 등록
// //
// //        wcex.cbSize         = sizeof( WNDCLASSEX );
// //        wcex.style          = CS_HREDRAW | CS_VREDRAW;
// //        wcex.lpfnWndProc    = MainWndProc;
// //        wcex.hInstance      = hInstance;
// //        wcex.hIcon          = LoadIcon( NULL, IDI_APPLICATION );
// //        wcex.hCursor        = LoadCursor( NULL, IDC_ARROW );
// //        wcex.hbrBackground  = ( HBRUSH )( COLOR_WINDOW + 1 );
// //        wcex.lpszClassName  = CLASSNAME;
// //        wcex.hIconSm        = LoadIcon( NULL, IDI_APPLICATION );
// //
// //        IsSuccess = RegisterClassExW( &wcex ) != 0;
// //        if( IsSuccess == false )
// //            break;
// //
// //        const auto hWnd = CreateWindowW( CLASSNAME, WNDNAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr );
// //
// //        if( hWnd != nullptr )
// //        {
// //            ShowWindow( hWnd, SW_HIDE );
// //            UpdateWindow( hWnd );
// //        }
// //
// //        IsSuccess = hWnd != nullptr;
// //
// //    } while( false );
// //
// //    if( IsSuccess == false )
// //        UnregisterClassW( CLASSNAME, hInstance );
// //
// //    return IsSuccess;
// //}
// //
// //bool CreateIndicatorWnd( HINSTANCE hInstance, HWND& hWnd )
// //{
// //    bool IsSuccess = false;
// //    WNDCLASSEX wcex = { 0 };
// //    const wchar_t WNDNAME[] = L"HyperIME Indicator Window";
// //    const wchar_t CLASSNAME[] = L"HyperIME Indicator Class";
// //
// //    do
// //    {
// //        wcex.cbSize         = sizeof( WNDCLASSEX );
// //        wcex.style          = CS_HREDRAW | CS_VREDRAW;
// //        wcex.lpfnWndProc    = HookWndProc;
// //        wcex.hInstance      = hInstance;
// //        wcex.hCursor        = LoadCursor( NULL, IDC_ARROW );
// //        wcex.lpszClassName  = CLASSNAME;
// //
// //        IsSuccess = RegisterClassExW( &wcex ) != 0;
// //        if( IsSuccess == false )
// //            break;
// //
// //        // 레이어드 윈도우 생성
// //        hWnd = CreateWindowExW( WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
// //                                CLASSNAME, WNDNAME, WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, INDICATOR_SIZE, INDICATOR_SIZE,
// //                                NULL, NULL, hInstance, NULL );
// //        if( hWnd == nullptr )
// //            break;
// //
// //        // 투명도 설정 (알파 블렌딩)
// //        SetLayeredWindowAttributes( hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
// //        SetLayeredWindowAttributes( hWnd, 0, 240, LWA_ALPHA);
// //
// //        ShowWindow( hWnd, SW_SHOWNOACTIVATE);
// //        UpdateWindow( hWnd );
// //
// //    } while( false );
// //
// //    if( IsSuccess == false )
// //        UnregisterClassW( CLASSNAME, hInstance );
// //
// //    return IsSuccess;
// //}
// //
// //LRESULT MainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
// //{
// //    switch( message )
// //    {
// //        case WM_CREATE: {
// //            CreateNotificationIcon( hWnd );
// //            SetTimer( hWnd, IMECheckTimerId, 800, nullptr );
// //        } break;
// //
// //        case WM_TIMER: {
// //            if( wParam != IMECheckTimerId )
// //                break;
// //
// //            UpdateIMEStatus();
// //        } break;
// //        case NotificationIconMsg: {
// //            if( lParam != WM_RBUTTONUP )
// //                break;
// //
// //             POINT pt;
// //             GetCursorPos(&pt);
// //
// //             HMENU hMenu = CreatePopupMenu();
// //             AppendMenu(hMenu, MF_STRING, NotificationMenu_About, L"정보(&A)");
// //             AppendMenu(hMenu, MF_STRING, NotificationMenu_Reset, L"상태 리셋(&R)");
// //             AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
// //             AppendMenu(hMenu, MF_STRING, NotificationMenu_Exit, L"종료(&X)");
// //
// //             SetForegroundWindow(hWnd);
// //             TrackPopupMenu( hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr );
// //             DestroyMenu(hMenu);
// //        } break;
// //        case WM_COMMAND: {
// //            const auto Id = LOWORD( wParam );
// //            if( Id == NotificationMenu_Exit )
// //            {
// //                PostQuitMessage( 0 );
// //            }
// //            if( Id == NotificationMenu_Reset )
// //            {
// //                IsKoreanModeOnHook = false;
// //                IMEActiveCheckTime = 0;
// //                UpdateIMEStatus();
// //            }
// //            if( Id == NotificationMenu_About )
// //            {
// //            }
// //        } break;
// //        case WM_DESTROY: {
// //            KillTimer( hWnd, IMECheckTimerId );
// //            DestroyNotificationIcon();
// //
// //            if( INDICATOR_HWND )
// //            {
// //                DestroyWindow( INDICATOR_HWND );
// //                INDICATOR_HWND = nullptr;
// //            }
// //
// //            PostQuitMessage(0);
// //        } break;
// //
// //        default: {
// //            return DefWindowProc(hWnd, message, wParam, lParam);
// //        } break;
// //    }
// //
// //    return 0;
// //}
// //
// //LRESULT HookWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
// //{
// //    switch( message )
// //    {
// //        case WM_CREATE: {} break;
// //        case WM_PAINT: {
// //            PAINTSTRUCT ps;
// //            HDC hdc = BeginPaint( hWnd, &ps );
// //
// //            // 더블 버퍼링
// //            HDC hdcMem = CreateCompatibleDC( hdc );
// //            HBITMAP hbmMem = CreateCompatibleBitmap( hdc, INDICATOR_SIZE, INDICATOR_SIZE );
// //            HBITMAP hbmOld = ( HBITMAP )SelectObject( hdcMem, hbmMem );
// //
// //            // 배경 투명
// //            HBRUSH hBrush = CreateSolidBrush( RGB( 0, 0, 0 ) );
// //            RECT rect = { 0, 0, INDICATOR_SIZE, INDICATOR_SIZE };
// //            FillRect( hdcMem, &rect, hBrush );
// //            DeleteObject( hBrush );
// //
// //            // 표시기 그리기
// //            DrawIndicator( hdcMem, IsKoreanMode );
// //
// //            // 화면에 복사
// //            BitBlt( hdc, 0, 0, INDICATOR_SIZE, INDICATOR_SIZE, hdcMem, 0, 0, SRCCOPY );
// //
// //            SelectObject( hdcMem, hbmOld );
// //            DeleteObject( hbmMem );
// //            DeleteDC( hdcMem );
// //
// //            EndPaint( hWnd, &ps );
// //        } break;
// //
// //        case WM_DESTROY:
// //            return 0;
// //
// //        default: {
// //            return DefWindowProc( hWnd, message, wParam, lParam );
// //        } break;
// //    }
// //
// //    return 0;
// //}

int IsKoreanIMEUsingTSF()
{
    int Result = -1;
    constexpr GUID CLSID_MS_KOREAN_IME = { 0xB5FE1F02, 0xD5F2, 0x4445, {0x9C, 0x03, 0xC5, 0x68, 0xF2, 0x3C, 0x99, 0xA1} };

    do
    {
        if( TSF_ProfileMgr == nullptr )
            break;

        TF_INPUTPROCESSORPROFILE Profile = {0};
        HRESULT Hr = TSF_ProfileMgr->GetActiveProfile( GUID_TFCAT_TIP_KEYBOARD, &Profile );
        if( FAILED( Hr ) )
            break;

        if( Profile.langid != MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN ) )
            break;

        if( Profile.hkl == nullptr )
        break;

        const WORD LangID = LOWORD( Profile.hkl );
        Result = LangID == MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN ) ? 1 : 0;

    } while( false );

    PrintDebugString( Format( L"%s : Result = %d", __FUNCTIONW__, Result ) );
    return Result;
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
                const auto R = SendMessageW( hIME, WM_IME_CONTROL, IMC_GETCONVERSIONMODE, 0 );
                Result = ( R & IME_CMODE_NATIVE ) != 0 ? TRUE : FALSE;
            }
        }

    } while( false );

    if( hIMC )
        ImmReleaseContext( hWnd, hIMC );

    PrintDebugString( Format( L"%s : Result = %d", __FUNCTIONW__, Result ) );
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

        Result = IsKoreanIMEUsingTSF();

        if( Result == TRUE )
            IsIMEMode = true;
        if( Result == FALSE )
            IsIMEMode = false;
        if( Result >= 0 )
            IsKoreanModeOnHook = IsIMEMode;
        if( Result >= 0 )
            break;

        const HWND hWnd = GetForegroundWindow();

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

        // 어떤 것도 성공하지 못하면 키보드 훅 기반의 값을 반환
        IsIMEMode = IsKoreanModeOnHook;

    } while( false );

    return IsIMEMode;
}

// //void UpdateIndicatorPosition()
// //{
// //    if( INDICATOR_HWND == nullptr )
// //        return;
// //
// //    CURSORINFO ci = {0,};
// //    ci.cbSize = sizeof( ci );
// //    GetCursorInfo( &ci );
// //    const auto CursorSize = GetCursorSize( ci.hCursor );
// //
// //    // 화면 경계 확인
// //    int screenWidth = GetSystemMetrics( SM_CXSCREEN );
// //    int screenHeight = GetSystemMetrics( SM_CYSCREEN );
// //
// //    int x = ci.ptScreenPos.x + CursorSize.cx + OFFSET_X;
// //    int y = ci.ptScreenPos.y + CursorSize.cy + OFFSET_Y;
// //
// //    // 화면 밖으로 나가지 않도록
// //    if( x + INDICATOR_SIZE > screenWidth )
// //        x = ci.ptScreenPos.x - OFFSET_X - INDICATOR_SIZE;
// //
// //    if( y + INDICATOR_SIZE > screenHeight )
// //        y = ci.ptScreenPos.y - OFFSET_Y - INDICATOR_SIZE;
// //
// //    SetWindowPos( INDICATOR_HWND, HWND_TOPMOST, x, y, 0, 0,
// //                  SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW );
// //}
// //
// //SIZE GetCursorSize( HCURSOR Cursor )
// //{
// //    SIZE res = { 0 };
// //    if( Cursor )
// //    {
// //        ICONINFO info = { 0 };
// //        if( ::GetIconInfo( Cursor, &info ) != 0 )
// //        {
// //            bool bBWCursor = ( info.hbmColor == NULL );
// //            BITMAP bmpinfo = { 0 };
// //            if( ::GetObject( info.hbmMask, sizeof( BITMAP ), &bmpinfo ) != 0 )
// //            {
// //                res.cx = bmpinfo.bmWidth;
// //                res.cy = abs( bmpinfo.bmHeight ) / ( bBWCursor ? 2 : 1 );
// //            }
// //
// //            ::DeleteObject( info.hbmColor );
// //            ::DeleteObject( info.hbmMask );
// //        }
// //    }
// //    return res;
// //}
// //
// //void DrawIndicator( HDC hdc, bool IsKoreanMode )
// //{
// //    Graphics graphics( hdc );
// //    graphics.SetSmoothingMode( SmoothingModeAntiAlias );
// //    graphics.SetTextRenderingHint( TextRenderingHintAntiAlias );
// //
// //    // 배경색 설정
// //    Color bgColor = IsKoreanMode ? Color(220, 255, 100, 100) : Color(220, 100, 100, 255);
// //    Color textColor = Color(255, 255, 255, 255);
// //
// //    // 그림자
// //    SolidBrush shadowBrush(Color(80, 0, 0, 0));
// //    graphics.FillEllipse(&shadowBrush, 3, 3, INDICATOR_SIZE - 4, INDICATOR_SIZE - 4);
// //
// //    // 배경 원
// //    LinearGradientBrush gradientBrush(
// //     Point(0, 0), Point(INDICATOR_SIZE, INDICATOR_SIZE),
// //     bgColor,
// //     Color(200, bgColor.GetR() - 30, bgColor.GetG() - 30, bgColor.GetB() - 30)
// //    );
// //    graphics.FillEllipse(&gradientBrush, 1, 1, INDICATOR_SIZE - 2, INDICATOR_SIZE - 2);
// //
// //    // 테두리
// //    Pen borderPen(Color(255, 255, 255, 255), 1.5f);
// //    graphics.DrawEllipse(&borderPen, 1, 1, INDICATOR_SIZE - 2, INDICATOR_SIZE - 2);
// //
// //    // 텍스트
// //    FontFamily fontFamily(L"맑은 고딕");
// //    Font font(&fontFamily, 10, FontStyleBold, UnitPixel);
// //    SolidBrush textBrush(textColor);
// //
// //    StringFormat stringFormat;
// //    stringFormat.SetAlignment(StringAlignmentCenter);
// //    stringFormat.SetLineAlignment(StringAlignmentCenter);
// //
// //    RectF rect(0.0f, 0.0f, (REAL)INDICATOR_SIZE, (REAL)INDICATOR_SIZE);
// //    graphics.DrawString( IsKoreanMode ? L"가" : L"A", -1, &font, rect, &stringFormat, &textBrush);
// //}

CIMECursorApp::CIMECursorApp( int& argc, char* argv[] )
    : QApplication( argc, argv )
{
    m_pTimer = new QTimer( this );
    m_pTrayIcon = new QSystemTrayIcon( this );
    m_pOverlay = new QWndBorderOverlay;

    QMetaObject::invokeMethod( this, "Initialize", Qt::QueuedConnection );
}

CIMECursorApp::~CIMECursorApp()
{
    if( KeyboardHook )
        UnhookWindowsHookEx( KeyboardHook );
    KeyboardHook = nullptr;
    if( MouseHook )
        UnhookWindowsHookEx( MouseHook );
    MouseHook = nullptr;

    CloseTSF();
}

void CIMECursorApp::Initialize()
{
    HRESULT Hr = S_OK;

    do
    {
        QWidget* w = new QWidget;

        w->resize( 100, 100 );
        w->setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint );
        QVBoxLayout* pLayout = new QVBoxLayout( w );
        pLayout->setContentsMargins( 0, 0, 0, 0 );
        pLayout->setSpacing( 0 );
        QLabel* pLabel = new QLabel( w );
        pLayout->addWidget( pLabel );
        pLabel->setText("영문");
        pLabel->setFont( QFont( pLabel->font().family(), 14 ) );
        w->setWindowModality( Qt::ApplicationModal );
        w->show();

        const QString f = "D:/Dev/SetOffTestSignDriver.cmd";
        const auto re = QFileInfo(f).canonicalFilePath();
        const auto rew = QFileInfo(f).canonicalPath();

        IF_FAILED_BREAK_TO_DEBUG( Hr, InitializeTSF(), L"TSF 서비스 초기화 실패 : 0x%08x", Hr );

        MouseHook = SetWindowsHookExW( WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0 );
        KeyboardHook = SetWindowsHookExW( WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0 );

        if( MouseHook == nullptr || KeyboardHook == nullptr )
        {
            PrintDebugString( Format( L"키보드 마우스 훅 설치 실패 : %d", ::GetLastError() ) );
            break;
        }

        updateIMEStatus();

        m_pTimer->connect( m_pTimer, &QTimer::timeout, this, &CIMECursorApp::updateIMEStatus );
        m_pTimer->setSingleShot( false );
        m_pTimer->setInterval( 1000 );
        m_pTimer->start();

        m_pTrayIcon->setIcon( QIcon( ":/res/app_icon.ico" ) );
        m_pTrayIcon->setToolTip( "HyperIMECursor" );
        m_pTrayIcon->show();

    } while( false );

    if( FAILED(Hr) )
        qApp->exit( -1 );
}

HRESULT CIMECursorApp::InitializeTSF()
{
    HRESULT Hr = S_OK;

    do
    {
        Hr = CoCreateInstance( CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER,
                               IID_ITfThreadMgr, reinterpret_cast< void** >( &TSF_ThreadMgr ) );

        if( FAILED( Hr ) )
            break;

        Hr = TSF_ThreadMgr->Activate( &TSF_dwThreadMgrEventCookie );
        if( FAILED( Hr ) )
            break;

        Hr = CoCreateInstance( CLSID_TF_InputProcessorProfiles, nullptr,
                             CLSCTX_INPROC_SERVER,
                             IID_ITfInputProcessorProfiles,
                             reinterpret_cast< void** >( &TSF_Profiles ) );

        if( SUCCEEDED( Hr ) )
        {
            Hr = TSF_Profiles->QueryInterface( IID_ITfInputProcessorProfileMgr,
                                               reinterpret_cast< void** >( &TSF_ProfileMgr ) );
            if( SUCCEEDED( Hr ) )
                break;
        }

        Hr = CoCreateInstance( CLSID_TF_InputProcessorProfiles, NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_ITfInputProcessorProfileMgr,
                             reinterpret_cast< void** >( &TSF_ProfileMgr ) );

    } while( false );

    return Hr;
}

void CIMECursorApp::CloseTSF()
{
    if ( TSF_ProfileMgr )
        TSF_ProfileMgr.Release();

    if( TSF_Profiles )
        TSF_Profiles.Release();

    if ( TSF_ThreadMgr && TSF_dwThreadMgrEventCookie != TF_INVALID_COOKIE)
    {
        TSF_ThreadMgr->Deactivate();
        TSF_dwThreadMgrEventCookie = TF_INVALID_COOKIE;
    }

    if ( TSF_ThreadMgr )
        TSF_ThreadMgr.Release();
}

void CIMECursorApp::updateIMEStatus()
{
    PrintDebugString( Format( L"%s", __FUNCTIONW__ ) );

    const bool IsIMEMode = IsKoreanModeInIME();

    if( IsIMEMode == IsKoreanMode )
        return;

    IsKoreanMode = IsIMEMode;
    PrintDebugString( Format( L"IME 상태 : %s", IsIMEMode ? L"한글" : L"영문" ) );

    // if( INDICATOR_HWND )
    // {
    //     InvalidateRect( INDICATOR_HWND, nullptr, TRUE );
    //     UpdateWindow( INDICATOR_HWND ); // Send WM_PAINT
    // }

    if( m_pTrayIcon != nullptr &&
        m_pTrayIcon->isVisible() )
    {
        if( IsKoreanMode )
            m_pTrayIcon->setIcon( QIcon( KOREAN_MODE_ICON ) );
        else
            m_pTrayIcon->setIcon( QIcon( ENGLISH_MODE_ICON ) );

        m_pTrayIcon->setToolTip( QString( "IME 상태 : %1" ).arg( IsKoreanMode ? tr("한글") : tr("영문") ) );
        m_pTrayIcon->show();
    }

    // if( NotificationIcon.cbSize > 0 )
    // {
    //     swprintf_s( NotificationIcon.szTip, L"IME 상태: %s", IsIMEMode ? L"한글" : L"영문" );
    //     Shell_NotifyIconW( NIM_MODIFY, &NotificationIcon );
    // }
}

LRESULT CIMECursorApp::LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    //    if( nCode >= 0 && wParam == WM_MOUSEMOVE )
    //        UpdateIndicatorPosition();
    //
    return CallNextHookEx( MouseHook, nCode, wParam, lParam );
}

LRESULT CIMECursorApp::LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    do
    {
        if( nCode != HC_ACTION )
            break;

        if( wParam != WM_KEYDOWN && wParam != WM_SYSKEYDOWN )
            break;

        const auto Info = reinterpret_cast< KBDLLHOOKSTRUCT* >( lParam );
        if( Info->vkCode != VK_HANGUL )
            break;

        // 수동 토글 상태 변경
        IsKoreanModeOnHook = !IsKoreanModeOnHook;
        // 마지막 체크 시간 무효화
        IMEActiveCheckTime = 0;

        const auto future = std::async( std::launch::async, []() {
            std::this_thread::sleep_for( std::chrono::milliseconds( 15 ) );
            QMetaObject::invokeMethod( qApp, "updateIMEStatus", Qt::QueuedConnection );
        } );

    } while( false );

    return CallNextHookEx( KeyboardHook, nCode, wParam, lParam );
}
