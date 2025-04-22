#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtTestDriver.h"

class QtTestDriver : public QMainWindow
{
    Q_OBJECT

public:
    QtTestDriver(QWidget *parent = nullptr);
    ~QtTestDriver();

private slots:
    void onConnectClicked();

private:
    Ui::QtTestDriverClass ui;
    QString role_; // caller or callee
};
