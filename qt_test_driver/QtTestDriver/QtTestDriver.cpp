#include "QtTestDriver.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

using namespace std::placeholders;

#define SIGNALING_PORT 8888

QtTestDriver::QtTestDriver(QWidget *parent)
    : QMainWindow(parent),
      signalingSocket_(new SignalingSocket(this)),
      webrtcClient_(rtc::make_ref_counted<WebRTCClient>()) {
    ui.setupUi(this);

    connect(ui.pushButton_connect, &QPushButton::clicked, this, &QtTestDriver::onConnectClicked);
    connect(ui.pushButton_startPush, &QPushButton::clicked, this, &QtTestDriver::onStartMirrorClicked);
    connect(ui.pushButton_stopPush, &QPushButton::clicked, this, &QtTestDriver::onStopMirrorClicked);

    connect(signalingSocket_, &SignalingSocket::connected, this, &QtTestDriver::onConnected);
    connect(signalingSocket_, &SignalingSocket::disconnected, this, &QtTestDriver::onDisconnected);
    connect(signalingSocket_, &SignalingSocket::messageReceived, this, &QtTestDriver::onMessageReceived);
    connect(signalingSocket_, &SignalingSocket::errorOccurred, this, &QtTestDriver::onSocketError);

    // 连接WebRTCClient信号
    webrtcClient_->onLocalSdpReady(std::bind(&QtTestDriver::onLocalSdpReady, this, ::_1, ::_2));
    webrtcClient_->onIceCandidateReady(std::bind(&QtTestDriver::onIceCandidateReady, this, ::_1, ::_2, ::_3));

    signalingSocket_->startListening(SIGNALING_PORT);
}

QtTestDriver::~QtTestDriver()
{
}

void QtTestDriver::onConnectClicked() {
    if (signalingSocket_->isConnected()) {
        qDebug() << "已经建立连接，无需重复连接";
        return;
    }

    QString ip = ui.lineEdit_ip->text().trimmed();
    quint16 port = ui.lineEdit_port->text().toUShort();

    if (ip.isEmpty() || port == 0) {
        qDebug() << "请输入 IP 和端口";
        return;
    }

    signalingSocket_->connectToHost(ip, port);
}

void QtTestDriver::onStartMirrorClicked() {
    if (!signalingSocket_->isConnected()) {
        qDebug() << "请先建立信令连接";
        return;
    }

    if (!webrtcClient_->init()) {
      qDebug() << "WebRTC 初始化失败";
      return;
    }

    webrtcClient_->createOffer();  // Caller 主动创建 Offer
}

void QtTestDriver::onStopMirrorClicked() {
    webrtcClient_->uninit();
}

// 连接成功
void QtTestDriver::onConnected() {
    if (signalingSocket_->role() == SignalingSocket::Role::Caller) {
        qDebug() << "[Caller] 信令连接建立，等待 Start 按钮开始镜像";
    } else {
        qDebug() << "[Callee] 已建立连接，等待对方发送 offer";
    }
}

// 接收消息（可以是 offer / answer / ice）
void QtTestDriver::onMessageReceived(const QString& msg) {
    qDebug() << "收到信令消息：" << msg;

    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    QString sdp = obj["sdp"].toString();

    if ((type == "offer" || type == "answer") && !sdp.isEmpty()) {
        if (!webrtcClient_->init()) {
          qDebug() << "WebRTC 初始化失败";
          return;
        }

        // 无论 caller 还是 callee 都要先设置远端 SDP
        webrtcClient_->setRemoteDescription(sdp.toStdString(),
                                          type.toStdString());

        if (type == "offer" &&
            signalingSocket_->role() == SignalingSocket::Role::Callee) {
            qDebug() << "收到对方 offer，准备创建 answer";
            webrtcClient_->createAnswer();
        }

    } else if (type == "ice") {
      webrtcClient_->addIceCandidate(obj["sdpMid"].toString().toStdString(),
                                     obj["sdpMLineIndex"].toInt(),
                                     obj["candidate"].toString().toStdString());
    }
}

void QtTestDriver::onLocalSdpReady(const std::string& type, const std::string& sdp) {
    QString typeStr = QString::fromStdString(type);
    QString sdpStr = QString::fromStdString(sdp);

    QMetaObject::invokeMethod(this, [this, typeStr, sdpStr]() {
      qDebug() << "[Caller] 本地 SDP 准备好，主线程中发送：" << typeStr;

      QJsonObject obj;
      obj["type"] = typeStr;
      obj["sdp"] = sdpStr;

      signalingSocket_->sendMessage(
          QJsonDocument(obj).toJson(QJsonDocument::Compact));
    });
}

void QtTestDriver::onIceCandidateReady(const std::string& sdpMid,
                                       int sdpMLineIndex,
                                       const std::string& candidate) {
    QString sdpMidStr = QString::fromStdString(sdpMid);
    QString candidateStr = QString::fromStdString(candidate);

    QMetaObject::invokeMethod(this, [this, sdpMidStr, sdpMLineIndex, candidateStr]() {
        qDebug() << "[Caller] 本地 ICECandidate 准备好，主线程中发送";

        QJsonObject obj;
        obj["type"] = "ice";
        obj["sdpMid"] = sdpMidStr;
        obj["sdpMLineIndex"] = sdpMLineIndex;
        obj["candidate"] = candidateStr;
        signalingSocket_->sendMessage(
          QJsonDocument(obj).toJson(QJsonDocument::Compact));
    });
}

void QtTestDriver::onDisconnected() {
    qDebug() << "对方断开连接";
}

void QtTestDriver::onSocketError(const QString& error) {
    qDebug() << "连接错误：" << error;
}