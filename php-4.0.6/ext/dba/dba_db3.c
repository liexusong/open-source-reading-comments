/*
   +----------------------------------------------------------------------+
   | PHP HTML Embedded Scripting Language Version 3.0                     |
   +----------------------------------------------------------------------+
   | Copyright (c) 1999 PHP Development Team (See Credits file)           |
   +----------------------------------------------------------------------+
   | This program is free software; you can redistribute it and/or modify |
   | it under the terms of one of the following licenses:                 |
   |                                                                      |
   |  A) the GNU General Public License as published by the Free Software |
   |     Foundation; either version 2 of the License, or (at your option) |
   |     any later version.                                               |
   |                                                                      |
   |  B) the PHP License as published by the PHP Development Team and     |
   |     included in the distribution in the file: LICENSE                |
   |                                                                      |
   | This program is distributed in the hope that it will be useful,      |
   | but WITHOUT ANY WARRANTY; without even the implied warranty of       |
   | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        |
   | GNU General Public License for more details.                         |
   |                                                                      |
   | You should have received a copy of both licenses referred to here.   |
   | If you did not, or have any questions about PHP licensing, please    |
   | contact core@php.net.                                                |
   +----------------------------------------------------------------------+
   | Authors: Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
 */

/* $Id: dba_db3.c,v 1.9 2001/04/30 12:43:32 andi Exp $ */

#include "php.h"

#if DBA_DB3
#include "php_db3.h"
#include <sys/stat.h>

#include <string.h>
#ifdef DB3_INCLUDE_FILE
#include DB3_INCLUDE_FILE
#else
#include <db.h>
#endif

#define DB3_DATA dba_db3_data *dba = info->dbf
#define DB3_GKEY \
	DBT gkey; \
	memset(&gkey, 0, sizeof(gkey)); \
	gkey.data = (char *) key; gkey.size = keylen

typedef struct {
	DB *dbp;
	DBC *cursor;
} dba_db3_data;

DBA_OPEN_FUNC(db3)
{
	DB *dbp = NULL;
	DBTYPE type;
	int gmode = 0;
	int filemode = 0644;
	struct stat check_stat;

	type =  info->mode == DBA_READER ? DB_UNKNOWN :
		info->mode == DBA_TRUNC ? DB_BTREE :
		VCWD_STAT(info->path, &check_stat) ? DB_BTREE : DB_UNKNOWN;
	  
	gmode = info->mode == DBA_READER ? DB_RDONLY :
		info->mode == DBA_CREAT  ? DB_CREATE : 
		info->mode == DBA_WRITER ? 0         : 
		info->mode == DBA_TRUNC ? DB_CREATE | DB_TRUNCATE : -1;

	if (gmode == -1)
		return FAILURE;

	if (info->argc > 0) {
		convert_to_long_ex(info->argv[0]);
		filemode = (*info->argv[0])->value.lval;
	}

	if (db_create(&dbp, NULL, 0) == 0 &&
			dbp->open(dbp, info->path, NULL, type, gmode, filemode) == 0) {
		dba_db3_data *data;

		data = malloc(sizeof(*data));
		data->dbp = dbp;
		data->cursor = NULL;
		info->dbf = data;
		
		return SUCCESS;
	} else if (dbp != NULL) {
		dbp->close(dbp, 0);
	}

	return FAILURE;
}

DBA_CLOSE_FUNC(db3)
{
	DB3_DATA;
	
	if (dba->cursor) dba->cursor->c_close(dba->cursor);
	dba->dbp->close(dba->dbp, 0);
	free(dba);
}

DBA_FETCH_FUNC(db3)
{
	DBT gval;
	char *new = NULL;
	DB3_DATA;
	DB3_GKEY;
	
	memset(&gval, 0, sizeof(gval));
	if (!dba->dbp->get(dba->dbp, NULL, &gkey, &gval, 0)) {
		if (newlen) *newlen = gval.size;
		new = estrndup(gval.data, gval.size);
	}
	return new;
}

DBA_UPDATE_FUNC(db3)
{
	DBT gval;
	DB3_DATA;
	DB3_GKEY;
	
	memset(&gval, 0, sizeof(gval));
	gval.data = (char *) val;
	gval.size = vallen;

	if (!dba->dbp->put(dba->dbp, NULL, &gkey, &gval, 
				mode == 1 ? DB_NOOVERWRITE : 0)) {
		return SUCCESS;
	}
	return FAILURE;
}

DBA_EXISTS_FUNC(db3)
{
	DBT gval;
	DB3_DATA;
	DB3_GKEY;
	
	memset(&gval, 0, sizeof(gval));
	if (!dba->dbp->get(dba->dbp, NULL, &gkey, &gval, 0)) {
		return SUCCESS;
	}
	return FAILURE;
}

DBA_DELETE_FUNC(db3)
{
	DB3_DATA;
	DB3_GKEY;

	return dba->dbp->del(dba->dbp, NULL, &gkey, 0) ? FAILURE : SUCCESS;
}

DBA_FIRSTKEY_FUNC(db3)
{
	DB3_DATA;

	if (dba->cursor) {
		dba->cursor->c_close(dba->cursor);
	}

	dba->cursor = NULL;
	if (dba->dbp->cursor(dba->dbp, NULL, &dba->cursor, 0) != 0) {
		return NULL;
	}

	/* we should introduce something like PARAM_PASSTHRU... */
	return dba_nextkey_db3(info, newlen);
}

DBA_NEXTKEY_FUNC(db3)
{
	DB3_DATA;
	DBT gkey, gval;
	char *nkey = NULL;
	
	memset(&gkey, 0, sizeof(gkey));
	memset(&gval, 0, sizeof(gval));

	if (dba->cursor->c_get(dba->cursor, &gkey, &gval, DB_NEXT) == 0) {
		if (gkey.data) {
			nkey = estrndup(gkey.data, gkey.size);
			if (newlen) *newlen = gkey.size;
		}
	}

	return nkey;
}

DBA_OPTIMIZE_FUNC(db3)
{
	return SUCCESS;
}

DBA_SYNC_FUNC(db3)
{
	DB3_DATA;

	return dba->dbp->sync(dba->dbp, 0) ? FAILURE : SUCCESS;
}

#endif
