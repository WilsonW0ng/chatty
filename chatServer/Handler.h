#pragma once
#ifndef MS_HANDLER
#define MS_HANDLER
#include <Message.h>
#include "ServerSocketProxy.h"
#include <string>
#include <iostream>
#include <unordered_map>
using namespace std;

namespace ms
{
	template <typename T>
	class Handler
	{
	private:
		T m_ID;
		bool m_IsLoggedOn{ false };
	public:
		string Name;
		string Password;
		Handler() { }
		Handler(string name, string password) :Name(name), Password(password) { }
		Handler(T id, string name, string password) : Handler(name, password), m_ID(id) { }

		T GetId();
		bool GetLogonStatus() { return m_IsLoggedOn; }

		vector<MessageBase*> Logon(ProxyBase<SOCKET>* pb, T id);

		// Terminate
		void Logoff();

		void Disconnect();

		void Send(ProxyBase<SOCKET>* pb, MessageBase* message);

		/*ostream& operator<<(ostream& out)
		{
			out << "MC:" << Name << ", Handler:" << m_ID;
			return out;
		}*/
	};
}
#endif