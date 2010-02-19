#ifndef _NULL_COMMAND_H
#define _NULL_COMMAND_H
#include "StdAfx.h"

class NullCommand : public Command
{
public:
	NullCommand(IRCSession* pSession, string target, string sender, string text) : Command(pSession, target, sender, text)
	{

	}

	bool isSyntaxOk()
	{
		return false;
	}

	void run()
	{
		
	}

	static Command* Create(IRCSession* pSession, string target, string sender, string text)
	{
		return new NullCommand(pSession, target, sender, text);
	}
};

#endif