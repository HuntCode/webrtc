#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtTestDriver.h"
#include "SignalingSocket.h"

class QtTestDriver : public QMainWindow
{
    Q_OBJECT

public:
    QtTestDriver(QWidget *parent = nullptr);
    ~QtTestDriver();

private slots:
    void onConnectClicked();

    // 信令事件
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QString& msg);
    void onSocketError(const QString& error);

private:
    Ui::QtTestDriverClass ui;
    SignalingSocket* signalingSocket_;
    bool isCaller_ = false;
};
