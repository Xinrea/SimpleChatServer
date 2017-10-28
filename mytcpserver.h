#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <QtSql>
#include "msgstruct.h"
#include "mytcpsocket.h"

class myTcpServer
{
private:
    WSADATA wsaData;
    sockaddr_in addr;
    SOCKET mainSocket;
    int errorCode;
    unsigned idnext;
    QSqlDatabase db;
    int listenNumber;
    bool run;
    unsigned respond(char* in,sockaddr_in addr, char* out);

public:
    myTcpServer();
    ~myTcpServer();
    void initDb();
    void showDb();
    void emptyDb();
    void stop();
    void getAccount(std::vector<QStringList> &data);
    void getOnline(std::vector<QStringList> &data);
    bool config(const int port, const int listenN);//TODO:考虑一下port的类型
private:
    void startListen();
};

#endif // MYTCPSERVER_H
