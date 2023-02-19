#pragma once
#ifndef MS_COMMPROTOCOL
#define MS_COMMPROTOCOL
#include <Message.h>

#include <WinSock2.h>
#include <string>
#include <iostream>
using namespace std;

namespace ms
{
	struct MyOverLapped
	{
		OVERLAPPED Overlapped;

		WSABUF WsaBuf;
	};

	template<typename CommT>
	class ProxyBase 
	{
	public:
		virtual CommT Accept() = 0;
		virtual void Send(CommT id, MessageBase* msg) = 0;
	};

	class ServerSocketProxy: public ProxyBase<SOCKET>
	{
	private:
		string ip;
		UINT16 port;
		SOCKET innerSocket;
		void init();
	public:
		HANDLE ioHanlde;
		ResponseMessage (*OnReceive)(SOCKET socket, RequestMessage msg);
		ServerSocketProxy(string ip, UINT16 port, ResponseMessage (*onReceive)(SOCKET socket, RequestMessage msg));
		~ServerSocketProxy();

		SOCKET Accept();
		virtual void Send(SOCKET id, MessageBase* msg);
	};
}
#endif