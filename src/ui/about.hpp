#pragma once

#include "ui_about.h"

class CAbout : public QDialog
{
    Q_OBJECT
public:
    explicit CAbout( QWidget* parent = nullptr );

private:
    Ui::dlgAbout                        Ui;
};