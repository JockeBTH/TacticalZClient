#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#define MESSAGESIZE 128

class AsyncClient
{
public:
	AsyncClient(boost::asio::io_service& io_service);
	AsyncClient(boost::asio::io_service& io_service, char ip[], int port);
	~AsyncClient();
	void Start();
	bool Send(std::string message);

private:
	void HandleReceive(const boost::system::error_code& error, std::size_t bytes);
	void HandleSend(boost::shared_ptr<std::string> message /*message*/,
		const boost::system::error_code& /*error*/,
		std::size_t /*bytes_transferred*/);
	bool Init(char ip[], int port, boost::asio::io_service& io_service);

	boost::asio::ip::udp::socket socket;
	boost::asio::ip::udp::endpoint receiver_endpoint;
	boost::array<char, MESSAGESIZE> receiveBuffer;
};

