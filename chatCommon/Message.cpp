#include "Message.h"
#include "pch.h"
#include <regex>
#include <sstream>

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";


static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(const char* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];  // store 3 byte of bytes_to_encode
    unsigned char char_array_4[4];  // store encoded character to 4 bytes

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);  // get three bytes (24 bits)
        if (i == 3) {
            // eg. we have 3 bytes as ( 0100 1101, 0110 0001, 0110 1110) --> (010011, 010110, 000101, 101110)
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2; // get first 6 bits of first byte,
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4); // get last 2 bits of first byte and first 4 bit of second byte
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6); // get last 4 bits of second byte and first 2 bits of third byte
            char_array_4[3] = char_array_3[2] & 0x3f; // get last 6 bits of third byte

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';

    }

    return ret;

}

std::string base64_decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}

MessageBase* MessageBase::FromString(string message)
{
    char messageType;
    string msgId;
    string relayMsgId;
    ResponseStatus status;
    string action;
    string content;

    string body = base64_decode(message);
    istringstream iss(body);
    string str;
    int i=0;
    while (getline(iss, str))
    {
        if (i == 0)
        {
            messageType = str[0];
        }
        if (i == 1)
        {
            if (str.length() == 10)
                msgId = str;
            if (str.length() == 20)
            {
                msgId = str.substr(0, 10);
                relayMsgId = str.substr(10, 10);
            }
        }
        if (i == 2)
        {
            if (str[0] == 'S')
            {
                status = stoi(str.substr(1)) == (int)ResponseStatus::OK ? ResponseStatus::OK : ResponseStatus::NOK;
            }
            if (str[0] == 'A')
            {
                action = str.substr(1);
            }
        }
        if (i == 3)
        {
            // fields
            content = str;
        }
        if (i > 3)
            content += ('\n' + str);
        i++;
    }

    MessageBase* msg(nullptr);
    switch (messageType)
    {
    case '&':
        msg = new RequestMessage(stoull(msgId), action, content);
        break;
    case '%':
        msg = new ResponseMessage(stoull(msgId), stoull(relayMsgId), status, content);
        break;
    default:
        Utilities::QuitWithError("Un-expected message received. Message: " + message);
    }
    return msg;
}

void RequestMessage::ResolveFieldMap(string fieldStr)
{
    std::regex fv("F(\\d+):V([^`]*)`");
    auto begin = std::sregex_iterator(fieldStr.begin(), fieldStr.end(), fv);
    auto end = std::sregex_iterator();

    for (std::sregex_iterator i = begin; i != end; ++i) {
        std::smatch match = *i;
        std::string match_str = match[0];
        FieldMap.insert({ stoi(match[1]), match[2]});
    }
}

void ResponseMessage::ResolveFieldMap(string fieldStr)
{
    std::smatch m;
    auto ret = std::regex_match(fieldStr, m, std::regex("^(F\\d+:V[^`]*`)*"));

    if (!ret) return;

    string fields = m[1];

    std::regex fv("F(\\d+):V([^`]*)`");
    auto begin = std::sregex_iterator(fields.begin(), fields.end(), fv);
    auto end = std::sregex_iterator();

    for (std::sregex_iterator i = begin; i != end; ++i) {
        std::smatch match = *i;
        std::string match_str = match[0];
        FieldMap.insert({ stoi(match[1]), match[2] });
    }
}

/// <summary>
/// TODO: the special charactor '`' is a known issue as sperator when chat message contains it.
/// </summary>
/// <returns></returns>
string RequestMessage::ToString()
{
    string type = "&";
    string msgId = to_string(GetMsgId());
    string action = Action;
    string msgContent;
    for (auto f : FieldMap)
    {
        msgContent += ("F" + to_string(f.first) + ":" + "V" + f.second + "`");
    }

    string body = type + "\n" + msgId + "\n" + "A" + action + "\n" + msgContent;
    body = base64_encode(body.c_str(), body.length());

    int size = (body).size();
    return to_string(size)+ "@" + body;
}

string ResponseMessage::ToString()
{
    string type = "%";
    string msgId = to_string(GetMsgId());
    string relayMsgId = to_string(GetRelayMsgId());
    string status = Status == ResponseStatus::OK ? "0" : "1";
    string msgContent;
    for (auto f : FieldMap)
    {
        msgContent += ("F" + to_string(f.first) + ":" + "V" + f.second + "`");
    }

    string body = type + "\n" + msgId + relayMsgId + "\n" + "S" + status + "\n" + msgContent;
    body = base64_encode(body.c_str(), body.length());

    int size = body.size();
    return to_string(size) + "@" + body;
}