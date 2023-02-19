#include "ServerSocketProxy.h"
#include <Utilities.h>
#include <Message.h>

#include <ws2tcpip.h>
#include <iostream>

namespace ms
{
    struct Arg
    {
        HANDLE ioHanlde;
        ResponseMessage (*OnReceive)(SOCKET socket, RequestMessage&& msg);
    };

    DWORD WINAPI ThreadRoutine(LPVOID lpThreadParameter)
    {
        DWORD RealOper = 0;

        ULONG_PTR CompletKey = 0;

        MyOverLapped* pOverLapped = nullptr;

        ServerSocketProxy* sp = static_cast<ServerSocketProxy*>(lpThreadParameter);
        HANDLE IoHandle = sp->ioHanlde;
        auto onReceive = sp->OnReceive;

        while (TRUE)
        {
            bool res = GetQueuedCompletionStatus(
                IoHandle, &RealOper, &CompletKey, (LPOVERLAPPED*)&pOverLapped, INFINITE);

            if (res == FALSE && RealOper == 0)
            {
                closesocket((SOCKET)CompletKey);

                onReceive(CompletKey, 
                    RequestMessage(SERVER_ACTIONS_CLIENTCLOSED, unordered_map<int, string>()));
            }
            else
            {
                string str(pOverLapped->WsaBuf.buf);
                str = str.substr(str.find("@")+ 1);
                RequestMessage* msg = (RequestMessage*)MessageBase::FromString(str);
                ResponseMessage response = onReceive(CompletKey, move(*msg));
                // send the response back to MC.
                sp->Send(CompletKey, (MessageBase*)&response);
                // send the posting message within the response.
                for (auto m : response.PostingMsgs)
                {
                    sp->Send(CompletKey, m);
                }
                delete msg;
            }


            delete[] pOverLapped->WsaBuf.buf;
            delete pOverLapped;

            // TODO: If a message is larger than 1024 bytes, the message will be truncate.
            MyOverLapped* OverLapped = new MyOverLapped{ 0 };
            OverLapped->WsaBuf.len = 0x400;
            OverLapped->WsaBuf.buf = new char[0x400]{ 0 };

            DWORD RealRecv = 0, Flags = 0;
            WSARecv((SOCKET)CompletKey, &OverLapped->WsaBuf, 1, &RealRecv, &Flags, (OVERLAPPED*)OverLapped, NULL);
        }

        return 0;
    }

    void ServerSocketProxy::init()
    {
        // Initialize windows asynchronous socket.
        WSAData WsaData = { 0 };
        int res = WSAStartup(MAKEWORD(2, 2), &WsaData);
        if (res != 0)
        {
            switch (res)
            {
            case WSASYSNOTREADY:
                Utilities::QuitWithError("Network subsystem is unavailable.");
                break;
            case WSAVERNOTSUPPORTED:
                Utilities::QuitWithError("Windows sockets implementation out of date, please check if Winsock.dll is correctly installed.");
                break;
            case WSAEINPROGRESS:
                Utilities::QuitWithError("A blocking operation is currently executing, please restart and try.");
                break;
            case WSAEPROCLIM:
                Utilities::QuitWithError("Too many processes, please close unnecessary application and try.");
                break;
            default:
                Utilities::QuitWithError("Failed to initialize. Error code is " + res);
                break;
            }
        }

        // create socket
        innerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (innerSocket == INVALID_SOCKET)
        {
            Utilities::QuitWithError("Failed to create socket.");
        }

        // Create IOCP kernal object
        ioHanlde = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

        // Get System information which contains number of CPUs.
        SYSTEM_INFO SystemInfo = { 0 };
        GetSystemInfo(&SystemInfo);

        // Create the thread based on number of CPUs.
        for (int i = 0; i < SystemInfo.dwNumberOfProcessors; ++i)
        {
            CreateThread(NULL, NULL, ThreadRoutine, this, NULL, NULL);
        }

        sockaddr_in ServerAddr = { 0 };
        ServerAddr.sin_port = port;
        ServerAddr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &ServerAddr.sin_addr.S_un);
        BOOL Result = bind(innerSocket, (SOCKADDR*)&ServerAddr, sizeof(sockaddr_in));
        if (Result == SOCKET_ERROR)
        {
            Utilities::QuitWithError("Failed to bind the socket.");
        }

        Result = listen(innerSocket, SOMAXCONN);
    }

    SOCKET ServerSocketProxy::Accept()
    {
        int dwSize = sizeof(sockaddr_in);
        sockaddr_in ClientAddr = { 0 };
        SOCKET ClientSocket = accept(innerSocket, (SOCKADDR*)&ClientAddr, &dwSize);

        CreateIoCompletionPort((HANDLE)ClientSocket, ioHanlde, (ULONG_PTR)ClientSocket, 0);

        MyOverLapped* OverLapped = new MyOverLapped{ 0 };
        OverLapped->WsaBuf.len = 0x400;
        OverLapped->WsaBuf.buf = new char[0x400]{ 0 };

        DWORD RealRecv = 0, Flags = 0;
        WSARecv(ClientSocket, &OverLapped->WsaBuf, 1, &RealRecv, &Flags, (OVERLAPPED*)OverLapped, NULL);

        return ClientSocket;
    }

    void ServerSocketProxy::Send(SOCKET id, MessageBase* msg)
    {
        string content = msg->ToString();

        send(id, (char*)content.c_str(), content.length(), 0);
    }

    ServerSocketProxy::ServerSocketProxy(string ip, UINT16 port, ResponseMessage (*onReceive)(SOCKET socket, RequestMessage res))
    {
        OnReceive = onReceive;
        this->ip = ip;
        this->port = port;
        init();
    }

    ServerSocketProxy::~ServerSocketProxy()
    {
        closesocket(innerSocket);

        WSACleanup();
    }
}