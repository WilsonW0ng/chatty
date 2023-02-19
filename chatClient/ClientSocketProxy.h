#pragma once
#ifndef MC_COMMPROTOCOL
#define MC_COMMPROTOCOL
#include "Message.h"

#include <WinSock2.h>
#include <string>
#include <vector>
using namespace std;
namespace mc
{
	template<typename CommT>
	class ProxyBase 
	{
	public:
		CommT ComService;
		void (*OnReceive)(PostedMessage msg);
		// indicate the proxy has initialized. for example, the client socket is initialized.
		bool IsInitialized;
		// indicate the proxy has connected to the target server. for example, the client socket is connected to the server port.
		bool IsConnected;
		// indicate the client has successfully logged on the server.
		bool IsLoggedOn;
		virtual int Connect(string ip, UINT16 port) = 0;
		virtual ResponseMessage Send(RequestMessage msg) = 0;
		virtual void Post(PostedMessage msg) = 0;
		virtual string GetIdString() = 0;
	};

	class ClientSocketProxy : public ProxyBase<SOCKET>
	{
		friend ostream& operator<<(ostream& out, ClientSocketProxy& socketProxy);
	private:
		string ip;
		UINT16 port;
		void initSocket();
	public:
		ClientSocketProxy(void (*onReceive)(PostedMessage msg))
		{
			OnReceive = onReceive;
			initSocket();
		}
		~ClientSocketProxy();

		int Connect(string ip, UINT16 port);
		ResponseMessage Send(RequestMessage msg);
		void Post(PostedMessage msg);
		SOCKET GetId();
		string GetIdString();
	};
}
#endif