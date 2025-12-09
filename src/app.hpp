#pragma once

class UiOpt;
class UiIndicator;
class UiIndicator_Caret;
class QWndBorderOverlay;

class CIMECursorApp : public QApplication
{
    Q_OBJECT
public:
    CIMECursorApp( int& argc, char* argv[] );
    ~CIMECursorApp() override;

    Q_INVOKABLE void                    Initialize();

protected:

    Q_INVOKABLE void                    settingsChanged();
    Q_INVOKABLE void                    updateIMEStatus();

private:
    static LRESULT CALLBACK             LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam );
    HRESULT                             applyAutoStartUP();

    QMenu*                              m_notificationMenu = nullptr;
    QSystemTrayIcon*                    m_notificationIcon = nullptr;

    QTimer*                             m_pTimer = nullptr;
    // QPointer<QWndBorderOverlay>         m_pOverlay = nullptr;
    //
    QPointer<UiOpt>                     m_pUiOpt = nullptr;
    // QPointer<UiIndicator>               m_pUiIndicator = nullptr;
    QPointer<UiIndicator_Caret>         m_pUiIndicatorCaret = nullptr;
};
