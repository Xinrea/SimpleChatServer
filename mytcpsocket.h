#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <QDebug>

#include "msgstruct.h"
class myTcpSocket
{
private:
    WSADATA wsaData;
    sockaddr_in addr;
    SOCKET connectSocket;
    int errorCode;
    int timeout = 3000;
    unsigned temp;
public:
    myTcpSocket();
    ~myTcpSocket();
    bool config(WCHAR* ip, const int port);//TODO:考虑一下port的类型
    bool config(unsigned long ip,const int port);
    bool connectToHost();
    unsigned tempPort();
    bool sendMsg(char* message, int length = DATALEN);
    bool recvMsg(char* message, int length = DATALEN);//阻塞型，直至接收完毕
    bool disconnect();
};

#endif // MYTCPSOCKET_H
