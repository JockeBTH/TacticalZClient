#pragma once
#include <asio.hpp>
#include <iostream>
#include <string>

class Client
{
public:
	Client();
	~Client();
	bool Connect(char ip[], char port[]);
	bool Receive(char* data, size_t& len, asio::ip::udp::endpoint& sender_endpoint);
	bool Send(std::string message);
	bool Send(char* data, size_t size);
	bool SendCompMessage(char* data, size_t size);
	template<typename ComponentType>
	bool Send(ComponentType comp)
	{
		char* buf = new char[comp.ByteSize()];
		comp.SerializeToArray(buf, comp.ByteSize());

		return Send(std::string(buf, comp.ByteSize()));
	}
private:
	asio::io_service io_service;
	asio::ip::udp::socket socket;
	asio::ip::udp::endpoint receiver_endpoint;
};

