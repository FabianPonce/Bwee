#ifndef _REALM_H
#define _REALM_H

struct Realm
{
private:
	std::string m_name;
	MySQLConnection* m_db;

public:
	Realm(std::string name, MySQLConnection* dbConn)
	{
		this->m_name = name;
		m_db = dbConn;
	}

	MySQLConnection* GetDB() { return m_db; }
	std::string GetName() { return m_name; }
};

#endif