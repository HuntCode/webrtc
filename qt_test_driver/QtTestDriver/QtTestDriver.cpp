#include "QtTestDriver.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#define SIGNALING_PORT 8888

QtTestDriver::QtTestDriver(QWidget *parent)
    : QMainWindow(parent), signalingSocket_(new SignalingSocket(this))
{
    ui.setupUi(this);

    connect(ui.pushButton_connect, &QPushButton::clicked, this, &QtTestDriver::onConnectClicked);

    connect(signalingSocket_, &SignalingSocket::connected, this, &QtTestDriver::onConnected);
    connect(signalingSocket_, &SignalingSocket::disconnected, this, &QtTestDriver::onDisconnected);
    connect(signalingSocket_, &SignalingSocket::messageReceived, this, &QtTestDriver::onMessageReceived);
    connect(signalingSocket_, &SignalingSocket::errorOccurred, this, &QtTestDriver::onSocketError);

    signalingSocket_->startListening(SIGNALING_PORT);
}

QtTestDriver::~QtTestDriver()
{
}

void QtTestDriver::onConnectClicked() {
  QString ip = ui.lineEdit_ip->text().trimmed();
  quint16 port = ui.lineEdit_port->text().toUShort();

  if (ip.isEmpty() || port == 0) {
    qDebug() << u8"请输入 IP 和端口";
    return;
  }

  signalingSocket_->connectToHost(ip, port);
}

// 连接成功
void QtTestDriver::onConnected() {
  if (signalingSocket_->role() == Role::Caller) {
    qDebug() << u8"[Caller] 已连接对方，准备发送 offer";

    // 示例信令消息（WebRTC 对接时替换）
    QJsonObject json;
    json["type"] = "offer";
    json["sdp"] = "dummy-offer-sdp";
    signalingSocket_->sendMessage(QJsonDocument(json).toJson(QJsonDocument::Compact));
  } else {
    qDebug() << u8"[Callee] 已建立连接，等待 offer";
  }
}

// 接收消息（可以是 offer / answer / ice）
void QtTestDriver::onMessageReceived(const QString& msg) {
  qDebug() << u8"收到信令消息：" << msg;

  QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
  if (!doc.isObject())
    return;

  QJsonObject obj = doc.object();
  QString type = obj["type"].toString();

  if (type == "offer" && signalingSocket_->role() == Role::Callee) {
    qDebug() << u8"收到对方 offer，回传 answer";

    // 示例应答（WebRTC 对接时替换）
    QJsonObject answerJson;
    answerJson["type"] = "answer";
    answerJson["sdp"] = "dummy-answer-sdp";
    signalingSocket_->sendMessage(QJsonDocument(answerJson).toJson(QJsonDocument::Compact));

  } else if (type == "answer") {
    qDebug() << u8"收到对方 answer，P2P 应该已建立";
  }
}

void QtTestDriver::onDisconnected() {
  qDebug() << u8"对方断开连接";
}

void QtTestDriver::onSocketError(const QString& error) {
  qDebug() << u8"连接错误：" << error;
}