#include "stdafx.h"
#include "imeborderindicator.hpp"

#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

// ========== BorderOverlay Implementation ==========
QWndBorderOverlay::QWndBorderOverlay( QWidget* parent )
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
{
    // m_borderWidth = IMEBorderSettings::instance().getBorderWidth();

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowFlags(windowFlags() | Qt::WindowTransparentForInput);

    // Windows specific: Make window click-through
    HWND hwnd  = ( HWND )winId();
    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_TRANSPARENT | WS_EX_LAYERED);
}

void QWndBorderOverlay::updateBorder( HWND targetHwnd, bool isKorean )
{
    // RECT rect;
    // if( !GetWindowRect(targetHwnd, &rect) )
    //     return;
    //
    // // Check if window is minimized
    // if( IsIconic(targetHwnd) )
    // {
    //     hide();
    //     return;
    // }
    //
    // // Update border width from settings
    // m_borderWidth = IMEBorderSettings::instance().getBorderWidth();
    //
    // // Get DWM extended frame bounds for more accurate borders
    // RECT frame = rect;
    // DwmGetWindowAttribute(targetHwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT));
    //
    // int x      = frame.left - m_borderWidth;
    // int y      = frame.top - m_borderWidth;
    // int width  = ( frame.right - frame.left ) + ( m_borderWidth * 2 );
    // int height = ( frame.bottom - frame.top ) + ( m_borderWidth * 2 );
    //
    // // Set color based on IME status from settings
    // auto&  settings  = IMEBorderSettings::instance();
    // QColor baseColor = isKorean ? settings.getKoreanBorderColor() : settings.getEnglishBorderColor();
    // baseColor.setAlpha(settings.getOpacity());
    // m_borderColor = baseColor;
    //
    // setGeometry(x, y, width, height);
    // show();
    // update();
}

void QWndBorderOverlay::paintEvent( QPaintEvent* event )
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw border
    QPen pen(m_borderColor, m_borderWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    QRect borderRect = rect().adjusted(m_borderWidth / 2, m_borderWidth / 2, -m_borderWidth / 2, -m_borderWidth / 2);
    painter.drawRect(borderRect);
}
