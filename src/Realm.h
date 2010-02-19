#ifndef _REALM_H
#define _REALM_H

struct Realm
{
private:
	std::string m_name;
	MySQLConnection* m_db;

public:
	/*
	 * Constructs a Realm object with the given name and connected MySQLConnection.
	 * @param name The name of the realm
	 * @param dbConn A database connection to the character database associated with this realm.
	 */
	Realm(std::string name, MySQLConnection* dbConn)
	{
		this->m_name = name;
		m_db = dbConn;
	}

	/*
	 * Returns the MySQLConnection associated with this realm's character database.
	 */
	MySQLConnection* GetDB() { return m_db; }

	/*
	 * Returns the name of this realm, as defined in the configuration file it was loaded from.
	 */
	std::string GetName() { return m_name; }
};

#endif