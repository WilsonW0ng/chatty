#include <stdio.h>

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <windows.h>
#include <ws2tcpip.h>

#include <vector>
#include <iostream>
#include <fstream>
#include "Message.h"
#include "Handler.h"
#include "ServerSocketProxy.h"

#include <unordered_map>

using namespace std;
using namespace ms;

unordered_map<string, Handler<SOCKET>> clientMap;
ResponseMessage OnReceiveMessage(SOCKET socket, RequestMessage res);
ProxyBase<SOCKET>* pb = new ServerSocketProxy("127.0.0.1", 12345, OnReceiveMessage);

void InitClientMap()
{
    fstream f;
    string path = ".\\account.data";
    f.open(path, ios::in);
    string u;
    string p;

    while (f >> u >> p)
    {
        clientMap.insert(
            pair<string, Handler<SOCKET>>(u, Handler<SOCKET>(u, p)));
    }

    f.close();
}

void registUser(SOCKET socket, string username, string password)
{
    if (clientMap.find(username) == clientMap.end())
    {
        fstream f;
        string path = ".\\account.data";
        f.open(path, ios::out | ios::app);
        f << username << " " << password << endl;
        f.close();

        clientMap.insert(pair<string, Handler<SOCKET>>(username, Handler<SOCKET>(username, password)));
    }
}

bool checkUserRegists(string username)
{
    return clientMap.find(username) != clientMap.end();
}

void LoginAction(SOCKET socket, RequestMessage& msg, ResponseMessage& resMsg)
{

    string username = msg.FieldMap[0];
    string password = msg.FieldMap[1];
    if (username.empty() || password.empty())
        throw invalid_argument("username or password is empty.");

    // verify user & password.
    if (clientMap[username].Password != password)
        throw exception("incorrect password.");

    // TODO: should schedule a thread and run after this response returns to MC.
    // Use the same thread for alive check?
    vector<MessageBase*> postingMsgs = clientMap[username].Logon(pb, socket);
    resMsg.PostingMsgs = postingMsgs;
    cout << "User:" << username << " logged on. Handler:" << clientMap[username].GetId() << endl;
}

void performLogoff(string name)
{
    clientMap[name].Logoff();
    cout << "User:" << name << " logged off." << endl;
}

unordered_map<int, string>* LogoffAction(SOCKET socket, unordered_map<int, string> fields)
{
    string name = fields[0];
    if (name.empty()) 
        throw invalid_argument("name is empty.");

    performLogoff(name);

    return nullptr;
};

unordered_map<int, string>* ClientClosedAction(SOCKET socket, unordered_map<int, string> fields)
{
    // if use is force close the MC. then needs to logoff user bofore setting disconnect.
    for (auto m : clientMap)
    {
        // in this case, we have only socket information.
        if (m.second.GetId() == socket)
        {
            if (m.second.GetLogonStatus())
            {
                performLogoff(m.second.Name);
            }

            m.second.Disconnect();
            cout << socket << " disconnected." << endl;
            break;
        }
    }

    return nullptr;
};

unordered_map<int, string>* BroadcastAction(SOCKET socket, unordered_map<int, string> fields)
{
    string from = fields[0];
    string msg = fields[1];
    string names;
    // broadcast, forward to all MCs except itself.
    for (auto iter : clientMap)
    {
        if (iter.second.Name == from)
            continue;

        iter.second.Send(pb,
            new PostedMessage(CLIENT_ACTIONS_BROADCAST, unordered_map<int, string>{ {0, from}, { 1, msg }}));
        
        names += iter.second.Name + " ";
    }
    cout << "Broadcasting message from: " << from << " to: [" << names << "] message: " << msg << endl;

    return nullptr;
}

unordered_map<int, string>* AtAction(SOCKET socket, unordered_map<int, string> fields)
{
    string from = fields[0];
    string to = fields[1];
    string msg = fields[2];

    clientMap[to].Send(pb, 
        new PostedMessage(CLIENT_ACTIONS_AT, unordered_map<int, string>{ {0, from}, { 1, msg }}));
    
    cout << "Sending message from: " << from << " to: " << to << " message: " << msg << endl;

    return nullptr;
}

unordered_map<int, string>* CheckUserRegistsAction(SOCKET socket, unordered_map<int, string> fields) 
{
    unordered_map<int, string>* resFields = new unordered_map<int, string>();
    if (checkUserRegists(fields[0]))
    {
        resFields->insert(pair<int, string>(0, "true"));
    }
    else
    {
        resFields->insert(pair<int, string>(0, "false"));
    }

    return resFields;
}

unordered_map<int, string>* RegistAction(SOCKET socket, unordered_map<int, string> fields)
{
    string username = fields[0];
    string password = fields[1];
    if (username.empty() || password.empty()) return nullptr;

    if (!checkUserRegists(username))
    {
        registUser(socket, username, password);
    }
    return nullptr;
}

typedef unordered_map<int, string>* (*Action)(SOCKET socket, unordered_map<int, string> fields);
unordered_map<string, Action> actionMap{
    {SERVER_ACTIONS_REGIST, RegistAction},
    {SERVER_ACTIONS_LOGOFF, LogoffAction}, {SERVER_ACTIONS_BROADCAST, BroadcastAction},
    {SERVER_ACTIONS_CHECKUSER_REGISTS,CheckUserRegistsAction }, {SERVER_ACTIONS_AT, AtAction },
    {SERVER_ACTIONS_CLIENTCLOSED, ClientClosedAction}
};

ResponseMessage OnReceiveMessage(SOCKET socket, RequestMessage msg)
{
    try
    {
        // special treat for the login action, for making sure the offline message send only after the login responses to MC.
        if (msg.Action == SERVER_ACTIONS_LOGIN)
        {
            ResponseMessage res(msg.GetMsgId());
            LoginAction(socket, msg, res);
            return res;
        }

        unordered_map<int, string>* res = actionMap[msg.Action](socket, msg.FieldMap);
        if(res == nullptr)
            return ResponseMessage(msg.GetMsgId());
        else
            return ResponseMessage(msg.GetMsgId(), *res);
    }
    catch (const std::exception& ex)
    {
        return ResponseMessage(msg.GetMsgId(), ex.what(), ResponseStatus::NOK);
    }
};

void SayGoodbye()
{
    for (auto iter : clientMap)
    {
        iter.second.Send(pb,
            new PostedMessage(CLIENT_ACTIONS_BROADCAST, unordered_map<int, string>{ {0, "server"}, { 1, "server is closing." }}));
    }
}

int main()
{
    cout << "server started at 127.0.0.1:12345" << endl;
    InitClientMap();

    while (true)
    {
        // wait for connection.
        cout << pb->Accept() << " connected." << endl;
    }

    SayGoodbye();

    system("pause");
    return 0;
}