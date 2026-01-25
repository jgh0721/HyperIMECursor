#include "stdafx.h"
#include "app.hpp"

/// Ui
#include "ui/indicator_caret.hpp"
#include "ui/indicator_popup.hpp"
#include "ui/opt.hpp"
#include "ui/about.hpp"

/// 내부 라이브러리
#include "main.hpp"
#include "settings.hpp"
#include "CTaskSchedulerHelper.hpp"
#include "CProcessMonitorETW.hpp"

/// 외부 라이브러리
/// 선언 헤더 파일
/// 플랫폼 파일

// // 키보드 / 마우스 훅
// std::atomic_int32_t                     HookCount { 0 };
HHOOK                                   KeyboardHook = nullptr;
HHOOK                                   MouseHook = nullptr;

/*!
 * HookCount
   시작 = 0
    후킹 시작 => 1
    예외 시작 => 0
    예외 종료 => 1, 다시 후킹 시작
    예외 시작 => 0
    예외 시작 => -1
    예외 종료 => 0
    예외 종료 => 1, 다시 후킹 시작
 */

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

    if( m_pProcessMonitor )
    {
        m_pProcessMonitor->Stop();
        m_pProcessMonitor->deleteLater();
    }

    StopHook();
}

void CIMECursorApp::Initialize()
{
    const auto StSettings = GetSettings();

    do
    {
        StSettings->sync();
        m_notificationMenu = new QMenu();
        m_notificationIcon = new QSystemTrayIcon();

        /// 통지영역 메뉴 초기화
        m_notificationMenu->addAction( tr( "옵션(&O)" ), this, &CIMECursorApp::ShowOptions );
        m_notificationMenu->addSeparator();
        m_notificationMenu->addAction( tr( "정보(&A)" ), this, &CIMECursorApp::ShowAbout );
        m_notificationMenu->addAction( tr( "종료(&E)" ), [=](){ qApp->quit(); } );
        m_notificationIcon->setIcon( QIcon( ":/res/app_icon.ico" ) );
        m_notificationIcon->setContextMenu( m_notificationMenu );
        connect( m_notificationIcon, &QSystemTrayIcon::activated, this, &CIMECursorApp::TrayActivated );
        m_notificationIcon->show();

        m_pTimer = new QTimer;
        m_pTimer->connect( m_pTimer, &QTimer::timeout, this, &CIMECursorApp::sltUpdateIMEStatus );
        m_pTimer->setSingleShot( false );
        m_pTimer->setInterval( GET_VALUE( OPTION_DETECT_DETECT_POLLING_MS ).toInt() );

        m_pUiOpt = new UiOpt;
        m_pUiOpt->LoadSettings();
        connect( m_pUiOpt, &UiOpt::settingsChanged, this, &CIMECursorApp::sltSettingsChanged );

        m_pUiIndicatorCaret = new UiIndicator_Caret;
        m_pUiIndicatorPopup = new UiIndicator_Popup;
        connect( this, &CIMECursorApp::sigMouseWheel, m_pUiIndicatorCaret, &UiIndicator_Caret::SltMouseWheel );
        connect( this, &CIMECursorApp::sigVScroll, m_pUiIndicatorCaret, &UiIndicator_Caret::SltVScroll );

        sltUpdateIMEStatus();

        m_pProcessMonitor = new CProcessMonitor(this);
        connect( m_pProcessMonitor, &CProcessMonitor::sigProcessStarted, this, &CIMECursorApp::sltProcessStarted );
        connect( m_pProcessMonitor, &CProcessMonitor::sigProcessTerminated, this, &CIMECursorApp::sltProcessTerminated );

        sltSettingsChanged();

        m_pTimer->start();
        m_pProcessMonitor->Start();
    } while( false );
}

