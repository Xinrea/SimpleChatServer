#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    server = new myTcpServer;

    onlinemodel = new QStandardItemModel;
    accountmodel = new QStandardItemModel;
    onlinemodel->setColumnCount(5);
    onlinemodel->setHeaderData(0,Qt::Horizontal,QString("Account"));
    onlinemodel->setHeaderData(1,Qt::Horizontal,QString("Session"));
    onlinemodel->setHeaderData(2,Qt::Horizontal,QString("Ip"));
    onlinemodel->setHeaderData(3,Qt::Horizontal,QString("Port"));
    onlinemodel->setHeaderData(4,Qt::Horizontal,QString("LastUpdate"));
    ui->tableView->setModel(onlinemodel);
    ui->tableView->setColumnWidth(0,81);
    ui->tableView->setColumnWidth(1,100);
    ui->tableView->setColumnWidth(2,100);
    ui->tableView->setColumnWidth(3,100);
    ui->tableView->setColumnWidth(4,100);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    accountmodel->setColumnCount(3);
    accountmodel->setHeaderData(0,Qt::Horizontal,QString("ID"));
    accountmodel->setHeaderData(1,Qt::Horizontal,QString("Account"));
    accountmodel->setHeaderData(2,Qt::Horizontal,QString("Username"));
    ui->tableView_2->setModel(accountmodel);
    ui->tableView_2->setColumnWidth(0,61);
    ui->tableView_2->setColumnWidth(1,200);
    ui->tableView_2->setColumnWidth(2,220);
    ui->tableView_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

MainWindow::~MainWindow()
{
    delete ui;
    if(server != nullptr)delete server;
}

void MainWindow::on_startButton_clicked()
{
    if(server == nullptr)server = new myTcpServer;
    server->config(ui->spinBox->value(),ui->spinBox_2->value());
    ui->spinBox->setEnabled(false);
    ui->spinBox_2->setEnabled(false);
    ui->stateLabel->setText("运行中");
}

void MainWindow::on_shutdownButton_clicked()
{
    if(server != nullptr)
    {
        server->stop();
        delete server;
        server = nullptr;
        ui->stateLabel->setText("关闭");
        ui->spinBox->setEnabled(true);
        ui->spinBox_2->setEnabled(true);
    }
}

void MainWindow::on_DBinitButton_clicked()
{
    if(server != nullptr){
        server->initDb();
        server->emptyDb();
    }
    else
    {
        QMessageBox::critical(this,"Error","服务器未启动");
    }

}

void MainWindow::refreshAccout()
{
    std::vector<QStringList> data;
    server->getAccount(data);
    int j = 0;
    for(QStringList i:data)
    {
        for(int k = 0;k<3;k++)
        {
            accountmodel->setItem(j,k,new QStandardItem(i[k]));
            accountmodel->item(j,k)->setTextAlignment(Qt::AlignCenter);
        }
        ++j;
    }
}

void MainWindow::refreshOnline()
{
    std::vector<QStringList> data;
    server->getOnline(data);
    int j = 0;
    for(QStringList i:data)
    {
        for(int k = 0;k<5;k++)
        {
            onlinemodel->setItem(j,k,new QStandardItem(i[k]));
            onlinemodel->item(j,k)->setTextAlignment(Qt::AlignCenter);
        }
        ++j;
    }
}

void MainWindow::on_refreshButton_2_clicked()
{
    accountmodel->removeRows(0, accountmodel->rowCount());
    refreshAccout();
}

void MainWindow::on_refreshButton_clicked()
{
    onlinemodel->removeRows(0, onlinemodel->rowCount());
    refreshOnline();
}
