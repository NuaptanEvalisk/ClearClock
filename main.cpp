#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    // 创建 QApplication 对象，管理应用程序的生命周期
    QApplication a(argc, argv);
    // 创建我们的主窗口对象
    MainWindow w;
    // 显示窗口
    w.show();
    // 进入事件循环，等待用户操作
    return a.exec();
}
