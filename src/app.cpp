#include "stdafx.h"
#include "app.hpp"
#include "CTaskSchedulerHelper.hpp"

#include "ui/about.hpp"
#include "ui/indicator_popup.hpp"
#include "ui/indicator_caret.hpp"

#include "imeborderindicator.hpp"
#include "main.hpp"
#include "settings.hpp"

#include "ui/opt.hpp"

///////////////////////////////////////////////////////////////////////////////
///

// 키보드 / 마우스 훅
HHOOK                                   KeyboardHook = nullptr;
HHOOK                                   MouseHook = nullptr;

// 상태 변수
std::atomic_bool                        IsKoreanMode = false;
std::atomic_bool                        IsKoreanModeOnHook = false; // 키보드 훅을 통해 직접 획득
uint64_t                                IMEActiveCheckTime = 0;

// 기타
CIMECursorApp*                          Main = nullptr;

using namespace nsCmn;

///////////////////////////////////////////////////////////////////////////////
///

CIMECursorApp::CIMECursorApp( int& argc, char* argv[] )
    : QApplication( argc, argv )
{
    Main = this;
    setWindowIcon( QIcon( ":/res/app_icon.ico" ) );
    QSettings::setDefaultFormat( QSettings::IniFormat );
    (void)QFontDatabase::addApplicationFont( ":res/SarasaMonoK-Light.ttf" );
}

CIMECursorApp::~CIMECursorApp()
{
    GetSettings()->sync();

    if( MouseHook )
        UnhookWindowsHookEx( MouseHook );
    MouseHook = nullptr;

    if( KeyboardHook )
        UnhookWindowsHookEx( KeyboardHook );
    KeyboardHook = nullptr;
}

void CIMECursorApp::Initialize()
{
    const auto StSettings = GetSettings();

    do
    {
        StSettings->sync();

        applyAutoStartUP();

        m_notificationMenu = new QMenu();
        m_notificationIcon = new QSystemTrayIcon();
        m_pTimer = new QTimer;
        m_pUiOpt = new UiOpt;
        m_pUiIndicatorCaret = new UiIndicator_Caret;
        m_pUiIndicatorPopup = new UiIndicator_Popup;
        connect( this, &CIMECursorApp::sigMouseWheel, m_pUiIndicatorCaret, &UiIndicator_Caret::SltMouseWheel );
        m_pUiIndicatorCaret->SetCheckIME( GET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_IME ).toBool() );
        m_pUiIndicatorCaret->SetCheckNumlock( GET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_NUMLOCK ).toBool() );
        m_pUiIndicatorCaret->SetPollingMs( GET_VALUE( OPTION_ENGINE_CARET_POLLING_MS ).toInt() );
        m_pUiIndicatorCaret->SetStyleSheet( GET_VALUE( OPTION_ENGINE_CARET_STYLESHEET ).toString() );
        m_pUiIndicatorCaret->SetOffset( QPoint( GET_VALUE( OPTION_ENGINE_CARET_OFFSET_X ).toInt(), GET_VALUE( OPTION_ENGINE_CARET_OFFSET_Y ).toInt() ) );

        connect( m_pUiOpt, &UiOpt::settingsChanged, this, &CIMECursorApp::settingsChanged );

        m_notificationMenu->addAction( tr( "옵션(&O)" ), [=]() {
            m_pUiOpt->exec();
        } );
        m_notificationMenu->addSeparator();
        m_notificationMenu->addAction( tr( "정보(&A)" ), [=]() {
            const auto Ui = new CAbout;
            Ui->setAttribute( Qt::WA_DeleteOnClose );
            Ui->show();
        } );
        m_notificationMenu->addAction( tr( "종료(&E)" ), [=](){ qApp->quit(); } );
        m_notificationIcon->setIcon( QIcon( ":/res/app_icon.ico" ) );
        m_notificationIcon->setContextMenu( m_notificationMenu );
        connect( m_notificationIcon, &QSystemTrayIcon::activated, [this]( QSystemTrayIcon::ActivationReason Reason ) {
            if( Reason == QSystemTrayIcon::DoubleClick )
            {
                m_pUiOpt->activateWindow();
                m_pUiOpt->exec();
            }
        });
        m_notificationIcon->show();

        updateIMEStatus();

        // m_pOverlay = new QWndBorderOverlay;

        const auto IsKeyboardHook = GET_VALUE( OPTION_DETECT_KEYBOARD_HOOK ).toBool();
        if( IsKeyboardHook == true )
        {
            KeyboardHook = SetWindowsHookExW( WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0 );

            if( KeyboardHook == nullptr )
                PrintDebugString( Format( L"키보드 훅 설치 실패 : %d", ::GetLastError() ) );
        }

        if( nsWin::PrivilegeHelper::IsRunningAsAdmin() == true &&
            GET_VALUE( OPTION_ENGINE_CARET_IS_USE ).toBool() == true )
        {
            MouseHook = SetWindowsHookExW( WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0 );

            if( MouseHook == nullptr )
                PrintDebugString( Format( L"마우스 훅 설치 실패 : %d", ::GetLastError() ) );
        }

        m_pTimer->connect( m_pTimer, &QTimer::timeout, this, &CIMECursorApp::updateIMEStatus );
        m_pTimer->setSingleShot( false );
        m_pTimer->setInterval( GET_VALUE( OPTION_DETECT_DETECT_POLLING_MS ).toInt() );
        m_pTimer->start();

    } while( false );
}

