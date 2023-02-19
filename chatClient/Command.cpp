#include "Utilities.h"
#include "Command.h"
#include "ClientSocketProxy.h"
#include "Message.h"
#include <iostream>
#include <regex>

namespace mc
{
    template<typename TComm>
    bool CommandBase<TComm>::Execute(ProxyBase<TComm>* pb, string& name)
    {
        if (CanExecute(pb, name))
            return InnerExecute(pb, name);
        else
            return false;
    }

    template<typename TComm>
    bool CommandBase<TComm>::CanExecute(ProxyBase<TComm>* pb, string& name)
    {
        if (name.empty())
        {
            this->error = "please use 'name' command to tell chat client your name. or, directly use 'hi' command to connect to chat server with your name.";
            return false;
        }

        if (!pb->IsLoggedOn)
        {
            this->error = "please connect to the server. sample command: HI 127.0.0.1:12345";
            return false;
        }

        return true;
    }

    template<typename TComm>
    bool HiCommand<TComm>::CanExecute(ProxyBase<TComm>* pb, string& name)
    {
        if (pb->IsLoggedOn)
        {
            this->error = "alreay logged on. please try other commands";
            return false;
        }

        return true;
    }

    template<typename TComm>
    bool NameCommand<TComm>::CanExecute(ProxyBase<TComm>* pb, string& name)
    {
        if (!name.empty())
        {
            if (pb->IsLoggedOn)
            {
                this->error = "can't change user, please logoff with 'bye' first.";
                return false;
            }
            else
            {
                return true;
            }
        }

        return true;
    }

    template<typename TComm>
    bool CommandBase<TComm>::InnerExecute(ProxyBase<TComm>* pb, string& username)
    {
        this->error = "command not implemented.";
        return false;
    }

    template<typename TComm>
    CommandBase<TComm>* CommandBase<TComm>::GetCommand(string cmd)
    {
        std::smatch m;
        // TODO: figure out the problem. (wilson -> WILSON)
        //transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
        if (cmd == "HI" || cmd == "hi" || cmd.find("HI ") == 0 || cmd.find("hi ") == 0)
        {
            // make 'command' case insensitive. (user name will be case sensitive, this is easier)
            auto ret = std::regex_match(cmd, m,
                std::regex("^HI (((([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])):([0-9]|[1-9]\\d{1,3}|[1-5]\\d{4}|6[0-4]\\d{4}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5]))$", std::regex::icase));
            if (ret)
            {
                string ip = m[2];
                UINT16 port = stoi(m[6]);

                return new HiCommand<TComm>(ip, port);
            }
            else
            {
                cout << "'hi' command parameter incorrect. should be ip + port(smaller than 65536). sample: hi 127.0.0.1:12345." << endl;
                return nullptr;
            }
        }
        else if(cmd == "NAME" || cmd == "name" || cmd.find("NAME ") == 0 || cmd.find("name ") == 0)
        {
            auto ret = std::regex_match(cmd, m, std::regex("^name ([a-zA-Z0-9]+)", std::regex::icase));
            if (ret)
            {
                string name = m[1];

                return new NameCommand<TComm>(name);
            }
            else
            {
                cout << "'name' command parameter incorrect. user name should be alphabet and numbers (without spaces)" << endl;
            }
        }
        else if (cmd.find("@") == 0)
        {
            auto ret = std::regex_match(cmd, m, std::regex("^(@([a-zA-Z0-9]+) (.+))"));
            if (ret)
            {
                string to = m[2];
                // make command case insensitive.
                transform(to.begin(), to.end(), to.begin(), ::toupper);
                // if command name and user name are the same, give command higher priority.
                // in this case, if a user name is 'all', always take it as broadcasting.
                if (to == "ALL")
                {
                    return new BroadcastCommand<TComm>(m[3]);
                }
                else
                {
                    return new AtCommand<TComm>(m[2], m[3]);
                }
            }
            else
            {
                cout << "'@' at command parameter incorrect. user name should be alphabet and numbers (without spaces). message can be any charactor. To enter multiple message, add '\\' to the end." << endl;
            }
        }
        else if (cmd == "BYE" || cmd == "bye")
        {
            // make command case insensitive.
            auto ret = std::regex_match(cmd, m, std::regex("^(BYE)", std::regex::icase));
            if (ret)
            {
                return new ByeCommand<TComm>();
            }
            else
            {
                cout << "'bye' command format is incorrect." << endl;
            }
        }
        else
        {
            cout << "command does not support." << endl;
        }

        return nullptr;
    }

    template<typename TComm>
    bool BroadcastCommand<TComm>::InnerExecute(ProxyBase<TComm>* pb, string& from)
    {
        while(msg.back() == '\\')
        {
            msg.pop_back();
            msg += "\n";
            // concat new line of messages.
            string message;
            getline(cin, message);
            msg += message;
        }

        sendMsg(pb, from);
        return true;
    }

    template<typename TComm>
    bool BroadcastCommand<TComm>::sendMsg(ProxyBase<TComm>* pb, string from)
    {
        auto res = pb->Send(
            RequestMessage(SERVER_ACTIONS_BROADCAST, unordered_map<int, string>{ {0, from }, { 1, msg }}));
        if (res.Status == ResponseStatus::OK)
        {
            cout << Utilities::GetCurrent() << " " << from << ": " << this->msg << endl;
            return true;
        }

        return false;
    }

