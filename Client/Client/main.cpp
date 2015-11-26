//#include "Client.h"
#include "AsyncClient.h"
#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lockfree/queue.hpp>

using namespace boost::asio::ip;
using namespace std;

typedef boost::shared_ptr<udp::socket> socket_ptr;
typedef boost::shared_ptr<std::string> string_ptr;
typedef boost::shared_ptr<queue<string_ptr> > messageQueue_ptr;

struct header
{
	int sizeInBytes;
	std::string componentType;
	char* data;
};

struct test
{
	int banan;
	float temperatur;
};

enum class Messages
{
	Ping,
	Snapshot,
	Connect
};

bool IsOwnMessage(string_ptr);
void DisplayLoop(socket_ptr); //Reads from the message queue and displays
void ReadFromServer(socket_ptr, string_ptr); //Read message from server and push to message queue
void WriteLoop(socket_ptr, string_ptr); //Reads input from user and writes to server
string* BuildPrompt();
bool Receive(udp::socket sock, char* data, size_t len);

udp::endpoint receiver_endpoint(address::from_string("192.168.1.6"), 13);
messageQueue_ptr messageQueue(new queue<string_ptr>);
const int inputSize = 128;
string_ptr promptCpy;
boost::asio::io_service ioService;


int main(int argc, char argv[])
{
	boost::thread_group threads;
	socket_ptr sock(new udp::socket(ioService));

	string_ptr prompt( BuildPrompt() );
	promptCpy = prompt;

	sock->connect(receiver_endpoint);
	cout << "I am client. BIP BOP\n";

	threads.create_thread(boost::bind(DisplayLoop, sock));
	threads.create_thread(boost::bind(ReadFromServer, sock, prompt));
	threads.create_thread(boost::bind(WriteLoop, sock, prompt));

	threads.join_all();

	return 0;
}

string* BuildPrompt()
{
	char inputBuf[inputSize] = { 0 };
	char nameBuf[inputSize] = { 0 };
	string* prompt = new string(": ");

	cout << "Enter username: ";
	cin.getline(nameBuf, inputSize);
	*prompt = (string)nameBuf + *prompt;
	boost::algorithm::to_lower(*prompt);

	return prompt;
}


int Receive(socket_ptr sock, char* data, size_t len)
{
	len = sock->receive_from(boost
		::asio::buffer((void*)data, len), 
		receiver_endpoint, 
		0);
	return len;
}

void ReadFromServer(socket_ptr sock, string_ptr prompt)
{
	int bytesRead = 128;
	char readBuf[1024] = { 0 };

	for (;;)
	{
		if (sock->available())
		{
			bytesRead = Receive(sock, readBuf, bytesRead);
			string_ptr msg(new string(readBuf, bytesRead));

			messageQueue->push(msg);
		}
	}
}

void WriteLoop(socket_ptr sock, string_ptr prompt)
{
	char inputBuf[inputSize] = { 0 };
	string inputMsg;

	for (;;)
	{
		cin.getline(inputBuf, inputSize);
		inputMsg = /**prompt + */ (string)inputBuf/* + '\n'*/;

		if (!inputMsg.empty())
		{
			sock->send_to(boost::asio::buffer(inputMsg), receiver_endpoint, 0);
		}

		if (inputMsg.find("exit") != string::npos)
			exit(1);
		inputMsg.clear();
		memset(inputBuf, 0, inputSize);
	}
}

void DisplayLoop(socket_ptr)
{
	for (;;)
	{
		if (!messageQueue->empty())	{
			if (!IsOwnMessage(messageQueue->front())) {
				cout << *(messageQueue->front()) << endl;
			}
			messageQueue->pop();
		}
	}
}

bool IsOwnMessage(string_ptr message)
{
	if (message->find(*promptCpy) != string::npos) {
		return true;
	}
	else {
		return false;
	}
}



//int main(int argc, char argv[])
//{
//	boost::asio::io_service ioService;
//	AsyncClient client(ioService,"192.168.1.6" , 13);
//	std::string message;
//	while (message != "q")
//	{
//		std::cin >> message;
//		client.Send(message);
//		client.Start();
//		
//	}
//	return 0;
//}

