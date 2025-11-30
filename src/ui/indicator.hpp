#pragma once

#include "ui_indicator.h"

class UiIndicator : public QDialog
{
    Q_OBJECT
public:
    explicit UiIndicator( QWidget* parent = nullptr, Qt::WindowFlags f = Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint );

private:
    Ui::dlgIndicator                    Ui;
};