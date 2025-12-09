#pragma once

#include "ui_indicator_popup.h"

class UiIndicator_Popup : public QDialog
{
    Q_OBJECT
public:
    explicit UiIndicator_Popup( QWidget* parent = nullptr, Qt::WindowFlags f = Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput );

    void                                SetIMEMode( bool IsKoreanMode );
    void                                Show( int WaitSecs = 3 );

private:
    QTimer*                             Timer = nullptr;
    Ui::dlgIndicator                    Ui;
};