void CIMECursorApp::settingsChanged()
{
    applyAutoStartUP();

    const auto CurPollingMs = m_pTimer->interval();
    const auto NexPollingMs = GET_VALUE( OPTION_DETECT_DETECT_POLLING_MS ).toInt();
    if( CurPollingMs != NexPollingMs )
        m_pTimer->setInterval( NexPollingMs );

    if( GET_VALUE( OPTION_DETECT_KEYBOARD_HOOK ).toBool() == true )
    {
        if( KeyboardHook == nullptr )
        {
            KeyboardHook = SetWindowsHookExW( WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0 );
            if( KeyboardHook == nullptr )
                nsCmn::PrintDebugString( nsCmn::Format( L"[HyperIMEIndicator] 키보드 후킹 설정 실패!@!" ) );
        }
    }
    else
    {
        if( KeyboardHook )
            UnhookWindowsHookEx( KeyboardHook );
        KeyboardHook = nullptr;
    }

    if( nsWin::PrivilegeHelper::IsRunningAsAdmin() == true )
    {
        if( GET_VALUE( OPTION_ENGINE_CARET_IS_USE ).toBool() == true )
        {
            if( MouseHook == nullptr )
            {
                MouseHook = SetWindowsHookExW( WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0 );
                if( MouseHook == nullptr )
                    nsCmn::PrintDebugString( nsCmn::Format( L"[HyperIMEIndicator] 키보드 후킹 설정 실패!@!" ) );
            }
        }
        else
        {
            if( MouseHook == nullptr )
                UnhookWindowsHookEx( MouseHook );
            MouseHook = nullptr;
        }
    }

    m_pUiIndicatorCaret->SetCheckIME( GET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_IME ).toBool() );
    m_pUiIndicatorCaret->SetCheckNumlock( GET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_NUMLOCK ).toBool() );
    m_pUiIndicatorCaret->SetPollingMs( GET_VALUE( OPTION_ENGINE_CARET_POLLING_MS ).toInt() );
    m_pUiIndicatorCaret->SetStyleSheet( GET_VALUE( OPTION_ENGINE_CARET_STYLESHEET ).toString() );
    m_pUiIndicatorCaret->SetOffset( QPoint( GET_VALUE( OPTION_ENGINE_CARET_OFFSET_X ).toInt(), GET_VALUE( OPTION_ENGINE_CARET_OFFSET_Y ).toInt() ) );
}

