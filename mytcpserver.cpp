#include "mytcpserver.h"
#include <QTime>
myTcpServer::myTcpServer():errorCode(0)
{
}


myTcpServer::~myTcpServer()
{
}

bool myTcpServer::config(const int port)//config socket setting, ready to connect
{
    QSqlQuery query;
    //从数据库中获取接下来注册的帐号从何处开始
    if(!query.exec("SELECT * FROM systemSet"))qDebug() << query.lastError();

    query.next();
    idnext = query.value(0).toInt();

    errorCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (errorCode != NO_ERROR) {

        return false;
    }
    mainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mainSocket == INVALID_SOCKET) {

        WSACleanup();
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = INADDR_ANY;//任意地址
    addr.sin_port = htons(port);
    return true;
}

bool myTcpServer::startListen()//connect to the host
{
    qDebug("Log: Bind Port");
    errorCode = bind(mainSocket, (sockaddr*)&addr, sizeof(addr));
    if (errorCode != 0)
    {
        std::cout << "Log: Bind failed!" << std::endl;
        return false;
    }
    qDebug("Log: Listen Port");
    errorCode = listen(mainSocket,10);
    if (errorCode != 0)
    {
        std::cout << "Log: Listen failed!" << std::endl;
        return false;
    }
    while(true)
    {
        sockaddr_in clientAddr;
        int length = sizeof(clientAddr);
        SOCKET clientSocket = accept(mainSocket, (sockaddr*)&clientAddr,&length);
        if(clientSocket == INVALID_SOCKET)
        {
            std::cout << "Accept failed" << std::endl;
        }
        else
        {
            qDebug("Log: Accept Client");
            char buff[128];
            errorCode = recv(clientSocket,buff,128,0);
            if (errorCode == SOCKET_ERROR)
            {
                std::cout << "Recv failed!" << std::endl;
            }
            else
            {
                char rebuff[128];
                if(respond(buff,clientAddr,rebuff))
                {
                    errorCode = send(clientSocket,rebuff,128,0);
                    if (errorCode == SOCKET_ERROR)
                    {
                        std::cout << "Send failed!" << std::endl;
                    }
                }
                else
                {
                    std::cout << "Construct message failed" << std::endl;
                }
            }
        }


    }
}

