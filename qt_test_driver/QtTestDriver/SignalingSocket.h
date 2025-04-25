#ifndef SIGNALING_SOCKET_H
#define SIGNALING_SOCKET_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class SignalingSocket : public QObject {
  Q_OBJECT

 public:
  explicit SignalingSocket(QObject* parent = nullptr);


  void startListening(quint16 port);
  void connectToHost(const QString& ip, quint16 port);
  void sendMessage(const QString& json);
  bool isConnected() const;

signals:
  void connected();
  void disconnected();
  void messageReceived(const QString& json);
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

#endif  // SIGNALING_SOCKET_H