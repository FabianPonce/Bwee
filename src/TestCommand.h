#ifndef _TEST_COMMAND_H
#define _TEST_COMMAND_H
#include "StdAfx.h"

class TestCommand : public Command
{
public:
	TestCommand(IRCSession* pSession, string target, string sender, string text) : Command(pSession, target, sender, text)
	{
		
	}

	bool isSyntaxOk()
	{
		return true;
	}

	void run()
	{
		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "This is a test of the NextGen command system for Bwee!");
	}

	static Command* Create(IRCSession* pSession, string target, string sender, string text)
	{
		return new TestCommand(pSession, target, sender, text);
	}
};

#endif