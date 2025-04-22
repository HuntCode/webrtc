#include "QtTestDriver.h"
#include <QDebug>

QtTestDriver::QtTestDriver(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    role_ = "";

    connect(ui.pushButton_connect, &QPushButton::clicked, this, &QtTestDriver::onConnectClicked);
}

QtTestDriver::~QtTestDriver()
{}

void QtTestDriver::onConnectClicked() {
  QString ip = ui.lineEdit_ip->text().trimmed();
  QString port = ui.lineEdit_port->text().trimmed();

  if (ip.isEmpty() || port.isEmpty()) {
    qDebug() << u8"请输入 IP 和端口";
    return;
  }

  // 当前作为 Caller
  role_ = "caller";

  qDebug() << u8"启动连接...";
  qDebug() << "角色：" << role_;
  qDebug() << "目标地址：" << ip << ":" << port;

  // TODO: 初始化 WebRTC PeerConnection，并尝试连接到指定 IP:Port
}