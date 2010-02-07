#ifndef _ONLINE_COMMAND_H
#define _ONLINE_COMMAND_H
#include "StdAfx.h"

class OnlineCommand : public Command
{
private:
	Realm * m_realm;

public:
	OnlineCommand(IRCSession* pSession, string target, string sender, string text) : Command(pSession, target, sender, text)
	{
	}

	virtual const char* getHelpText() { return "Format: !online $RealmName\nFor a list of realms, type !realms."; }

	bool isSyntaxOk()
	{
		GET_REALM_FROM_PARAM_OR_FAIL(m_realm);

		return true;
	}

	void run()
	{
		QueryResult tquery = m_realm->GetDB()->Query("SELECT COUNT(*) FROM characters WHERE online > 0");
		QueryResult aquery = m_realm->GetDB()->Query("SELECT COUNT(*) FROM characters WHERE race IN (1,3,4,7,11) AND online > 0");
		if(tquery && aquery)
		{
			uint32 CountTotal = tquery->Fetch()[0].GetUInt32();
			uint32 CountAlliance = aquery->Fetch()[0].GetUInt32();
			uint32 CountHorde = CountTotal - CountAlliance;

			GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "There are a total of %u characters online, consisting of %u Horde and %u Alliance.", CountTotal, CountHorde, CountAlliance);
		}
	}

	static Command* Create(IRCSession* pSession, string target, string sender, string text)
	{
		return new OnlineCommand(pSession, target, sender, text);
	}
};

#endif