void CIMECursorApp::updateIMEStatus()
{
    static const auto StSettings = GetSettings();

    const bool IsIMEMode = IsKoreanModeInIME();

    if( IsIMEMode == IsKoreanMode )
        return;

    IsKoreanMode = IsIMEMode;
    PrintDebugString( Format( L"IME 상태 : %s", IsIMEMode ? L"한글" : L"영문" ) );

    m_pUiIndicatorCaret->SetIMEMode( IsKoreanMode );
    if( GET_VALUE( OPTION_ENGINE_CARET_IS_USE ).toBool() == true )
        m_pUiIndicatorCaret->Show();

    m_pUiIndicatorPopup->SetIMEMode( IsKoreanMode );
    if( GET_VALUE( OPTION_ENGINE_POPUP_IS_USE ).toBool() == true )
        m_pUiIndicatorPopup->Show();

    // if( StSettings->value( OPTION_NOTIFY_SHOW_NOTIFICATION_ICON, OPTION_NOTIFY_SHOW_NOTIFICATION_ICON_DEFAULT ).toBool() == true )
    // {
    //     if( m_pTrayIcon != nullptr &&
    //         m_pTrayIcon->isVisible() )
    //     {
    //         if( IsKoreanMode )
    //             m_pTrayIcon->setIcon( QIcon( KOREAN_MODE_ICON ) );
    //         else
    //             m_pTrayIcon->setIcon( QIcon( ENGLISH_MODE_ICON ) );
    //
    //         m_pTrayIcon->setToolTip( QString( "IME 상태 : %1" ).arg( IsKoreanMode ? tr("한글") : tr("영문") ) );
    //         m_pTrayIcon->show();
    //     }
    // }
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

        const auto Future = std::async( std::launch::async, []() {
            std::this_thread::sleep_for( std::chrono::milliseconds( 15 ) );
            QMetaObject::invokeMethod( qApp, "updateIMEStatus", Qt::QueuedConnection );
        } );

    } while( false );

    return CallNextHookEx( KeyboardHook, nCode, wParam, lParam );
}

LRESULT CIMECursorApp::LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    do
    {
        if( nCode != HC_ACTION )
            break;

        if( wParam == WM_MOUSEWHEEL || wParam == WM_MOUSEHWHEEL )
            Q_EMIT Main->sigMouseWheel( QDateTime::currentDateTime(),
                                        QPoint( GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) ) );

    } while( false );

    return CallNextHookEx( MouseHook, nCode, wParam, lParam );
}

HRESULT CIMECursorApp::applyAutoStartUP()
{
    CTaskSchedulerHelper Helper;
    HRESULT Result = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    const auto IsRunningAsAdmin = nsWin::PrivilegeHelper::IsRunningAsAdmin();
    const auto IsLaunchOnBoot = GET_VALUE( OPTION_STARTUP_START_ON_WINDOWS_BOOT ).toBool();
    const auto IsLaunchOnBootAsAdmin = GET_VALUE( OPTION_STARTUP_START_AS_ADMIN_ON_WINDOWS_BOOT ).toBool();

    do
    {
        Result = Helper.Initialize();
        if( FAILED( Result ) )
            break;

        if( IsLaunchOnBoot )
        {
            if( Helper.TaskExists( SETTINGS_TASK_NAME ) == true )
            {
                Result = Helper.UnregisterTask( SETTINGS_TASK_NAME );
                if( FAILED( Result ) )
                    break;
            }

            CTaskSchedulerHelper::TaskSettings Task;

            Task.TaskName = SETTINGS_TASK_NAME;
            Task.Description = L"윈도가 시작될 때 HyperIMEIndicator 를 실행합니다";

            Task.FileFullPath = QDir::toNativeSeparators( QCoreApplication::applicationFilePath() ).toStdWString();
            Task.Trigger = CTaskSchedulerHelper::TriggerType::Logon;
            // 관리자 권한으로 작업 스케쥴러에 등록
            Task.RunLevel = IsLaunchOnBootAsAdmin ? CTaskSchedulerHelper::RunLevel::Highest : CTaskSchedulerHelper::RunLevel::LUA;
            Task.RunOnlyIfLoggedOn = true;
            Task.Enabled = true;

            // 작업 스케쥴러에 등록
            Result = Helper.RegisterTask( Task );
        }
        else
        {
            // 작업 스케쥴러에 작업이 있다면 해제함
            if( Helper.TaskExists( SETTINGS_TASK_NAME ) == true )
                Result = Helper.UnregisterTask( SETTINGS_TASK_NAME );
            else
                Result = S_OK;
        }

    } while( false );

    return Result;
}
