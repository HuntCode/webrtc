#ifndef QTTESTDRIVER_H
#define QTTESTDRIVER_H

#include "WebRTCClient.h"

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
    void onStartMirrorClicked();
    void onStopMirrorClicked();

    // 信令事件
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QString& msg);
    void onSocketError(const QString& error);

    void onLocalSdpReady(const std::string & type, const std::string& sdp);
    void onIceCandidateReady(const std::string& sdpMid,
                             int sdpMLineIndex,
                             const std::string& candidate);

private:
    Ui::QtTestDriverClass ui;
    SignalingSocket* signalingSocket_;
    rtc::scoped_refptr<WebRTCClient> webrtcClient_;
};

#endif  // QTTESTDRIVER_H