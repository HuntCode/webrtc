#include "QtTestDriver.h"
#include <QtWidgets/QApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

void createConsole() {
#ifdef Q_OS_WIN
  AllocConsole();                   // 创建控制台窗口
  freopen("CONOUT$", "w", stdout);  // 绑定标准输出到控制台
  freopen("CONOUT$", "w", stderr);
  freopen("CONIN$", "r", stdin);

  qDebug() << u8"控制台已创建，qDebug 可以打印了！";
#endif
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    createConsole();  // 创建控制台窗口

    QtTestDriver w;
    w.show();
    return a.exec();
}
