//#include "Client.h"
#include "AsyncClient.h"
#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <Windows.h>
#include <boost/timer/timer.hpp>
#include <ctime>

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

enum class MsgType
{
	Connect,
	ClientPing,
	ServerPing,
	Message,
	Snapshot,
	Disconnect,
	Event

};

struct PlayerPositions
{
	int x;
	int y;
};

bool IsOwnMessage(string_ptr);
void DisplayLoop(socket_ptr); //Reads from the message queue and displays
void ReadFromServer(socket_ptr, string_ptr); //Read message from server and push to message queue
void WriteLoop(socket_ptr, string_ptr); //Reads input from user and writes to server
string* BuildPrompt();
int Receive(socket_ptr sock, char* data, size_t len);
int CreateEventMessage(MsgType msgType, string message, char* dataLocation);
void MoveMsgHead(char*& data, size_t& len, size_t stepSize);

void ParseEventMessage(char* data, size_t len);
void ParseConnect(char* data, size_t len);
void ParsePing();
void ParseServerPing(socket_ptr sock);

const int boardSize = 16;
char gameBoard[boardSize][boardSize];
PlayerPositions playerPositions[8];

std::clock_t startPingTime;
double durationOfPingTime;

udp::endpoint receiver_endpoint(address::from_string("192.168.1.6"), 13);
messageQueue_ptr messageQueue(new queue<string_ptr>);
const int inputSize = 128;
string_ptr promptCpy;
boost::asio::io_service ioService;

int PlayerID = -1;

