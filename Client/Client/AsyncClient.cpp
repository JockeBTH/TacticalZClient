#include "AsyncClient.h"



AsyncClient::AsyncClient(boost::asio::io_service& io_service) : socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 13))
{
	Init("192.168.1.6", 13, io_service);
}

AsyncClient::AsyncClient(boost::asio::io_service& io_service, char ip[] , int port) : socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port))
{
	Init(ip, port, io_service);
}

AsyncClient::~AsyncClient()
{						   
}						

bool AsyncClient::Init(char ip[], int port, boost::asio::io_service& io_service)
{
	boost::system::error_code errCode;
	boost::asio::ip::udp::resolver resolver(io_service);
	boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), ip, std::to_string(port));
	receiver_endpoint = *resolver.resolve(query, errCode);
	if (errCode.value() != 0)
	{
		std::cerr << errCode.message() << std::endl;
		return false;
	}
	socket.open(boost::asio::ip::udp::v4(), errCode);
	if (errCode.value() != 0)
		std::cerr << errCode.message() << std::endl;
	return !errCode;

}
						   
void AsyncClient::Start()  
{
	socket.async_receive_from(
		boost::asio::buffer(receiveBuffer), receiver_endpoint,
		boost::bind(&AsyncClient::HandleReceive, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void AsyncClient::HandleReceive(const boost::system::error_code & error, std::size_t bytes)
{
	if (!error || error == boost::asio::error::message_size)
	{
		boost::shared_ptr<std::string> message(new std::string("hej"));

		socket.async_send_to(boost::asio::buffer(*message), receiver_endpoint,
			boost::bind(&AsyncClient::HandleSend, this, message,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void AsyncClient::HandleSend(boost::shared_ptr<std::string> message, const boost::system::error_code &, std::size_t)
{
	std::cout << "HandleSend. Message sent: " << message->data() << "\n";
}

bool AsyncClient::Send(std::string message)
{
	boost::system::error_code errCode;
	socket.send_to(boost::asio::buffer(message), receiver_endpoint, 0, errCode);
	if (errCode.value() != 0)
		std::cerr << errCode.message() << std::endl;
	return !errCode;
}