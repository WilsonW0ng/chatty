#include "Utilities.h"
#include "ClientSocketProxy.h"
#include "Command.h"
#include "Message.h"
#include "Command.cpp"

#include <stdio.h>

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <conio.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <regex>
#include <type_traits>

using namespace mc;
using namespace std;

bool isConnected{ false };
ProxyBase<SOCKET>* pb;
string user;

using namespace std;

bool putWaitIndicator()
{
    if (pb->IsLoggedOn)
        cout << pb->GetIdString() << "|";

    if (!user.empty())
        cout << user;

    // put wait indicator
    cout << " > ";
    return true;

}

void AtAction(time_t msgTime, unordered_map<int, string> fields)
{
    string from = fields[0];
    string msg = fields[1];

    cout << endl;
    cout << Utilities::GetTime(msgTime) << " " << from << ": " << msg << endl;
    putWaitIndicator();
}

void BroadcastAction(time_t msgTime, unordered_map<int, string> fields)
{
    string from = fields[0];
    string msg = fields[1];

    cout << endl;
    cout << Utilities::GetTime(msgTime) << " broadcast from " << from << ": " << msg << endl;

    putWaitIndicator();
}

typedef void (*Action)(time_t msgTime, unordered_map<int, string> fields);
unordered_map<string, Action> actionMap{
    {CLIENT_ACTIONS_BROADCAST, BroadcastAction}, {CLIENT_ACTIONS_AT, AtAction }
};

void OnReceiveMessage(PostedMessage msg)
{
    try
    {
        actionMap[msg.Action](msg.GetMsgId(), msg.FieldMap);
    }
    catch (const std::exception& ex)
    {
        cout << "error while receiving server message. " + string(ex.what()) << endl;
    }
}

int main(int argc, char* argv[])
{
    // accept the name parameter if it is provided in launch.
    if (argc > 2 && string(argv[1]) == "--name" && strlen(argv[2]) > 0)
    {
        user = argv[2];
    }

    string cmd;
    pb = new ClientSocketProxy(OnReceiveMessage);
    string outMsg;

    cout << "welcome! available commands are 'name', 'hi', 'bye', '@all', '@username'. more are comming!" << endl;

    // command interactive loop.
    while (cmd != "quit")
    {
        putWaitIndicator();
        getline(cin, cmd);
        if (cmd.empty() || cmd == "quit") continue;

        CommandBase<SOCKET>* aCmd = CommandBase<SOCKET>::GetCommand(cmd);
        if (aCmd == nullptr)
        {
            continue;
        }

        if (!aCmd->Execute(pb, user) && !aCmd->GetError().empty())
        {
            cout << aCmd->GetError() << endl;
            aCmd->ClearError();
        }

        delete aCmd;
    }

    system("pause");
    return 0;
}
