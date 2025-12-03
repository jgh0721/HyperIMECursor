#pragma once

#include "ui_indicator.h"

class UiIndicator : public QDialog
{
    Q_OBJECT
public:
    explicit UiIndicator( QWidget* parent = nullptr, Qt::WindowFlags f = Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput );

    void                                SetIMEMode( bool IsKoreanMode );
    void                                Show( int WaitSecs = 3 );

private:
    QTimer*                             Timer = nullptr;
    Ui::dlgIndicator                    Ui;
};