#pragma once

#include <Windows.h>
#include <imm.h>
#include <QWidget>
#include <QTimer>
#include <QMap>

#pragma comment(lib, "imm32.lib")

class QWndBorderOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit QWndBorderOverlay( QWidget* parent = nullptr );

    void setBorderColor( const QColor& color ) { m_borderColor = color; }
    void setBorderWidth( int width ) { m_borderWidth = width; }
    int getBorderWidth() const { return m_borderWidth; }
    QColor getBorderColor() const { return m_borderColor; }

    void updateBorder( HWND targetHwnd, bool isKorean );

protected:
    void paintEvent( QPaintEvent* event ) override;

private:
    QColor                              m_borderColor;
    int                                 m_borderWidth = 5;
};

//
// class IMEBorderIndicator : public QObject
// {
//     Q_OBJECT
//
// public:
//     explicit IMEBorderIndicator(QObject *parent = nullptr);
//     ~IMEBorderIndicator();
//
//     bool start();
//     void stop();
//
// private slots:
//     void checkActiveWindow();
//
// private:
//     static LRESULT CALLBACK WindowProc(int nCode, WPARAM wParam, LPARAM lParam);
//     static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
//     static IMEBorderIndicator* s_instance;
//
//     void updateIMEStatus(HWND hwnd);
//     bool getIMEStatus(HWND hwnd);
//     void updateBorderForWindow(HWND hwnd);
//
//     HHOOK m_hWndHook;
//     HHOOK m_hKeyboardHook;
//     HWND m_currentWindow;
//     BorderOverlay* m_overlay;
//     QTimer* m_timer;
//     bool m_lastIMEStatus;
// };
