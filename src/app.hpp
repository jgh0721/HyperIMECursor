#pragma once

#include "CProcessMonitorETW.hpp"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>

class CIMECursorApp;
extern CIMECursorApp* Main;
class UiOpt;
class UiIndicator_Caret;
class UiIndicator_Popup;
// class QWndBorderOverlay;
class CProcessMonitor;

class CIMECursorApp : public QApplication
{
    Q_OBJECT
public:
    CIMECursorApp(int &argc, char* argv[] );
    ~CIMECursorApp() override;

    Q_INVOKABLE void                    Initialize();

    Q_INVOKABLE void                    StartHook();
    Q_INVOKABLE void                    StopHook();

public Q_SLOTS:
    void                                TrayActivated( QSystemTrayIcon::ActivationReason Reason );
    void                                ShowOptions();
    void                                ShowAbout();

Q_SIGNALS:
    void                                sigVScroll( const QDateTime& TimeStamp, bool IsUp );
    void                                sigMouseWheel( const QDateTime& TimeStamp, const QPoint& Pos );

protected Q_SLOTS:
    void                                sltSettingsChanged();
    void                                sltProcessStarted( const ProcessEventInfo& Info );
    void                                sltProcessTerminated( const ProcessEventInfo& Info );
    void                                sltUpdateIMEStatus();

private:
    static LRESULT CALLBACK             LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam );
    static LRESULT CALLBACK             LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam );
    HRESULT                             applyAutoStartUP();

private:
    struct TyState
    {
        std::atomic<bool>               IsCaretEnabled;
        std::atomic<bool>               IsPopupEnabled;

        // 예외 프로세스가 실행될 때마다 PID 를 넣고, 모든 프로세스가 종료되면 다시 시작한다.
        QVector< quint32 >              CurrentExcludes;
    };

    TyState                             m_State;
    QReadWriteLock                      m_lckState;

    QTimer*                             m_pTimer = nullptr;
    QPointer<UiOpt>                     m_pUiOpt = nullptr;
    QMenu*                              m_notificationMenu = nullptr;
    QSystemTrayIcon*                    m_notificationIcon = nullptr;

    QPointer<UiIndicator_Caret>         m_pUiIndicatorCaret = nullptr;
    QPointer<UiIndicator_Popup>         m_pUiIndicatorPopup = nullptr;

    QPointer<CProcessMonitor>           m_pProcessMonitor = nullptr;
};
