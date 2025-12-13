#pragma once

class UiOpt;
class UiIndicator_Popup;
class UiIndicator_Caret;
class QWndBorderOverlay;

class CIMECursorApp : public QApplication
{
    Q_OBJECT
public:
    CIMECursorApp( int& argc, char* argv[] );
    ~CIMECursorApp() override;

    Q_INVOKABLE void                    Initialize();

Q_SIGNALS:
    void                                sigVScroll( const QDateTime& TimeStamp, bool IsUp );
    void                                sigMouseWheel( const QDateTime& TimeStamp, const QPoint& Pos );

protected:

    Q_INVOKABLE void                    settingsChanged();
    Q_INVOKABLE void                    updateIMEStatus();

private:
    static LRESULT CALLBACK             LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam );
    static LRESULT CALLBACK             LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam );
    HRESULT                             applyAutoStartUP();

    QMenu*                              m_notificationMenu = nullptr;
    QSystemTrayIcon*                    m_notificationIcon = nullptr;

    QTimer*                             m_pTimer = nullptr;
    // QPointer<QWndBorderOverlay>         m_pOverlay = nullptr;
    //
    QPointer<UiOpt>                     m_pUiOpt = nullptr;
    QPointer<UiIndicator_Caret>         m_pUiIndicatorCaret = nullptr;
    QPointer<UiIndicator_Popup>         m_pUiIndicatorPopup = nullptr;
};