int main(int argc, char argv[])
{
	boost::thread_group threads;
	socket_ptr sock(new udp::socket(ioService));
	
	for (size_t i = 0; i < boardSize; i++)
	{
		for (size_t j = 0; j < boardSize; j++)
		{
			gameBoard[j][i] = ' ';
		}
	}
	

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

void ParseMsgType(char* data, size_t len, socket_ptr sock)
{
	int messageType = -1;

	memcpy(&messageType, data, sizeof(int));
	MoveMsgHead(data, len, sizeof(int));


	switch (static_cast<MsgType>(messageType)) {
	case MsgType::Connect:
		ParseConnect(data, len);
		break;
	case MsgType::ClientPing:
		ParsePing();
		break;
	case MsgType::ServerPing:
		ParseServerPing(sock);
		break;
	case MsgType::Message:
		break;
	case MsgType::Snapshot:
		break;
	case MsgType::Disconnect:
		break;
	case MsgType::Event:
		ParseEventMessage(data, len);
		break;
	default:
		break;
	}

}


void WriteLoop(socket_ptr sock, string_ptr prompt)
{
	char* testMsg = new char[128];
	int amountMessagesSent = 0;
	int testOffset = 0;
	bool previousKey = false;
	std::clock_t previousePingMsgTime = 1000 * std::clock() / static_cast<double>(CLOCKS_PER_SEC);
	std::clock_t previousCommandTime = 1000 * std::clock() / static_cast<double>(CLOCKS_PER_SEC);
	int intervallMs = 33; // ~30 times per second
	int commandInterval = 200; // for commands like ping and connect, name might be ambigiuos

	for (;;)
	{

		std::clock_t currentTime = 1000 * std::clock() / static_cast<double>(CLOCKS_PER_SEC);
		if (commandInterval < currentTime - previousCommandTime)
		{
			if (!previousKey &&  GetAsyncKeyState('P'))
			{
				int testOffset = CreateEventMessage(MsgType::ClientPing, "Ping", testMsg);
				++amountMessagesSent;
				cout << "Messages sent: " << amountMessagesSent << endl;

				startPingTime = std::clock();
				sock->send_to(boost::asio::buffer(
					testMsg,
					testOffset),
					receiver_endpoint, 0);
			}

			if (GetAsyncKeyState('C'))
			{
				int testOffset = CreateEventMessage(MsgType::Connect, "tja", testMsg);
				++amountMessagesSent;
				cout << "Messages sent: " << amountMessagesSent << endl;

				startPingTime = std::clock();
				sock->send_to(boost::asio::buffer(
					testMsg,
					testOffset),
					receiver_endpoint, 0);
			}

			if (GetAsyncKeyState('Q')) // Does not work. Plez fix
				exit(1);
			memset(testMsg, 0, inputSize);
			previousKey = GetAsyncKeyState('P');
			previousCommandTime = currentTime;
		}
		if (intervallMs < currentTime - previousePingMsgTime)
		{
			if (PlayerID != -1)
			{
				if (GetAsyncKeyState('W')) {
					int len = CreateEventMessage(MsgType::Event, "+Forward", testMsg);			
					sock->send_to(boost::asio::buffer(
						testMsg,
						len),
						receiver_endpoint, 0);
				}
				if (GetAsyncKeyState('A')) {
					int len = CreateEventMessage(MsgType::Event, "-Right", testMsg);
					sock->send_to(boost::asio::buffer(
						testMsg,
						len),
						receiver_endpoint, 0);
				}
				if (GetAsyncKeyState('S')) {
					int len = CreateEventMessage(MsgType::Event, "-Forward", testMsg);
					sock->send_to(boost::asio::buffer(
						testMsg,
						len),
						receiver_endpoint, 0);
				}
				if (GetAsyncKeyState('D')) {
					int len = CreateEventMessage(MsgType::Event, "+Right", testMsg);
					sock->send_to(boost::asio::buffer(
						testMsg,
						len),
						receiver_endpoint, 0);
				}
			}
			previousePingMsgTime = currentTime;
		}
	}
}


void DisplayLoop(socket_ptr)
{
	for (;;)
	{
		if (!messageQueue->empty())
		{
			if (!IsOwnMessage(messageQueue->front())) {
				cout << *(messageQueue->front()) << endl;
			}
			messageQueue->pop();
		}
	}
}

void ReadFromServer(socket_ptr sock, string_ptr prompt)
{
	int bytesRead = 128;
	char readBuf[1024] = { 0 };

	for (;;)
	{
		if (sock->available())
		{
			bytesRead = Receive(sock, readBuf, inputSize);
			string_ptr msg(new string(readBuf, bytesRead));
			ParseMsgType(readBuf, bytesRead, sock);

			//messageQueue->push(msg);
		}
	}
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

void MoveMsgHead(char*& data, size_t& len, size_t stepSize)
{
	data += stepSize;
	len -= stepSize;
}

void ParseConnect(char* data, size_t len)
{
	memcpy(&PlayerID, data, sizeof(int));
	cout << "I am player: " << PlayerID << endl;
}

void ParsePing()
{
	durationOfPingTime = 1000 * (std::clock() - startPingTime) / static_cast<double>(CLOCKS_PER_SEC);
	cout << "response time with ctime(ms): " << durationOfPingTime << endl;
}

void ParseServerPing(socket_ptr sock)
{
	char* testMsg = new char[128];
	int testOffset = CreateEventMessage(MsgType::ServerPing, "Ping recieved", testMsg);

	cout << "Parsing ping." << endl;

	sock->send_to(boost::asio::buffer(
		testMsg,
		testOffset),
		receiver_endpoint, 0);
}

void ParseEventMessage(char* data, size_t len)
{
	cout << "Event message: " << string(data) << endl;
	MoveMsgHead(data, len, string(data).size() + 1);
}





int CreateEventMessage(MsgType msgType, string message, char* dataLocation)
{
	int lengthOfMsg = 0;
	int iMsgType = static_cast<int>(msgType);
	lengthOfMsg = message.size();

	int offset = 0;
	// Message type
	memcpy(dataLocation + offset, &iMsgType, sizeof(int));
	offset += sizeof(int);
	// Message, add one extra byte for null terminator
	memcpy(dataLocation + offset, message.data(), (lengthOfMsg + 1) * sizeof(char));
	offset += (lengthOfMsg + 1) * sizeof(char);

	return offset;
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