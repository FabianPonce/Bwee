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
		string uCmd = m_reader.getRemainder();
		if( uCmd.length() )
		{
			string uResult = GetSession()->GetCommandParser()->buildCommandList(uCmd);
			GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), uResult.c_str());
			return;
		}

		string uResult = "Available commands include: " + GetSession()->GetCommandParser()->buildCommandList() + ".";
		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), uResult.c_str());
		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "You may use !commands $Command to get a list of subcommands for commands with astericks by their names.");
	}

	static Command* Create(IRCSession* pSession, string target, string sender, string text)
	{
		return new CommandsCommand(pSession, target, sender, text);
	}
};

#endif