//int main(int argc, char argv[])
//{
//	std::cout << "I be client." << std::endl;
//	Client client;
//	bool connected = false;
//	char* connectionData = new char[sizeof(int)];
//	size_t lengthOfmessage = 0;
//	std::string ping = "Ping Message";
//	/*while (connected)
//	{*/
//		client.Connect("192.168.1.6", "13");
//		//client.Send(ping);
//		//client.Receive();
//	//}
//
//	// Message received from cmd
//	std::string message;
//
//	while (message != "q" && message != "Q")
//	{
//
//		///////////////////
//		/////// Test Code
//		///////////////////
//		//size_t dataSize;
//		//// Read input
//		std::cin >> message;
//		////// Read size of input and sett size variable
//		////dataSize = message.size();
//
//		////std::cout << message << "\n";
//
//		//// Allocate the memory. +1 for null terminator
//		//char* data = new char[dataSize + 1];
//		//strcpy(data, message.c_str());
//		//std::cout << " Test data: " << (std::string)data << "\n";
//		//std::cout << "Message size: " << message.size() << "\n";
//		int messagetype = 0;
//		
//		try
//		{
//			messagetype = std::stoi(message);
//		}
//		catch (const std::exception&)
//		{
//
//			messagetype = -1;
//		}
//
//
//		std::string componentName = "TestComponent1";
//		int nameLength = componentName.size();
//		int banan = 8;
//		float temperature = 7.55f;
//
//		unsigned char * lengthP = reinterpret_cast<unsigned char*>(&nameLength);
//		unsigned char * nameP = reinterpret_cast<unsigned char*>(&nameLength);
//		unsigned char * bananP = reinterpret_cast<unsigned char*>(&nameLength);
//		unsigned char * temperatureP = reinterpret_cast<unsigned char*>(&nameLength);
//		/////////
//		// Pack
//		///////////
//		char* testPackage = new char[sizeof(int) + sizeof(int) +
//			nameLength + sizeof(int) + sizeof(float)];
//		
//		// Also size of data that we are sending
//		int offset = 0;
//		// MessageType int for now
//		memcpy(testPackage + offset, &messagetype, sizeof(int));
//		offset += sizeof(int);
//		// Length of string
//		memcpy(testPackage + offset, &nameLength, sizeof(int));
//		offset += sizeof(int);
//		// component name data (string)
//		memcpy(testPackage + offset, componentName.data(), nameLength);
//		offset += nameLength;
//		// int test data 
//		memcpy(testPackage + offset, &banan, sizeof(int));
//		offset += sizeof(int);
//		// float test data
//		memcpy(testPackage + offset, &temperature, sizeof(float));
//		offset += sizeof(float);
//		/////////////
//		// Parse
//		////////////
//		int messageTypeParsed = 0;
//		int nameLengthParsed = 0;
//		int bananParsed = 0;
//		float temperatureParsed = 0.0f;
//		int dataOffset = 0; // used to read the data from the byte array
//		
//
//		// Get length of string
//		memcpy(&messageTypeParsed, testPackage + dataOffset, sizeof(int));
//		dataOffset += sizeof(int);
//
//		// Get length of string
//		memcpy(&nameLengthParsed, testPackage + dataOffset, sizeof(int));
//		dataOffset += sizeof(int);
//		
//		// String as char array
//		char* temp = new char[nameLengthParsed + 1];
//		temp[nameLengthParsed] = '\0';
//		memcpy(temp, testPackage + dataOffset, nameLengthParsed);
//		dataOffset += nameLengthParsed;
//		
//		// Parse component data
//		memcpy(&bananParsed, testPackage + dataOffset, sizeof(int));
//		dataOffset += sizeof(int);
//		// Parse component data
//		memcpy(&temperatureParsed, testPackage + dataOffset, sizeof(float));
//		dataOffset += sizeof(float);
//
//
//		std::cout << "\n";
//
//		std::cout << "Message type: " << messageTypeParsed << +"\n";
//		std::cout << "Name length: " << nameLengthParsed << + "\n";
//		std::cout << "Name: " << temp << + "\n";
//		std::cout << "Int Nr: " << bananParsed << + "\n";
//		std::cout << "Float Nr: " << temperatureParsed << + "\n";
//
//		client.Send(message);
//		if (messagetype == 2)
//		{
//			std::cout << "Connect to server";
//			udp::endpoint remote_endpoint_Temp;
//			size_t len = 128;
//			char recv_buf[128]; // size of len
//			client.Receive(recv_buf, len, remote_endpoint_Temp);
//
//			int userId = -1;
//			// Parse connection message
//			memcpy(&userId, recv_buf, sizeof(int));
//			std::cout << "User id: " << userId << "\n";
//		}
//
//		/*	
//		if (data != NULL)
//		{
//			delete[] data;
//			data = NULL;
//		}
//		*/
//
//	}
//	
//	return 0;
//}