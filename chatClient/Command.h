#pragma once
#ifndef MC_COMMANDS
#define MC_COMMANDS
#include "ClientSocketProxy.h"

#include <WinSock2.h>
#include <string>
#include <vector>
using namespace std;
namespace mc
{
	enum class CommandType
	{
		Name,
		Hi,
		Broadcast,
		At,
		Bye
	};
	template<typename TComm>
	class CommandBase
	{
	protected:
		string error;
	public:
		CommandType CmdType;
		virtual bool CanExecute(ProxyBase<TComm>* pb, string& username);
		bool Execute(ProxyBase<TComm>* pb, string& username);
		virtual bool InnerExecute(ProxyBase<TComm>* pb, string& username);
		static CommandBase<TComm>* GetCommand(string cmd);
		string GetError() { return error; }
		void ClearError() { error = ""; }
	};

	template<typename TComm>
	class HiCommand : public CommandBase<TComm>
	{
	private:
		string cmd;
		string ip;
		UINT16 port;
		string password;
		bool createProxy(ProxyBase<TComm>* pb);
		string collectUsername(ProxyBase<TComm>* pb);
		bool checkUserExistance(ProxyBase<TComm>* pb, string username);
		string collectPassword(ProxyBase<TComm>* pb, bool again = false);
		bool regist(ProxyBase<TComm>* pb, string username, string password);
		bool login(ProxyBase<TComm>* pb, string username, string password);
	public:
		HiCommand(string ip, UINT16 port) :ip(ip), port(port)
		{
			this->CmdType = CommandType::Hi;
		}

		bool InnerExecute(ProxyBase<TComm>* pb, string& name);
		bool CanExecute(ProxyBase<TComm>* pb, string& name);
	};

	template<typename TComm>
	class BroadcastCommand : public CommandBase<TComm>
	{
	protected:
		string msg;
		virtual bool sendMsg(ProxyBase<TComm>* pb, string from);
	public:
		BroadcastCommand(string message) : msg(message)
		{
			this->CmdType = CommandType::Broadcast;
		}

		bool InnerExecute(ProxyBase<TComm>* pb, string& from);
	};

	template<typename TComm>
	class NameCommand : public CommandBase<TComm>
	{
	public:
		string UserName;
		NameCommand(string name): UserName(name)
		{
			this->CmdType = CommandType::Name;
		}

		bool InnerExecute(ProxyBase<TComm>* pb, string& name);
		bool CanExecute(ProxyBase<TComm>* pb, string& name);
	};

	template<typename TComm>
	class AtCommand : public BroadcastCommand<TComm>
	{
	private:
		string to;
	protected:
		bool sendMsg(ProxyBase<TComm>* pb, string from);
	public:
		AtCommand(string to, string message)
			: BroadcastCommand<TComm>(message), to(to)
		{
			this->CmdType = CommandType::At;
		}
	};

	template<typename TComm>
	class ByeCommand : public CommandBase<TComm>
	{
	private:
		bool logoff(ProxyBase<TComm>* pb, string from);
	public:
		ByeCommand()
		{
			this->CmdType = CommandType::Bye;
		}

		bool InnerExecute(ProxyBase<TComm>* pb, string& from);
	};
}
#endif