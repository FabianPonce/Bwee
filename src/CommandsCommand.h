#ifndef _COMMANDS_COMMAND_H
#define _COMMANDS_COMMAND_H
#include "StdAfx.h"

class CommandsCommand : public Command
{
public:
	CommandsCommand(IRCSession* pSession, string target, string sender, string text) : Command(pSession, target, sender, text)
	{

	}

	bool isSyntaxOk()
	{
		return true;
	}

	void run()
	{
		string uResult = "Available commands include: " + GetSession()->GetCommandParser()->buildCommandList();
		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), uResult.c_str());
	}

	static Command* Create(IRCSession* pSession, string target, string sender, string text)
	{
		return new CommandsCommand(pSession, target, sender, text);
	}
};

#endif