bool myTcpServer::respond(char *in,sockaddr_in addr, char *out)
{
    QSqlQuery query;
    basicMessage* inMessage = reinterpret_cast<basicMessage*>(in);
    switch(inMessage->msgType){
    case STATE:{
        stateMessage* stateMsg = reinterpret_cast<stateMessage*>(in);
        stateMessage* resMsg = reinterpret_cast<stateMessage*>)(out);
        QString accountid = QString::number(stateMsg->accountID);
        QString session = QString::number(stateMsg->session);
        if(stateMsg->keepAlive)
        {
            //TODO update info
        }
        else
        {
            //TODO 验证信息
            if(!query.exec("SELECT * FROM onlineAccount WHERE accountid = "+accountid+" AND "+"session = "+session))qDebug()<< query.lastError();
            if(!query.exec("DELETE FROM onlineAccount WHERE accountid="+accountid))qDebug()<< query.lastError();
            resMsg->keepAlive = false;
        }
        break;
    }
    case LOGIN:{
        //处理登录信息
        QTime currentTime = QTime::currentTime();
        qDebug()<<"Login:Recived Login Request";
        loginMessage* in_loginMsg = reinterpret_cast<loginMessage*>(in);
        basicMessage* out_resMsg = reinterpret_cast<basicMessage*>(out);
        QString accountid = QString::number(in_loginMsg->accountID);
        QString password(in_loginMsg->password);
        QString ip = QString::number(addr.sin_addr.S_un.S_addr);
        QString port = QString::number(in_loginMsg->port);
        unsigned time = currentTime.hour()*3600+currentTime.minute()*60+currentTime.second();
        unsigned session = 0;

        if(!query.exec("SELECT * FROM accountTB WHERE accountid = "+accountid+" AND "+"password = \""+password+"\"")){
            qDebug()<< query.lastError();
            out_resMsg->session = 0;
            break;
        }
        if(!query.next())
        {
            out_resMsg->session = 0;
        }
        else
        {
            //TODO:产生session

            session = time+addr.sin_addr.S_un.S_addr;
            out_resMsg->session = session;
            if(!query.exec("SELECT * FROM onlineAccount WHERE accountid="+accountid))qDebug()<< query.lastError();
            if(!query.next()){
                qDebug()<<"Log: 帐号不在线，插入在线信息";
                QString session_s = QString::number(session);
                if(!query.exec("INSERT INTO onlineAccount (accountid,session,ip,port,lastUpdate) VALUES ("+accountid+","
                               +session_s+","
                               +ip+","
                               +port+","
                               +QString::number(time)+")"))qDebug()<< query.lastError();
            }
            else
            {
            qDebug()<<"Log: 账号在线，修改在线信息";
            QString session_s = QString::number(session);
            if(!query.exec("UPDATE onlineAccount SET session="+session_s+","
                           "ip="+ip+","
                           "port="+port+","
                           "lastUpdate="+QString::number(time)+
                           " WHERE "+"accountid="+accountid))qDebug()<< query.lastError();
            }
        }
        break;
        }
    case REGISTER:{
        //处理注册信息
        registerMessage* in_regMsg = reinterpret_cast<registerMessage*>(in);
        basicMessage* out_resMsg = reinterpret_cast<basicMessage*>(out);
        QString accountid = QString::number(idnext);
        QString username_(in_regMsg->userName);
        QString password_(in_regMsg->password);
        QString comfirminfo_(in_regMsg->comfirmInfo);
        out_resMsg->msgType = BASIC;
        out_resMsg->accountID = idnext;
        qDebug() << "Register:" << accountid <<"|"<< username_ << "|" << password_ <<"|"<<comfirminfo_;
        QString insertStr("INSERT INTO accountTB (accountid,username,password,comfirminfo) VALUES ("+accountid+",\""+username_+"\",\""+password_+"\",\""+comfirminfo_+"\")");
        if(!query.exec(insertStr))qDebug()<< query.lastError();
        if(!query.exec("UPDATE systemSet SET idnext = "+QString::number(idnext+1)))qDebug()<< query.lastError();
        idnext++;
        break;
        }
    case FINDPWD:{
        //
        break;
    }
    case REQUEST:{
        //
        break;
    }
    }
    return true;
}

void myTcpServer::initDb()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("accountInfo.db");
    db.open();
    QSqlQuery query;
    qDebug()<<"Create accountTB";
    if(!query.exec("CREATE TABLE accountTB ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "accountid INT,"
               "username VARCHAR(8),"
               "password VARCHAR(12),"
               "comfirminfo VARCHAR(12))"))qDebug()<<query.lastError();
    qDebug()<<"Create systemSet";
    if(!query.exec("CREATE TABLE systemSet ("
               "idnext INT)"))qDebug()<<query.lastError();
    qDebug()<<"Create onlineAccount";
    if(!query.exec("CREATE TABLE onlineAccount ("
                   "accountid INT,"
                   "session INT,"
                   "ip INT,"
                   "port INT,"
                   "lastUpdate INT)"))qDebug()<<query.lastError();
    else if(!query.exec("INSERT INTO systemSet VALUES (1)"))qDebug()<<query.lastError();
}

void myTcpServer::emptyDb()
{
    QSqlQuery query;
    if(!query.exec("DELETE * FROM accountTB"))qDebug()<<query.lastError();
    if(!query.exec("DELETE * FROM systemSet"))qDebug()<<query.lastError();
    if(!query.exec("DELETE * FROM onlineAccount"))qDebug()<<query.lastError();
}

void myTcpServer::showDb()
{
    QSqlQuery query;
    qDebug("Online Account Info");
    if(!query.exec("SELECT * FROM onlineAccount"))qDebug()<<query.lastError();
    while(query.next())
    {
        qDebug("AccountID:%d|session:%d|ip:%d",query.value(0).toInt(),query.value(1).toInt(),query.value(2).toInt());
    }
    if(!query.exec("SELECT * FROM systemSet"))qDebug()<<query.lastError();
    while(query.next())
    {
        qDebug("帐号起始分配位置IDNEXT=%d",query.value("idnext").toInt());
    }
}