void CIMECursorApp::StartHook()
{
    if( GET_VALUE( OPTION_DETECT_KEYBOARD_HOOK ).toBool() == true )
    {
        if( KeyboardHook == nullptr )
        {
            KeyboardHook = SetWindowsHookExW( WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0 );
            if( KeyboardHook == nullptr )
                nsCmn::PrintDebugString( nsCmn::Format( L"[HyperIMEIndicator] 키보드 후킹 설정 실패!@!" ) );
        }
    }

    if( GET_VALUE( OPTION_DETECT_MOUSE_HOOK ).toBool() == true )
    {
        if( MouseHook == nullptr )
        {
            MouseHook = SetWindowsHookExW( WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0 );
            if( MouseHook == nullptr )
                nsCmn::PrintDebugString( nsCmn::Format( L"[HyperIMEIndicator] 마우스 후킹 설정 실패!@!" ) );
        }
    }
}

void CIMECursorApp::StopHook()
{
    if( KeyboardHook )
        UnhookWindowsHookEx( KeyboardHook );
    KeyboardHook = nullptr;

    if( MouseHook )
        UnhookWindowsHookEx( MouseHook );
    MouseHook = nullptr;
}

void CIMECursorApp::TrayActivated( QSystemTrayIcon::ActivationReason Reason )
{
    if( Reason != QSystemTrayIcon::DoubleClick )
        return ;

    ShowOptions();
}

void CIMECursorApp::ShowOptions()
{
    if( m_pUiOpt == nullptr )
        return ;

    m_pUiOpt->show();
    m_pUiOpt->activateWindow();
}

void CIMECursorApp::ShowAbout()
{
    const auto Ui = new CAbout;
    Ui->setAttribute( Qt::WA_DeleteOnClose );
    Ui->show();
}

///////////////////////////////////////////////////////////////////////////////

void CIMECursorApp::sltSettingsChanged()
{
     applyAutoStartUP();

     const auto CurPollingMs = m_pTimer->interval();
     const auto NexPollingMs = GET_VALUE( OPTION_DETECT_DETECT_POLLING_MS ).toInt();
     if( CurPollingMs != NexPollingMs )
         m_pTimer->setInterval( NexPollingMs );

    m_State.IsCaretEnabled.store( GET_VALUE( OPTION_ENGINE_CARET_IS_USE ).toBool() );
    m_State.IsPopupEnabled.store( GET_VALUE( OPTION_ENGINE_POPUP_IS_USE ).toBool() );

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

    if( GET_VALUE( OPTION_DETECT_MOUSE_HOOK ).toBool() == true )
    {
        if( MouseHook == nullptr )
        {
            MouseHook = SetWindowsHookExW( WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0 );
            if( MouseHook == nullptr )
                nsCmn::PrintDebugString( nsCmn::Format( L"[HyperIMEIndicator] 마우스 후킹 설정 실패!@!" ) );
        }
    }
    else
    {
        if( MouseHook )
            UnhookWindowsHookEx( MouseHook );
        MouseHook = nullptr;
    }

     m_pUiIndicatorCaret->SetCheckIME( GET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_IME ).toBool() );
     m_pUiIndicatorCaret->SetCheckNumlock( GET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_NUMLOCK ).toBool() );
     m_pUiIndicatorCaret->SetPollingMs( GET_VALUE( OPTION_ENGINE_CARET_POLLING_MS ).toInt() );
     m_pUiIndicatorCaret->SetStyleSheet( GET_VALUE( OPTION_ENGINE_CARET_STYLESHEET ).toString() );
     m_pUiIndicatorCaret->SetOffset( QPoint( GET_VALUE( OPTION_ENGINE_CARET_OFFSET_X ).toInt(), GET_VALUE( OPTION_ENGINE_CARET_OFFSET_Y ).toInt() ) );
}

