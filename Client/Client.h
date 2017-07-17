#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Client.h"
#include "chat_client.hpp"



class Client : public QMainWindow
{
	Q_OBJECT
		
public:
	Client(QWidget *parent = Q_NULLPTR);

private:
	Ui::ClientClass ui;
	io_service ioservice_;
	boost::shared_ptr<chat_client> client_;
	boost::shared_ptr<io_service::work> work_;
	chat_message msg_;
private slots:
	void soltConnectServer();
	void closeEvent(QCloseEvent *event);
	void soltSendMessage();
	void handle_packet();
};

