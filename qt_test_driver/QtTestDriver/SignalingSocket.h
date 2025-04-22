#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class SignalingSocket : public QObject {
  Q_OBJECT

 public:
  explicit SignalingSocket(QObject* parent = nullptr);

  // 启动监听作为 callee
  void startListening(quint16 port);

  // 作为 caller 连接到远端
  void connectToHost(const QString& ip, quint16 port);

  // 发送 JSON 字符串
  void sendMessage(const QString& json);

 signals:
  void messageReceived(const QString& json);
  void connected();
  void disconnected();
  void errorOccurred(const QString& error);

 private slots:
  void onNewConnection();
  void onReadyRead();
  void onSocketDisconnected();
  void onSocketError(QAbstractSocket::SocketError socketError);

 private:
  QTcpServer* server_ = nullptr;
  QTcpSocket* socket_ = nullptr;
  QByteArray buffer_;
};
