#include "Client.h"
using asio::ip::udp;

Client::Client():io_service(),socket(io_service)
{

}


Client::~Client()
{
}

bool Client::Connect(char ip[], char port[])
{
	asio::error_code errCode;
	udp::resolver resolver(io_service);
	udp::resolver::query query(udp::v4(), ip, port);
	receiver_endpoint = *resolver.resolve(query, errCode);
	if (errCode.value() != 0)
	{
		std::cerr << errCode.message() << std::endl;
		return false;
	}
	socket.open(udp::v4(), errCode);
	if (errCode.value() != 0)
		std::cerr << errCode.message() << std::endl;
	return !errCode;
}

bool Client::Send(char* data, size_t size)
{
	asio::error_code errCode;
	socket.send_to(asio::buffer(data, size), receiver_endpoint, 0, errCode);
	if (errCode.value() != 0)
		std::cerr << errCode.message() << std::endl;
	return !errCode;
}

bool Client::SendCompMessage(char * data, size_t size)
{
	asio::error_code errCode;
	socket.send_to(asio::buffer(data, size), receiver_endpoint, 0, errCode);
	if (errCode.value() != 0)
		std::cerr << errCode.message() << std::endl;
	return !errCode;
}

bool Client::Send(std::string message)
{
	asio::error_code errCode;
	socket.send_to(asio::buffer(message), receiver_endpoint, 0, errCode);
	if (errCode.value() != 0)
		std::cerr << errCode.message() << std::endl;
	return !errCode;
}

bool Client::Receive(char* data, size_t& len, udp::endpoint& sender_endpoint)
{
	asio::error_code errCode;
	len = socket.receive_from(asio::buffer((void*)data, len), sender_endpoint, 0, errCode);
	if (errCode.value() != 0)
		std::cerr << errCode.message() << std::endl;
	return !errCode;
}

