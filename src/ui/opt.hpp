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

protected Q_SLOTS:
    void                                on_btnOK_clicked( bool checked = false );
    void                                on_btnCancel_clicked( bool checked = false );

private:
    Ui::dlgOpt                          Ui;
};