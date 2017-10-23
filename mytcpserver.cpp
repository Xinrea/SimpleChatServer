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
    QSqlQuery query,querydele;
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
                qDebug("Log: ready to respond");
                switch(respond(buff,clientAddr,rebuff))
                {
                case 0:{
                    errorCode = send(clientSocket,rebuff,128,0);
                    if (errorCode == SOCKET_ERROR)
                    {
                        std::cout << "Send failed!" << std::endl;
                    }
                    break;
                }
                case 1:{
                    break;
                }
                case 2:{
                    //发送离线队列
                    qDebug("Log: 准备发送离线消息队列");
                    errorCode = send(clientSocket,rebuff,128,0);
                    respondMessage* infoMsg = reinterpret_cast<respondMessage*>(rebuff);
                    if (errorCode == SOCKET_ERROR)
                    {
                        std::cout << "Send failed!" << std::endl;
                    }
                    unsigned long targetIP;
                    unsigned targetPort;
                    if(!query.exec("SELECT * FROM onlineAccount WHERE accountid="+QString::number(infoMsg->msgType-255)))qDebug()<< query.lastError();//从在线表中获得IP+PORT
                    if(query.next())
                    {
                        targetIP = query.value(2).toInt();
                        targetPort = query.value(3).toInt();
                    }
                    if(!query.exec("SELECT * FROM offlineMessage WHERE targetid="+QString::number(infoMsg->msgType-255)))qDebug()<< query.lastError();//查询离线消息，并一一发送
                    while(query.next())
                    {
                        QString messageID = QString::number(query.value(0).toInt());
                        myTcpSocket offlineSocket;
                        basicMessage sendMsg;
                        sendMsg.accountID = query.value(1).toInt();
                        sendMsg.targetID = infoMsg->msgType-255;
                        qDebug("Log: 离线消息：TO %d | %s",infoMsg->msgType-255,query.value(3).toString().toStdString().data());
                        strcpy(sendMsg.body,query.value(3).toString().toStdString().data());
                        offlineSocket.config(targetIP,targetPort);
                        offlineSocket.connectToHost();
                        offlineSocket.sendMsg(reinterpret_cast<char*>(&sendMsg));
                        offlineSocket.disconnect();
                        if(!querydele.exec("DELETE FROM offlineMessage WHERE targetid="+QString::number(infoMsg->msgType-255)+" AND messageID="+messageID))qDebug()<< query.lastError();
                        qDebug("Log: 离线消息发送");
                    }
                    break;
                }
            }
        }
        shutdown(clientSocket,2);
        }
    }
}

