#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
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
    unsigned respond(char* in,sockaddr_in addr, char* out);

public:
    myTcpServer();
    ~myTcpServer();
    void initDb();
    void showDb();
    void emptyDb();
    bool config(const int port);//TODO:考虑一下port的类型
    bool startListen();
};

#endif // MYTCPSERVER_H
