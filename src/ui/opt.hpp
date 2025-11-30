#pragma once

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include "ui_opt.h"

class UiOpt : public QDialog
{

private:
    Q_OBJECT

public:
    explicit UiOpt( QWidget *parent = nullptr, Qt::WindowFlags f = Qt::Dialog );

    void                                LoadSettings();
    void                                SaveSettings();

Q_SIGNALS:
    void                                settingsChanged();

protected:
    void                                closeEvent( QCloseEvent* Event ) override;

private:
    Ui::dlgOpt                          Ui;
};