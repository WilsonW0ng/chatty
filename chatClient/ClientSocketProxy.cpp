#include "Utilities.h"
#include "Message.h"
#include "ClientSocketProxy.h"

#include <ws2tcpip.h>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <regex>

namespace mc
{
    std::mutex mtx;
    std::condition_variable cv;
    unordered_map<unsigned long long, ResponseMessage*> waitingItems;

    DWORD WINAPI ThreadRoutine(LPVOID lpThreadParameter)
    {
        const int bufferSize = 0x400;
        CHAR Buffer[bufferSize] = { 0 };
        string receivedData;

        ClientSocketProxy* arg = static_cast<ClientSocketProxy*>(lpThreadParameter);
        auto onReceive = arg->OnReceive;
        int i;
        while ((i = recv(arg->ComService, Buffer, 0x400, 0)) > 0)
        {
            receivedData += string(Buffer, i);

            string toProcess;
            while (!receivedData.empty())
            {
                toProcess = "";
                int pos = receivedData.find_first_of("@");
                int size = stoi(receivedData.substr(0, pos));
                int l = receivedData.substr(pos + 1).length();
                if (size > l)
                {
                    // not completely received.
                    //continue;
                    break;
                }
                else if (size == l)
                {
                        // completely received.
                        toProcess = receivedData.substr(pos + 1, size);
                        receivedData = "";
                }
                else
                {
                    toProcess = receivedData.substr(pos + 1, size);
                    // received additional data(from the next message).
                    receivedData = receivedData.substr(size + 1 + to_string(size).length());
                }

                if (!toProcess.empty())
                {
                    std::unique_lock<std::mutex> lck(mtx);
                    MessageBase* msg = MessageBase::FromString(toProcess);
                    if (typeid(*msg) == typeid(ResponseMessage))
                    {
                        // if msg is 'relay' message. deliver it to the waiting list.
                        if (waitingItems.find(((ResponseMessage*)msg)->GetRelayMsgId()) != waitingItems.end())
                        {
                            waitingItems[((ResponseMessage*)msg)->GetRelayMsgId()] = (ResponseMessage*)msg;
                        }
                        else
                        {
                            // shouldn't be here.
                        }
                        cv.notify_one();
                    }
                    else
                    {
                        // it must be a post message, because MC would never receive RequestMessage from server.
                        onReceive(*(PostedMessage*)msg);
                    }
                }
            }
        }

        return 0;
    }

    ResponseMessage ClientSocketProxy::Send(RequestMessage msg)
	{
        std::unique_lock<std::mutex> lck(mtx);
        string content = msg.ToString();
        send(ComService, content.c_str(), content.length(), 0);
        // add a wish item.
        waitingItems.insert(pair<unsigned long long, ResponseMessage*>(msg.GetMsgId(), nullptr));

        // wait until received the responnse.
        cv.wait(lck);
        // get the reply message.
        if (waitingItems[msg.GetMsgId()] != nullptr)
        {
            ResponseMessage relayingMsg = *waitingItems[msg.GetMsgId()];
            delete waitingItems[msg.GetMsgId()];
            waitingItems.erase(msg.GetMsgId());
            return relayingMsg;
        }
        else
        {
            //TODO: implement wait timeout.
            return ResponseMessage(msg.GetMsgId(), "No response.");
        }
	}

    void ClientSocketProxy::Post(PostedMessage msg)
    {
        string content = msg.ToString();
        send(ComService, content.c_str(), content.length(), 0);
    }

    string ClientSocketProxy::GetIdString()
    {
        return this->ip + ":" + to_string(this->port);
    }

    int ClientSocketProxy::Connect(string ip, UINT16 port)
    {
        sockaddr_in ServerAddr = { 0 };
        ServerAddr.sin_port = port;
        ServerAddr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &ServerAddr.sin_addr.S_un);
        auto res = connect(ComService, (SOCKADDR*)&ServerAddr, sizeof(sockaddr_in));

        if (res >= 0)
        {
            this->ip = ip;
            this->port = port;
            CreateThread(NULL, NULL, ThreadRoutine, this, NULL, NULL);
        }
        return res;
    }

    void ClientSocketProxy::initSocket()
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
        ComService = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (ComService == INVALID_SOCKET)
        {
            Utilities::QuitWithError("Failed to create socket.");
        }
    }

    ClientSocketProxy::~ClientSocketProxy()
    {
        shutdown(ComService, SD_BOTH);
        this->IsConnected = false;

        closesocket(ComService);
        this->IsInitialized = false;

        WSACleanup();
    }

    ostream& operator<<(ostream& out, ClientSocketProxy& socketProxy)
    {
        out << socketProxy.ip << to_string(socketProxy.port);
        return out;
    }
}