void CIMECursorApp::sltProcessStarted( const ProcessEventInfo& Info )
{
    qDebug() << "[Started]" << Info.ProcessId << " : " << Info.ExecutableFileName;
    if( GET_VALUE( OPTION_HOOK_EXCLUDE_IS_USE ).toBool() == false )
        return;

    QFileInfo Image( Info.ExecutableFileName );
    const auto Excludes = GET_VALUE( OPTION_HOOK_EXCLUDE_PROCESS_NAMES ).toString().split( "|", Qt::SkipEmptyParts );
    if( Excludes.contains( Image.fileName(), Qt::CaseInsensitive ) == true )
    {
        QWriteLocker Guard( &m_lckState );
        m_State.CurrentExcludes.append( Info.ProcessId );
        if( m_State.CurrentExcludes.size() == 1 )
        {
            if( m_pUiIndicatorCaret != nullptr )
                m_pUiIndicatorCaret->Hide();

            StopHook();
            qDebug() << "[HOK]" << "Paused";
        }
    }
}

void CIMECursorApp::sltProcessTerminated( const ProcessEventInfo& Info )
{
    qDebug() << "[Terminated]" << Info.ProcessId << " : " << Info.ExecutableFileName;
    if( GET_VALUE( OPTION_HOOK_EXCLUDE_IS_USE ).toBool() == false )
        return;

    {
        QReadLocker Guard( &m_lckState );
        const auto PrevExcludes = m_State.CurrentExcludes.size();
        if( PrevExcludes == 0 )
            return;
    }

    QWriteLocker Guard( &m_lckState );
    m_State.CurrentExcludes.removeOne( Info.ProcessId );

    if( m_State.CurrentExcludes.isEmpty() == true )
    {
        if( m_pUiIndicatorCaret != nullptr )
        {
            if( GET_VALUE( OPTION_ENGINE_CARET_IS_USE ).toBool() == true )
            m_pUiIndicatorCaret->Show();
        }

        StartHook();
    }
}

void CIMECursorApp::sltUpdateIMEStatus()
{
     static const auto StSettings = GetSettings();
     const bool IsIMEMode = IsKoreanModeInIME();

     if( IsIMEMode == IsKoreanMode )
         return;

     IsKoreanMode = IsIMEMode;
     PrintDebugString( Format( L"IME 상태 : %s", IsIMEMode ? L"한글" : L"영문" ) );

     m_pUiIndicatorCaret->SetIMEMode( IsKoreanMode );
     if( m_State.IsCaretEnabled.load( std::memory_order::memory_order_relaxed ) == true )
         m_pUiIndicatorCaret->Show();

     m_pUiIndicatorPopup->SetIMEMode( IsKoreanMode );
     if( m_State.IsPopupEnabled.load( std::memory_order::memory_order_relaxed ) == true )
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

///////////////////////////////////////////////////////////////////////////////
///

LRESULT CIMECursorApp::LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
     do
     {
         if( nCode != HC_ACTION )
             break;

         if( wParam != WM_KEYDOWN && wParam != WM_SYSKEYDOWN )
             break;

         const auto Info = reinterpret_cast< KBDLLHOOKSTRUCT* >( lParam );

         if( wParam == WM_KEYDOWN )
         {
             if( Info->vkCode == VK_UP ||
                 Info->vkCode == VK_DOWN )
             {
                 if( (GetAsyncKeyState(VK_CONTROL) & 0x8000) )
                 {
                     Q_EMIT Main->sigVScroll( QDateTime::currentDateTime(), Info->vkCode == VK_UP );
                 }

                 break;
             }

             if( Info->vkCode == VK_HANGUL )
             {
                 // 수동 토글 상태 변경
                 IsKoreanModeOnHook = !IsKoreanModeOnHook;
                 // 마지막 체크 시간 무효화
                 IMEActiveCheckTime = 0;

                 QThreadPool::globalInstance()->tryStart( []() {
                     QMetaObject::invokeMethod( qApp, "sltUpdateIMEStatus", Qt::QueuedConnection );
                 } );
             }
         }

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
         {
             QThreadPool::globalInstance()->tryStart( [wParam, lParam]() {
                 Q_EMIT Main->sigMouseWheel( QDateTime::currentDateTime(),
                                             QPoint( GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) ) );
             });
         }

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

