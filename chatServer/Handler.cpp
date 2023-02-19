#include "Handler.h"
#include "Utilities.h"
#include <fstream>
#include <WinSock2.h>
#include <unordered_map>
#include <cstdio>

namespace ms
{
	SOCKET Handler<SOCKET>::GetId()
	{
		return m_ID;
	}

	/// <summary>
	/// for simplicity, store one message storage file for one use. 
	/// for simplicity, delete the file instead of update the message records. So, it does not support message history.
	/// TODO: maybe, move the file content to user history file is a good idea.
	/// </summary>
	/// <returns></returns>
	vector<MessageBase*> getMessages(string user)
	{
		vector<MessageBase*> res;
		fstream f;
		// the message file is by user (recipent)
		string path = ".\\" + user + ".msgdata";
		f.open(path, ios::in);
		if (!f.good())
		{
			f.close();
			return res;
		}

		string msg;
		while (f >> msg)
		{
			res.push_back(MessageBase::FromString(msg.substr(msg.find("@") + 1)));
		}

		f.close();

		if (remove(path.c_str()) != 0)
		{
			Utilities::QuitWithError("failed to delete the message history.");
		}

		return res;
	}

	/// <summary>
	/// Noted, only posted message can be stored. 
	/// The response message(having relaying message waiting in client side) are not good candidate for storage like this.
	/// </summary>
	/// <param name="to"></param>
	/// <param name="msg"></param>
	void addMessage(string user, string msg)
	{
		fstream f;
		// create message file if does not exist.

		// the message file is by user (recipent)
		string path = ".\\" + user + ".msgdata";
		f.open(path, ios::out | ios::app);
		f << msg << endl;
		f.close();
	}

	vector<MessageBase*> Handler<SOCKET>::Logon(ProxyBase<SOCKET>* pb, SOCKET socket)
	{
		vector<MessageBase*> msgs = getMessages(Name);

		this->m_IsLoggedOn = true;
		this->m_ID = socket;

		return msgs;
	}

	// Terminate
	void Handler<SOCKET>::Logoff()
	{
		this->m_IsLoggedOn = false;
	}

	void Handler<SOCKET>::Disconnect()
	{
		this->m_ID = 0;
	}


	void Handler<SOCKET>::Send(ProxyBase<SOCKET>* pb, MessageBase* message)
	{
		if(m_IsLoggedOn)
			pb->Send(m_ID, message);
		else
			addMessage(Name, message->ToString());
	}
}