#include <stdio.h>
#include <stdlib.h>
#include "db.h"

sqlite3 *db = NULL;

int db_open(const char *path) {
    if (sqlite3_open(path, &db) != SQLITE_OK) {
        fprintf(stderr, "Erro ao abrir banco de dados: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    return 1;
}

void db_close(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

void db_init(void) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "    id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    username      TEXT    NOT NULL UNIQUE,"
        "    password_hash TEXT    NOT NULL,"
        "    created_at    TEXT    NOT NULL DEFAULT (datetime('now'))"
        ");"

        "CREATE TABLE IF NOT EXISTS accounts ("
        "    id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    user_id INTEGER NOT NULL,"
        "    name    TEXT    NOT NULL,"
        "    balance REAL    NOT NULL DEFAULT 0.0,"
        "    FOREIGN KEY (user_id) REFERENCES users(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS categories ("
        "    id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    user_id INTEGER NOT NULL,"
        "    name    TEXT    NOT NULL,"
        "    FOREIGN KEY (user_id) REFERENCES users(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS credit_cards ("
        "    id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    user_id INTEGER NOT NULL,"
        "    name    TEXT    NOT NULL,"
        "    \"limit\"   REAL    NOT NULL,"
        "    due_day INTEGER NOT NULL CHECK (due_day BETWEEN 1 AND 28),"
        "    FOREIGN KEY (user_id) REFERENCES users(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS transactions ("
        "    id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    user_id        INTEGER NOT NULL,"
        "    account_id     INTEGER NOT NULL,"
        "    category_id    INTEGER,"
        "    credit_card_id INTEGER,"
        "    transfer_id    INTEGER,"
        "    type           TEXT    NOT NULL CHECK (type IN ('income','expense','transfer')),"
        "    amount         REAL    NOT NULL CHECK (amount > 0),"
        "    date           TEXT    NOT NULL,"
        "    description    TEXT    NOT NULL,"
        "    FOREIGN KEY (user_id)        REFERENCES users(id),"
        "    FOREIGN KEY (account_id)     REFERENCES accounts(id),"
        "    FOREIGN KEY (category_id)    REFERENCES categories(id),"
        "    FOREIGN KEY (credit_card_id) REFERENCES credit_cards(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS goals ("
        "    id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    user_id        INTEGER NOT NULL,"
        "    name           TEXT    NOT NULL,"
        "    target_amount  REAL    NOT NULL CHECK (target_amount > 0),"
        "    current_amount REAL    NOT NULL DEFAULT 0.0,"
        "    deadline       TEXT    NOT NULL,"
        "    FOREIGN KEY (user_id) REFERENCES users(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS bills ("
        "    id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    user_id        INTEGER NOT NULL,"
        "    description    TEXT    NOT NULL,"
        "    amount         REAL    NOT NULL CHECK (amount > 0),"
        "    due_day        INTEGER NOT NULL CHECK (due_day BETWEEN 1 AND 31),"
        "    paid           INTEGER NOT NULL DEFAULT 0 CHECK (paid IN (0,1)),"
        "    transaction_id INTEGER,"
        "    FOREIGN KEY (user_id)        REFERENCES users(id),"
        "    FOREIGN KEY (transaction_id) REFERENCES transactions(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS loans ("
        "    id                INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    user_id           INTEGER NOT NULL,"
        "    creditor          TEXT    NOT NULL,"
        "    total_amount      REAL    NOT NULL CHECK (total_amount > 0),"
        "    remaining_amount  REAL    NOT NULL,"
        "    installments      INTEGER NOT NULL CHECK (installments > 0),"
        "    installments_paid INTEGER NOT NULL DEFAULT 0,"
        "    FOREIGN KEY (user_id) REFERENCES users(id)"
        ");";

    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "Erro ao inicializar schema: %s\n", err);
        sqlite3_free(err);
        db_close();
        exit(EXIT_FAILURE);
    }
    
    // const char *sqlUser = "INSERT OR IGNORE INTO users (username, password_hash) VALUES ('admin', 'admin');";
    // if (sqlite3_exec(db, sqlUser, NULL, NULL, &err) != SQLITE_OK) {
    //     fprintf(stderr, "Erro ao criar usuario admin: %s\n", err);
    //     sqlite3_free(err);
    //     db_close();
    //     exit(EXIT_FAILURE);
    // }
}

sqlite3* returnConnection(void)
{
    if(db){
        return db; //Retorno esta conexão para ser usada em outras partes do código
    }
    return NULL;
}