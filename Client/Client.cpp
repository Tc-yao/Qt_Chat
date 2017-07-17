#include "Client.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>

Client::Client(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	ui.lineEdit->setText("192.168.0.168");
	ui.lineEdit_2->setText("8888");
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(soltConnectServer()));
	connect(ui.pushButton_2, SIGNAL(clicked()), this, SLOT(soltSendMessage()));
}

void Client::closeEvent(QCloseEvent *event)
{
	qDebug() << QString::fromLocal8Bit("Debug:准备关闭");
	QMessageBox::StandardButton button;
	button = QMessageBox::question(this, QString::fromLocal8Bit("退出程序"),
		QString::fromLocal8Bit("是否结束操作退出？"),
		QMessageBox::Yes | QMessageBox::No);
	if (button == QMessageBox::No)
	{
		event->ignore();
	}
	else if (button == QMessageBox::Yes)
	{
		if (client_.get() != nullptr)
		{
			client_->close();
			ioservice_.stop();
		}
		event->accept();
	}
	
}

void Client::soltConnectServer()
{
	tcp::resolver resolver(ioservice_);  //装配了一个解析器
	tcp::resolver::query query(ui.lineEdit->text().toStdString().c_str(), 
		ui.lineEdit_2->text().toStdString().c_str());  //多个IP（DNS）
	tcp::resolver::iterator iterator = resolver.resolve(query);

	client_ = boost::make_shared<chat_client>(ioservice_, iterator);

	work_ = boost::make_shared<io_service::work> (ioservice_);  //没有工作io_service也不会停止工作

	QObject::connect(client_.get(), SIGNAL(received_packet()), this, SLOT(handle_packet()));

	boost::shared_ptr<boost::thread> t(
		boost::make_shared<boost::thread>(
			boost::bind(&boost::asio::io_service::run, &ioservice_)));

	t->detach();
}

void Client::soltSendMessage()
{
	if (client_.get() != nullptr)
	{
		QByteArray buf = ui.textEdit_2->toPlainText().toLocal8Bit();

		msg_.set_body_length(buf.size());
		memcpy(msg_.body(), buf.data(), buf.size());

		msg_.encode_header();
		client_->send(msg_);
	}
	
}
void Client::handle_packet()
{
// 	QMessageBox::critical(NULL, QString::fromLocal8Bit("biaoti"), ui.lineEdit_2->text(),
// 		QMessageBox::Ok);
	chat_message msg = client_->get_message();
	char szText[chat_message::max_body_length_ + 1] = { 0 };
	memcpy(szText, msg.body(), msg.body_length());
	ui.textEdit->append(QString::fromLocal8Bit(szText));

}