    template<typename TComm>
    bool AtCommand<TComm>::sendMsg(ProxyBase<TComm>* pb, string from)
    {
        auto res = pb->Send(
            RequestMessage(SERVER_ACTIONS_AT, unordered_map<int, string>{ {0, from }, { 1, to }, { 2, this->msg }}));
        if (res.Status == ResponseStatus::OK)
        {
            cout << Utilities::GetCurrent() << " " << from << ": " << this->msg << endl;
            return true;
        }

        return false;
    }

    template<typename TComm>
    bool HiCommand<TComm>::createProxy(ProxyBase<TComm>* pb)
    {
        if (pb->IsConnected) return true;

        auto cres = pb->Connect(ip, port);
        if (cres < 0)
        {
            this->error = "failed connecting to server. please check if server is running at " + ip + ":" + to_string(port) + ".";
            return false;
        }

        return pb->IsConnected = true;
    }

    template<typename TComm>
    string HiCommand<TComm>::collectUsername(ProxyBase<TComm>* pb)
    {
        cout << "please input username: ";
        string name;
        getline(cin, name);
        return name;
    }

    template<typename TComm>
    bool HiCommand<TComm>::checkUserExistance(ProxyBase<TComm>* pb, string username)
    {
        ResponseMessage res = pb->Send(
            RequestMessage(SERVER_ACTIONS_CHECKUSER_REGISTS, unordered_map<int, string>{ { 0, username }}));
        if (res.Status == ResponseStatus::OK)
        {
            return res.FieldMap[0] == "true";
        }
        else
        {
            this->error = res.GetError();
            return false;
        }
    }

    template<typename TComm>
    string HiCommand<TComm>::collectPassword(ProxyBase<TComm>* pb, bool again)
    {
        // send login request just after connection is made.
        if(again)
            cout << "please confirm the password: ";
        else
            cout << "please input the password: ";
        password = Utilities::Getpassword();
        // cin.ignore();
        return password;
    }

    template<typename TComm>
    bool HiCommand<TComm>::regist(ProxyBase<TComm>* pb, string username, string password)
    {
        unordered_map<int, string> fs;
        fs.insert(pair<int, string>(0, username));
        fs.insert(pair<int, string>(1, password));
        ResponseMessage res = pb->Send(RequestMessage(SERVER_ACTIONS_REGIST, fs));
        return res.Status == ResponseStatus::OK;
    }

    template<typename TComm>
    bool HiCommand<TComm>::login(ProxyBase<TComm>* pb, string username, string password)
    {
        ResponseMessage loginRes = pb->Send(
            RequestMessage(SERVER_ACTIONS_LOGIN, unordered_map<int, string> { {0, username}, { 1, password } }));
        if (loginRes.Status == ResponseStatus::OK)
        {
            pb->IsLoggedOn = true;
            return true;
        }
        else
        {
            this->error = loginRes.GetError();
            return false;
        }
    }

    template<typename TComm>
    bool HiCommand<TComm>::InnerExecute(ProxyBase<TComm>* pb, string& name)
    {
        if (!createProxy(pb)) return false;
        
        string loggingUser = name;
        if(loggingUser.empty())
            loggingUser = collectUsername(pb);
        if (!checkUserExistance(pb, loggingUser))
        {
            cout << "User does not exist, please input password to regist." << endl;
            bool match(false);
            while (!match)
            {
                string password = collectPassword(pb);
                string password2 = collectPassword(pb, true);
                if (password == password2)
                {
                    match = true;
                    if (regist(pb, loggingUser, password) && login(pb, loggingUser, password))
                    {
                        name = loggingUser;
                    }
                }
                else
                {
                    cout << "Please make sure the passwords are the same." << endl;;
                }
            }
        }
        else
        {
            bool success = false;
            while (!success)
            {
                string password = collectPassword(pb);
                success = login(pb, loggingUser, password);
                if (!success)
                {
                    cout << this->error << "try again? (Y/N): ";
                    string in;
                    getline(cin, in);
                    if (in == "N" || in == "n")
                    {
                        this->error = "";
                        return false;
                    }
                    else if(in == "Y" || in == "y")
                    {
                        continue;
                    }
                }
                name = loggingUser;
                this->error = "";
            }
        }
    }

    template<typename TComm>
    bool ByeCommand<TComm>::logoff(ProxyBase<TComm>* pb, string from)
    {
        auto res = pb->Send(RequestMessage(SERVER_ACTIONS_LOGOFF, unordered_map<int, string> { {0, from} }));
        if (res.Status == ResponseStatus::OK)
        {
            pb->IsLoggedOn = false;
            return true;
        }
        else
        {
            this->error = res.GetError();
        }
        return false;
    }


    template<typename TComm>
    bool ByeCommand<TComm>::InnerExecute(ProxyBase<TComm>* pb, string& from)
    {
        if (logoff(pb, from))
        {
            return true;
        }

        return false;
    }

    template<typename TComm>
    bool NameCommand<TComm>::InnerExecute(ProxyBase<TComm>* pb, string& name)
    {
        name = UserName;
        return true;
    }
}