unsigned myTcpServer::respond(char *in,sockaddr_in addr, char *out)
{
    QSqlQuery query;
    basicMessage* inMessage = reinterpret_cast<basicMessage*>(in);
    switch(inMessage->msgType){
    case BASIC:{
        //离线消息储存
        qDebug("Log: 收到离线消息");
        basicMessage* basicMsg = reinterpret_cast<basicMessage*>(in);
        basicMessage* resMsg = reinterpret_cast<basicMessage*>(out);
        QString accountid = QString::number(basicMsg->accountID);
        QString targetid = QString::number(basicMsg->targetID);
        QString body(basicMsg->body);
        QString insertStr("INSERT INTO offlineMessage (accountid,targetid,body) VALUES ("+accountid+",\""+targetid+"\",\""+body+"\")");
        if(!query.exec(insertStr))qDebug()<< query.lastError();
        resMsg->msgType = 255;//不产生回复信息
        return 1;
    }
    case STATE:{
        stateMessage* stateMsg = reinterpret_cast<stateMessage*>(in);
        stateMessage* resMsg = reinterpret_cast<stateMessage*>(out);
        QString accountid = QString::number(stateMsg->accountID);
        QString session = QString::number(stateMsg->session);
        if(stateMsg->keepAlive)
        {
            if(!query.exec("SELECT * FROM onlineAccount WHERE accountid = "+accountid+" AND session = "+session))
            {
                qDebug()<< query.lastError();
                resMsg->keepAlive = true;//更新失败，保持原状态不变
                resMsg->session = stateMsg->session;
            }
            else if(!query.next())
            {
                //在线列表中没有这个帐号，需要重新登录，不得更新
                qDebug("Log: 帐号不在在线表中");
                resMsg->keepAlive = false;
                resMsg->session = 0;
            }
            else
            {
                //列表中找到了这个帐号
                qDebug("Log: 帐号在在线表中");
                QTime currentTime = QTime::currentTime();
                unsigned time = currentTime.hour()*3600+currentTime.minute()*60+currentTime.second();
                if(!query.exec("UPDATE onlineAccount SET lastUpdate = "+QString::number(time)+" WHERE "+"accountid = "+accountid))
                {
                    qDebug()<< query.lastError();
                    resMsg->keepAlive = true;//更新错误，保持原状态
                    resMsg->session = stateMsg->session;
                }
                else
                {
                    qDebug("Log: 更新lastUpdate");
                    resMsg->keepAlive = true;//更新状态正常
                    resMsg->session = stateMsg->session;
                }
            }
        }
        else
        {
            if(!query.exec("SELECT * FROM onlineAccount WHERE accountid = "+accountid+" AND "+"session = "+session))
            {
                qDebug()<< query.lastError();
                resMsg->keepAlive = true;//下线一定失败，因此响应中为true
            }
            else if(!query.next())
            {//没有在线
                resMsg->keepAlive = false;
            }
            else if(!query.exec("DELETE FROM onlineAccount WHERE accountid="+accountid))
            {
                qDebug()<< query.lastError();
                resMsg->keepAlive = true;//下线失败
            }
            else
                {
                qDebug("Log: 下线成功");
                resMsg->keepAlive = false;
            }
        }
        break;
    }
    case LOGIN:{
        //处理登录信息
        QTime currentTime = QTime::currentTime();
        qDebug()<<"Log: Recived Login Request";
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
            qDebug("Log: 账号在线，修改在线信息session=%d",session);
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
        //处理找回密码请求
        findpwdMessage* in_fpwdMsg = reinterpret_cast<findpwdMessage*>(in);
        basicMessage* out_resMsg = reinterpret_cast<basicMessage*>(out);
        QString accountid_s = QString::number(in_fpwdMsg->accountID);
        QString comfirminfo_s(in_fpwdMsg->comfirmInfo);
        QString password_s(in_fpwdMsg->password);
        if(!query.exec("SELECT * FROM accountTB WHERE accountid = "+accountid_s+" AND comfirminfo = "+comfirminfo_s)){
            qDebug()<< query.lastError();//服务器出错
            out_resMsg->accountID = 1;//失败
        }
        else
        {
            if(query.next())
            {
                //表中找到该项,修改密码
                qDebug("Log: 找到该项,进行密码修改");
                if(!query.exec("UPDATE accountTB SET password = "+password_s+" WHERE accountid = "+accountid_s))
                {
                    qDebug()<< query.lastError();//修改出错
                    out_resMsg->accountID = 1;
                }
                else
                {
                    qDebug("Log: 密码修改成功");
                    out_resMsg->accountID = 0;//修改成功
                    //如果被修改帐号在线,则将其下线
                    if(!query.exec("DELETE FROM onlineAccount WHERE accountid="+accountid_s))qDebug()<<query.lastError();
                }

            }
            else
            {
                //表中没有该项
                qDebug("Log: 未找到该项,无法修改密码");
                out_resMsg->accountID = 1;//无法进行修改
            }
        }

        break;
    }
    case REQUEST:{
        //处理查询信息
        requestMessage* in_reqMsg = reinterpret_cast<requestMessage*>(in);
        respondMessage* out_resMsg = reinterpret_cast<respondMessage*>(out);
        QString accountid = QString::number(in_reqMsg->accountID);
        QString targetid = QString::number(in_reqMsg->requestID);
        QString session = QString::number(in_reqMsg->session);

        if(!query.exec("SELECT * FROM onlineAccount WHERE accountid = "+accountid+" AND session = "+session))
        {
            qDebug()<< query.lastError();
            out_resMsg->msgType = 0;//查询失败
        }
        else if(query.next())
        {
            //查询到请求方的在线记录,查询请求有效
            if(!query.exec("SELECT * FROM accountTB WHERE accountid = "+targetid))
            {
                qDebug()<< query.lastError();
                out_resMsg->msgType = 0;//查询失败
            }
            else if(query.next())
            {
                //查询成功
                qDebug("Log: 得到被查询帐号用户名");
                out_resMsg->msgType = 1;
                strcpy(out_resMsg->username,query.value(2).toString().toStdString().data());
                //下面是在线帐号才有的信息
                if(!query.exec("SELECT * FROM onlineAccount WHERE accountid = "+targetid))
                {
                    qDebug()<< query.lastError();
                }
                else if(query.next())
                {
                    //在线表中查找到目标账号
                    qDebug("Log: 被查询帐号在线");
                    out_resMsg->ip = query.value(2).toInt();
                    out_resMsg->port = query.value(3).toInt();

                    if(in_reqMsg->accountID == in_reqMsg->requestID)
                    {
                        if(!query.exec("SELECT * FROM offlineMessage WHERE targetid="+accountid))qDebug()<< query.lastError();
                        if(query.next())
                        {
                            //有离线消息
                            out_resMsg->msgType = in_reqMsg->accountID+255;//离线消息标记,标记特殊处理
                            return 2;
                        }
                    }
                }
                else
                {
                    qDebug("Log: 被查询帐号不在线");
                    out_resMsg->msgType = 2;
                }

            }
            else
            {
                qDebug()<< query.lastError();
                qDebug("Log: 被查询帐号不存在");
                out_resMsg->msgType = 0;//查询失败,无此ID
            }

        }
        else
        {
            qDebug("Log: 查询帐号无效");
            out_resMsg->msgType = 0;//无权限查询
        }
        break;
    }
    }
    return 0;
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
    qDebug()<<"Create offline message table";
    if(!query.exec("CREATE TABLE offlineMessage ("
                   "messageID INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "accountid INT,"
                   "targetid INT,"
                   "body VARCHAR(112))"))qDebug()<<query.lastError();
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
