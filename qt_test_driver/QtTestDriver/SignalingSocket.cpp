#include "SignalingSocket.h"
#include <QDebug>


SignalingSocket::SignalingSocket(QObject* parent)
    : QObject(parent) {
    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection, this, &SignalingSocket::onNewConnection);
}

void SignalingSocket::startListening(quint16 port) {
    if (!server_->listen(QHostAddress::Any, port)) {
      emit errorOccurred("监听失败: " + server_->errorString());
      return;
    }
    qDebug() << "监听中，端口：" << port;
}

void SignalingSocket::connectToHost(const QString& ip, quint16 port) {
    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::readyRead, this, &SignalingSocket::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &SignalingSocket::onSocketDisconnected);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this, &SignalingSocket::onSocketError);
    connect(socket_, &QTcpSocket::connected, this, &SignalingSocket::connected);

    socket_->connectToHost(ip, port);
}

void SignalingSocket::sendMessage(const QString& json) {
  if (!socket_)
    return;
  socket_->write(json.toUtf8() + '\n');  // 以换行符分隔消息
}

bool SignalingSocket::isConnected() const {
  return socket_ && socket_->state() == QAbstractSocket::ConnectedState;
}

void SignalingSocket::onNewConnection() {
    if (socket_)
      return;

    socket_ = server_->nextPendingConnection();

    connect(socket_, &QTcpSocket::readyRead, this, &SignalingSocket::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &SignalingSocket::onSocketDisconnected);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this, &SignalingSocket::onSocketError);

    qDebug() << "[Callee] 客户端连接成功";
    emit connected();
}

void SignalingSocket::onReadyRead() {
    buffer_ += socket_->readAll();
    while (true) {
      int newlineIndex = buffer_.indexOf('\n');
      if (newlineIndex == -1)
        break;

      QByteArray message = buffer_.left(newlineIndex);
      buffer_ = buffer_.mid(newlineIndex + 1);
      emit messageReceived(QString::fromUtf8(message));
    }
}

void SignalingSocket::onSocketDisconnected() {
    qDebug() << "连接已断开";
    emit disconnected();
    socket_->deleteLater();
    socket_ = nullptr;
}

void SignalingSocket::onSocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    emit errorOccurred(socket_->errorString());
}
