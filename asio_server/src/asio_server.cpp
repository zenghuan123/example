#include "asio_server.h"
#include <iostream>
std::string asio_server::gen_id()
{
	clock_t t = clock_t();
	return std::to_string(t);
}

asio_server::TcpClient::TcpClient(asio::io_context& io_context, std::string ip, int port):MessageHandle(io_context)
{
	tcp::resolver resolver(io_context);
	end_point_ = resolver.resolve(ip, std::to_string(port));

}

void asio_server::TcpClient::write_async(const std::string& message)
{
	Message msg;
	
	msg.body_length(message.size());
	memcpy(msg.body(), message.c_str(), msg.body_length());
	msg.encode_header();
	asio::post(std::bind(&asio_server::TcpClient::write, shared_from_this(), msg));
}


void asio_server::TcpClient::close()
{

};


void asio_server::TcpClient::connect()
{
	asio::post(io_context_,std::bind(&asio_server::TcpClient::do_connect,this));
	
};
void asio_server::TcpClient::do_connect()
{
	asio::async_connect(this->socket_,this->end_point_, std::bind(&asio_server::TcpClient::handle_connect,this,std::placeholders::_1));
}

void asio_server::TcpClient::handle_connect(const asio::error_code& error)
{
	if(!error)
	{
		asio::async_read(socket_, asio::buffer(read_msg_.data(),Message::header_length),std::bind(&asio_server::TcpClient::read_head,this,std::placeholders::_1));

	}else{
		std::cout<<"connect fail"<<std::endl;
	}
}

//---------------------------------------

asio_server::MessageHandle::MessageHandle(asio::io_context& io_context):io_context_(io_context),socket_(io_context_)
{

}
asio_server::MessageHandle::~MessageHandle()
{
	socket_.close();
}
void asio_server::MessageHandle::write(const Message& message)
{
	bool need_process = write_msg_queue_.empty();
	write_msg_queue_.push(message);
	if (need_process)
	{
		Message m = write_msg_queue_.front();
		asio::async_write(socket_, asio::buffer(m.data(), m.length()), std::bind(&asio_server::MessageHandle::handle_write, this, std::placeholders::_1));
	}
}
void asio_server::MessageHandle::handle_write(const asio::error_code&error)
{
	if (!error)
	{
		write_msg_queue_.pop();
		if (!write_msg_queue_.empty())
		{
			Message m = write_msg_queue_.front();
			asio::async_write(socket_, asio::buffer(m.data(), m.length()), std::bind(&asio_server::MessageHandle::handle_write, this, std::placeholders::_1));
		}
	}
	else {
		std::cout << "write fail" << std::endl;
	}
}

void asio_server::MessageHandle::read_head(const asio::error_code&error)
{
	if (!error)
	{
		read_msg_.decode_header();
		asio::async_read(socket_, asio::buffer(read_msg_.body(), read_msg_.body_length()), std::bind(&asio_server::MessageHandle::read_body, this, std::placeholders::_1));
	}
	else {
		std::cout << "read head fail" << std::endl;
	}
}
void asio_server::MessageHandle::read_body(const asio::error_code& error)
{
	if (!error)
	{
		auto ptr = read_msg_.body();
		std::string  message = std::string(ptr, ptr + read_msg_.body_length());
		this->handle_read(message);
	}
}

void asio_server::MessageHandle::handle_read(const std::string&message)
{
	std::cout << "client receive" << message << std::endl;
}