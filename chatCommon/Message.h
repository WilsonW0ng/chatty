#pragma once
#ifndef MS_MESSAGE
#define MS_MESSAGE
#include <string>
#include <time.h>
#include <WinSock2.h>
#include <unordered_map>
using namespace std;

static const string CLIENT_ACTIONS_BROADCAST = "Broadcast";
static const string CLIENT_ACTIONS_AT = "At";

static const string SERVER_ACTIONS_REGIST = "Regist";
static const string SERVER_ACTIONS_LOGIN = "Login";
static const string SERVER_ACTIONS_LOGOFF = "Logoff";
static const string SERVER_ACTIONS_BROADCAST = "Broadcast";
static const string SERVER_ACTIONS_AT = "At";
static const string SERVER_ACTIONS_CHECKUSER_REGISTS = "CheckUserRegists";
static const string SERVER_ACTIONS_CLIENTCLOSED = "ClientClosed";
static const string SERVER_ACTIONS_CLIENTHB = "Heartbeat";

class MessageBase
{
private:
	unsigned long long m_MsgId;

public:
	MessageBase() : m_MsgId(time(NULL)) { }
	MessageBase(unsigned long long msgId) : m_MsgId(msgId) { }

	static MessageBase* FromString(string message);
	virtual string ToString() = 0;
	unsigned long long GetMsgId() { return m_MsgId; }
};

class RequestMessage : public MessageBase
{
public:
	string Action;
	unordered_map<int, string> FieldMap;

	/// <summary>
	/// request with parameters
	/// </summary>
	/// <param name="action"></param>
	/// <param name="fieldMap"></param>
	RequestMessage(string action, unordered_map<int, string> fieldMap)
		: MessageBase(), Action(action), FieldMap(fieldMap) { }

	/// <summary>
	/// reconstruct the request message from string.
	/// </summary>
	/// <param name="msgId"></param>
	/// <param name="body"></param>
	RequestMessage(unsigned long long msgId, string action, string fieldStr) 
		: MessageBase(msgId), Action(action)
	{
		ResolveFieldMap(fieldStr);
	}

	void ResolveFieldMap(string fieldStr);
	virtual string ToString();
};

enum class ResponseStatus
{
	OK = 0,
	NOK = 1
};

class ResponseMessage : public MessageBase
{
private:
	unsigned long long m_RelayMsgId;
public:
	ResponseStatus Status;
	unordered_map<int, string> FieldMap;
	/// <summary>
	/// The messages generated during the action execute. 
	/// But they have to send back to MC after the response message.
	/// This is to make sure the correct order of messages in MC's perspective.
	/// </summary>
	vector<MessageBase*> PostingMsgs;

	/// <summary>
	/// reconstruct the response message from string.
	/// </summary>
	/// <param name="msgId"></param>
	/// <param name="relayMsgId"></param>
	/// <param name="body"></param>
	ResponseMessage(unsigned long long msgId, unsigned long long relayMsgId, ResponseStatus status, string body)
		: MessageBase(msgId) , m_RelayMsgId(relayMsgId), Status(status)
	{
		ResolveFieldMap(body);
	}

	/// <summary>
	/// response with no returned value.
	/// </summary>
	ResponseMessage(unsigned long long relayMsgId, ResponseStatus status = ResponseStatus::OK)
		: MessageBase(), m_RelayMsgId(relayMsgId), Status(status) { }

	/// <summary>
	/// response with returned value.
	/// </summary>
	/// <param name="relayMsgId"></param>
	/// <param name="error"></param>
	/// <param name="status"></param>
	ResponseMessage(unsigned long long relayMsgId, unordered_map<int, string> fieldMap, ResponseStatus status = ResponseStatus::OK)
		: MessageBase(), FieldMap(fieldMap), m_RelayMsgId(relayMsgId), Status(status) { }

	/// <summary>
	/// response with error.
	/// </summary>
	ResponseMessage(unsigned long long relayMsgId, string error, ResponseStatus status = ResponseStatus::NOK)
		: MessageBase(), m_RelayMsgId(relayMsgId), Status(status) 
	{ 
		FieldMap.insert({ 0, error });
	}

	unsigned long long GetRelayMsgId() { return m_RelayMsgId; }
	
	void ResolveFieldMap(string fieldStr);
	virtual string ToString();

	string GetError() {
		if (Status == ResponseStatus::NOK)
			return FieldMap[0];
	}
};

class PostedMessage : public RequestMessage
{
public:
	PostedMessage(string action, unordered_map<int, string> fieldMap)
		: RequestMessage(action, fieldMap)  { }
};
#endif