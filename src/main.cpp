#include "stdafx.h"
#include "main.hpp"
#include "imeborderindicator.hpp"
#include "settings.hpp"

#include "ui/opt.hpp"
#include "ui/indicator.hpp"
#include "ui/indicator_caret.hpp"

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
        QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );

        CIMECursorApp App( __argc, __argv );
        Q_INIT_RESOURCE( app );
        App.setQuitOnLastWindowClosed( false );
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

    //PrintDebugString( Format( L"%s : Result = %d", __FUNCTIONW__, Result ) );
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

        const auto IsAttachThreadUI = StSettings->value( OPTION_DETECT_ATTACH_THREAD_INPUT, OPTION_DETECT_ATTACH_THREAD_INPUT_DEFAULT ).toBool() ||
                                      StSettings->value( OPTION_NOTIFY_SHOW_CARET, OPTION_NOTIFY_SHOW_CARET_DEFAULT ).toBool();

        Result = IsKoreanIMEUsingTSF();

        if( Result == TRUE )
            IsIMEMode = true;
        if( Result == FALSE )
            IsIMEMode = false;
        if( Result >= 0 )
            IsKoreanModeOnHook = IsIMEMode;
        if( Result >= 0 )
            break;

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

CIMECursorApp::CIMECursorApp( int& argc, char* argv[] )
    : QApplication( argc, argv )
{
    m_pTimer = new QTimer( this );
    m_pTrayIcon = new QSystemTrayIcon( this );
    m_pOverlay = new QWndBorderOverlay;
    m_pUiOpt = new UiOpt;
    m_pUiIndicator = new UiIndicator;
    m_pUiIndicatorCaret = new UiIndicator_Caret;

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
        IF_FAILED_BREAK_TO_DEBUG( Hr, InitializeTSF(), L"TSF 서비스 초기화 실패 : 0x%08x", Hr );

        // MouseHook = SetWindowsHookExW( WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0 );
        // KeyboardHook = SetWindowsHookExW( WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0 );
        //
        // if( MouseHook == nullptr || KeyboardHook == nullptr )
        // {
        //     PrintDebugString( Format( L"키보드 마우스 훅 설치 실패 : %d", ::GetLastError() ) );
        //     break;
        // }

        updateIMEStatus();

        m_pTimer->connect( m_pTimer, &QTimer::timeout, this, &CIMECursorApp::updateIMEStatus );
        m_pTimer->setSingleShot( false );
        m_pTimer->setInterval( 1000 );
        m_pTimer->start();

        const auto Menu = new QMenu;
        Menu->addAction( tr("옵션(&O)"), [=]() {
            m_pUiOpt->show();
        } );
        Menu->addSeparator();
        Menu->addAction( tr("정보(&A)"), [=](){} );

        m_pTrayIcon->setIcon( QIcon( ":/res/app_icon.ico" ) );
        m_pTrayIcon->setContextMenu( Menu );
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
    const auto StSettings = GetSettings();
    const bool IsIMEMode = IsKoreanModeInIME();

    if( IsIMEMode == IsKoreanMode )
        return;

    IsKoreanMode = IsIMEMode;
    PrintDebugString( Format( L"IME 상태 : %s", IsIMEMode ? L"한글" : L"영문" ) );

    m_pUiIndicatorCaret->SetIMEMode( IsKoreanMode );
    m_pUiIndicatorCaret->Show();

    if( StSettings->value( OPTION_NOTIFY_SHOW_CARET, OPTION_NOTIFY_SHOW_CARET_DEFAULT ).toBool() == true || true )
    {
    }

    if( StSettings->value( OPTION_NOTIFY_SHOW_POPUP, OPTION_NOTIFY_SHOW_POPUP_DEFAULT ).toBool() == true )
    {
        m_pUiIndicator->SetIMEMode( IsKoreanMode );
        m_pUiIndicator->Show();
    }

    if( StSettings->value( OPTION_NOTIFY_SHOW_NOTIFICATION_ICON, OPTION_NOTIFY_SHOW_NOTIFICATION_ICON_DEFAULT ).toBool() == true )
    {
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
    }
}

LRESULT CIMECursorApp::LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    static const auto StSettings = GetSettings();

    if( nCode >= 0 && wParam == WM_MOUSEMOVE )
    {
        if( StSettings->value( OPTION_NOTIFY_SHOW_CURSOR, OPTION_NOTIFY_SHOW_CURSOR ).toBool() == true )
        {
            // UpdateIndicatorPosition();
        }
    }

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
