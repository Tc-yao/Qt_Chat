#ifdef WIN32
#define _WIN32_WINNT 0x0501
#endif // WIN32

#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "../Include/chat_message.h"
#include <set>
#include <deque>
#include "Server.h"
#include <QMessageBox>
using namespace boost::asio;
using namespace boost::system;
using boost::asio::ip::tcp;

class chat_participant
{
public:
	virtual ~chat_participant() {}

	virtual void deliver(const chat_message& msg) = 0;
};

typedef std::deque<chat_message> chat_message_deque;

class chat_room
{
private:
	std::set<boost::shared_ptr<chat_participant> > participants_;//存储用户的容器
	enum { max_recent_msgs = 100 };
	chat_message_deque recent_msg_;//历史聊天记录
public:
	void join_room(boost::shared_ptr<chat_participant> participant)
	{
		//std::cout << "someone join in!" << std::endl;
		participants_.insert(participant);
		std::for_each(recent_msg_.begin(), recent_msg_.end(),
			boost::bind(&chat_participant::deliver, participant, _1));//对象调用方法，参数是_1
	}

	void leave(boost::shared_ptr<chat_participant> participant)
	{
		/*std::cout << "oneguy leave!" << std::endl;*/
		participants_.erase(participant);
	}
	void deliver(const chat_message& msg)   //派发函数
	{
		recent_msg_.push_back(msg);//把消息压到消息队列

		while (recent_msg_.size() > max_recent_msgs)
		{
			recent_msg_.pop_front();    //把最早的消息扔了
		}
		std::for_each(participants_.begin(), participants_.end(),
			boost::bind(&chat_participant::deliver, _1, boost::ref(msg))//_1是for_each里的迭代器participant
		);

	}
};

class chat_session : public chat_participant,//有纯虚函数
	public boost::enable_shared_from_this<chat_session>
{
private:
	tcp::socket socket_;
	chat_message read_msg_;
	chat_room& room_;
	chat_message_deque write_msg_;
public:
	chat_session(io_service& ioservice, chat_room& room)
		:socket_(ioservice), room_(room)
	{

	}

	~chat_session()
	{

	}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		room_.join_room(shared_from_this());
		boost::asio::async_read(socket_,
			boost::asio::buffer(read_msg_.data(), chat_message::header_length_),
			bind(&chat_session::read_handle_header, shared_from_this(), _1)
		);
	}

	void read_handle_header(const error_code& error)
	{
		if (!error && read_msg_.decode_header())
		{
			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
				bind(&chat_session::read_handle_body, shared_from_this(), _1)
			);
		}
		else
		{
			//踢出去
			room_.leave(shared_from_this());
		}
	}

	void read_handle_body(const error_code& error)
	{
		if (!error)
		{
// 			std::string str(read_msg_.body(), read_msg_.body_length());
// 			std::cout << "用户:" << socket().remote_endpoint().address()
// 				<< "  " << str
// 				<< std::endl;
			room_.deliver(read_msg_);
			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.data(), chat_message::header_length_),
				bind(&chat_session::read_handle_header, shared_from_this(), _1)
			);
		}
		else
		{
			room_.leave(shared_from_this());
		}
	}
	virtual void deliver(const chat_message& msg)
	{
		bool write_in_progress = !write_msg_.empty();
		write_msg_.push_back(msg);//把要发送的内容全压到某个对象服务队列容器中
		if (!write_in_progress)
		{
			boost::asio::async_write(socket_, buffer(write_msg_.front().data(), write_msg_.front().length()),
				boost::bind(&chat_session::write_handle, shared_from_this(), _1));
		}
	}
	void write_handle(const error_code& error)
	{
		if (!error)
		{
			write_msg_.pop_front();
			if (!write_msg_.empty())
			{
				boost::asio::async_write(socket_,
					buffer(write_msg_.front().data(), write_msg_.front().length()),
					boost::bind(&chat_session::write_handle, shared_from_this(), _1));
			}
		}
	}
};

class chat_server
{
private:
	io_service& io_service_;
	tcp::acceptor acceptor_;
	chat_room room_;          //生成聊天室
public:
	chat_server(io_service& ioservice, tcp::endpoint endpoint)
		:io_service_(ioservice),
		acceptor_(ioservice, endpoint)
	{
		start_accept();
	}

	void start_accept()
	{
		boost::shared_ptr<chat_session> new_session(boost::make_shared<chat_session>(io_service_, room_));//生成新会话

		acceptor_.async_accept(new_session->socket(), boost::bind(&chat_server::accept_handle, this, new_session, _1));

	}

	void accept_handle(boost::shared_ptr<chat_session> session, const error_code& error)
	{
		if (!error)
		{
// 			std::cout << "IP地址:" << session->socket().remote_endpoint().address()
// 				<< " 端口号:" << session->socket().remote_endpoint().port() << std::endl;

			session->start();
		}

		start_accept();
	}
};

Server::Server(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	//ui.lineEdit->setText(QString::fromLocal8Bit("nihao"));
	ui.lineEdit->setText("6666");
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(slotStartServer()));
	connect(ui.pushButton_2, SIGNAL(clicked()), this, SLOT(slotStopServer()));
}

void Server::slotStartServer()
{
// 	QMessageBox::critical (NULL, QString::fromLocal8Bit("标题"), 
// 		 ui.lineEdit->text(),
// 		 QMessageBox::Ok);
//  	
// 	ui.lineEdit->setText("hello,Tc");
	std::thread t(
		[&]()
	{
		io_service ioservice;

		tcp::endpoint endpoint(tcp::v4(), ui.lineEdit->text().toInt());
		
		chat_server server(ioservice, endpoint);

		ioservice.run();
	}
	);
	t.detach();
}

void Server::slotStopServer()
{

}