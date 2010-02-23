#ifndef _REALMS_COMMAND_H
#define _REALMS_COMMAND_H
#include "StdAfx.h"

class RealmsCommand : public Command
{
public:
	RealmsCommand(IRCSession* pSession, string target, string sender, string text) : Command(pSession, target, sender, text)
	{

	}

	bool isSyntaxOk()
	{
		return true;
	}

	void run()
	{
		if( GetSession()->GetRealmCount() == 0 )
		{
			GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "There are no defined realms.");
			return;
		}

		stringstream ss;
		ss << "Available realms include: ";
		uint32 uRealmCount = GetSession()->GetRealmCount();
		for(uint32 i = 0; i < uRealmCount; ++i) 
		{
			Realm* uRealm = GetSession()->GetRealm(i);
			if(!uRealm)
				break;

			ss << uRealm->GetName() << ", ";
		}

		string uText = ss.str();
		// Remove the last ", "
		uText = uText.substr(0, uText.length()-2); 
		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), uText.c_str());
	}

	static Command* Create(IRCSession* pSession, string target, string sender, string text)
	{
		return new RealmsCommand(pSession, target, sender, text);
	}
};

#endif