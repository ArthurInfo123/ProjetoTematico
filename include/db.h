#ifndef DB_H
#define DB_H

#include <sqlite3.h>

extern sqlite3 *db;

int  db_open(const char *path);
void db_close(void);
void db_init(void);

#endif
