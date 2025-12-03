#pragma once

#include <string>

#include <QApplication>
#include <QSystemTrayIcon>

const auto KOREAN_MODE_ICON = ":/res/korean_icon.ico";
const auto ENGLISH_MODE_ICON = ":/res/english_icon.ico";

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

    HRESULT                             InitializeTSF();
    void                                CloseTSF();

protected:

    Q_INVOKABLE void                    updateIMEStatus();

private:
    static LRESULT CALLBACK             LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam );
    static LRESULT CALLBACK             LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam );

    QTimer*                             m_pTimer = nullptr;
    QSystemTrayIcon*                    m_pTrayIcon = nullptr;
    QPointer<QWndBorderOverlay>         m_pOverlay = nullptr;

    QPointer<UiOpt>                     m_pUiOpt = nullptr;
    QPointer<UiIndicator>               m_pUiIndicator = nullptr;
    QPointer<UiIndicator_Caret>         m_pUiIndicatorCaret = nullptr;
};

bool                                    IsProcessInAppContainor( HWND hWnd );

/*!
 * TSF 서비스를 이용하여 현재 입력기의 입력 모드 반환
 * @return 1 = Korean, 0 = English, -1 = Unknown
 * IsKoreanIMELayout
 */
int                                     IsKoreanIMEUsingTSF();
int                                     IsKoreanIMEUsingIMM32( HWND hWnd );
int                                     IsKoreanIMEUsingKeyboardLayout( HWND hWnd );

/*!
 * 다양한 방법을 사용하여 IME 를 사용 중이고, 현재 한글 모드인지 확인
 * @return true = 한글 모드, false = 그외
 * IsKoreanIMEActive
 */
bool                                    IsKoreanModeInIME();
//
// void                                    UpdateIndicatorPosition();
// SIZE                                    GetCursorSize( HCURSOR Cursor );
// void                                    DrawIndicator( HDC hdc, bool IsKoreanMode );