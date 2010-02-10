#ifndef _HELP_COMMAND_H
#define _HELP_COMMAND_H
#include "StdAfx.h"

class HelpCommand : public Command
{
private:
	string m_commandName;
public:
	HelpCommand(IRCSession* pSession, string target, string sender, string text) : Command(pSession, target, sender, text)
	{

	}

	virtual const char* getHelpText() { return "Format: !help $CommandName.\nFor a list of commands, type !commands."; }

	bool isSyntaxOk()
	{
		if( !m_reader.hasNextWord() )
			return false;

		m_commandName = m_reader.getRemainder();
		return true;
	}

	void run()
	{
		GetSession()->GetCommandParser()->sendHelpTextForCommand(m_commandName, m_target);
	}

	static Command* Create(IRCSession* pSession, string target, string sender, string text)
	{
		return new HelpCommand(pSession, target, sender, text);
	}
};

#endif