#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QStandardItemModel>
#include "mytcpserver.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_startButton_clicked();

    void on_shutdownButton_clicked();

    void on_DBinitButton_clicked();

    void on_refreshButton_2_clicked();

    void on_refreshButton_clicked();

private:
    Ui::MainWindow *ui;
    QStandardItemModel  *onlinemodel;
    QStandardItemModel  *accountmodel;
    myTcpServer* server = nullptr;

    void refreshAccout();
    void refreshOnline();
};

#endif // MAINWINDOW_H
