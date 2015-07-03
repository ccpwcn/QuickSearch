#pragma once

#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include "CommonModule.h"

typedef class db_mgr
{
public:
	db_mgr();
	~db_mgr();
	int Update(__in const FIND_DATA & fd);
	int Insert(__in const FIND_DATA & fd);
	int Delete(__in const FIND_DATA & fd);
	int Query(__in_opt const FIND_DATA & fd);
private:
	CHAR m_szSQLiteDll[MAX_PATH];
	CHAR m_szDBFilename[MAX_PATH];
}DB_MGR, *LPDB_MGR;