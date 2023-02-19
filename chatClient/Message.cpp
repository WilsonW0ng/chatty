#include "Message.h"
#include <regex>

namespace mc
{
    Message Message::FromString(string message)
    {
        char messageType;
        string from;
        string to;
        string msg;
        string username;
        string password;
        MessageType mt;
        std::smatch m;
        // format: "OptType+Uuser+[/Ppassword] +[Tto] +Mmessage"
        //auto ret = std::regex_match(message, m, std::regex("^([@,*]{1})(U(\\w+))? M(.+)?"));
        auto ret = std::regex_match(message, m, std::regex("^([*,@,!,+,-]{1})U(\\w+)(/P(\\w+))? ?(T(\\w+) )?(M(.+))?"));

        if (ret)
        {
            //messageType = m[1].str()[0];
            //from = m[3];
            //msg = m[4];
           messageType = m[1].str()[0];
            from = m[2];
            username = m[2];
            password = m[4];
            to = m[6];
            msg = m[8];
        }
        switch (messageType)
        {
        case '+':
            mt = MessageType::LogIn;
            break;
        case '!':
            mt = MessageType::Heartbeat;
            break;
        case '*':
            mt = MessageType::Broadcast;
            break;
        case '@':
            mt = MessageType::At;
            break;
        case '-':
            mt = MessageType::LogOff;
            break;
        }

        return Message(mt, from, to, password, msg);
    }

    string Message::ToString(Message message)
    {
        string type;
        string userinfo;
        string addr;
        string msgContent;

        switch (message.MsgType)
        {
        case MessageType::None:
            break;
        case MessageType::LogIn:
            type = "+";
            break;
        case MessageType::Heartbeat:
            type = "^";
            break;
        case MessageType::Broadcast:
            type = "*";
            break;
        case MessageType::At:
            type = "@";
            break;
        case MessageType::LogOff:
            type = "-";
            break;
        case MessageType::CloseServer:
            break;
        }

        if (!message.From.empty())
        {
            userinfo = "U" + message.From;
            if (!message.Password.empty())
            {
                userinfo += "/P" + message.Password;
            }
            userinfo += " ";
        }

        if (!message.To.empty())
        {
            addr = "T" + message.To + " ";
        }

        if (!message.Msg.empty())
        {
            msgContent += "M" + message.Msg;
        }

        return type + userinfo + addr + msgContent;
    }

    Message Message::CreateHeartbeatMessage(string username)
    {
        Message msg(MessageType::Heartbeat, username);
        string content = ToString(msg);
        msg.Msg = content;
        return msg;
    }

    Message Message::CreateLoginMessage(string username, string password)
    {
        // we cannot specify the default parameter :(
        Message msg(MessageType::LogIn, username, "", password, "");
        string content = ToString(msg);
        msg.Msg = content;
        return msg;
    }

    Message Message::CreateAtMessage(string from, string to, string msgContent)
    {
        Message msg(MessageType::At, from, to, "", msgContent);
        string content = ToString(msg);
        msg.Msg = content;
        return msg;
    }

    Message Message::CreateBroadcastMessage(string from, string msgContent)
    {
        Message msg(MessageType::Broadcast, from, "", "", msgContent);
        string content = ToString(msg);
        msg.Msg = content;
        return msg;
    }

    Message Message::CreateLogoffMessage(string from)
    {
        Message msg(MessageType::LogOff, from);
        string content = ToString(msg);
        msg.Msg = content;
        return msg;
    }
}