#include "myTcpSocket.h"

myTcpSocket::myTcpSocket():errorCode(0)
{
}


myTcpSocket::~myTcpSocket()
{
}

bool myTcpSocket::config(WCHAR* ip, const int port)//config socket setting, ready to connect
{
    errorCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (errorCode != NO_ERROR) {

        return false;
    }
    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(connectSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int));
    if (connectSocket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
    char temp[12];
    WideCharToMultiByte(CP_ACP,0,ip,-1,temp,12,NULL,NULL);
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = inet_addr(temp);
    addr.sin_port = htons(port);
   qDebug("Ready to connect: %s  %d\n",temp,port);
    return true;
}

bool myTcpSocket::config(unsigned long ip, const int port)//config socket setting, ready to connect
{
    errorCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (errorCode != NO_ERROR) {

        return false;
    }
    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(connectSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int));
    if (connectSocket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = ip;
    addr.sin_port = htons(port);
   qDebug("Ready to connect");
    return true;
}

bool myTcpSocket::connectToHost()//connect to the host
{
    qDebug("Start to connect\n");
    errorCode = connect(connectSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    sockaddr tempaddr;
    int tempLen = sizeof(tempaddr);
    getsockname(connectSocket,&tempaddr,&tempLen);
    temp = ((sockaddr_in*)&tempaddr)->sin_port;
    if (errorCode == SOCKET_ERROR) {
        qDebug("Connect Error\n");
        errorCode = closesocket(connectSocket);
        if (errorCode == SOCKET_ERROR) {

            WSACleanup();
        }
        return false;
    }
    return true;
}

unsigned myTcpSocket::tempPort()
{
    return temp;
}

bool myTcpSocket::sendMsg(char* message, int length)//send message to the host
{
    qDebug("Start to sendmsg\n");
    errorCode = send(connectSocket, message,length, 0);
    if (errorCode == SOCKET_ERROR)return false;
    return true;
}

bool myTcpSocket::recvMsg(char* message, int length)
{
    qDebug("Start to recvmsg\n");
    errorCode = recv(connectSocket, message,length, MSG_WAITALL);
    if(errorCode == SOCKET_ERROR || GetLastError() == WSAENETRESET)
    {
        return false;
    }
    return true;
}


bool myTcpSocket::disconnect()//disconnet for destorying or next config
{
    qDebug("Start to disconnect\n");
    errorCode = closesocket(connectSocket);
    if (errorCode == SOCKET_ERROR) {

        return false;
    }
    WSACleanup();
    return true;
}
