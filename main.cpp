#include <QCoreApplication>
#include <iostream>
#include "mytcpserver.h"
using namespace std;
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    cout << "Server Start" << endl;
    myTcpServer server;
    server.initDb();
    server.showDb();
    server.config(12000);
    server.startListen();
    return a.exec();
}
