#include <stdio.h>
#include <string.h>
#include "db.h"
#include "session.h"
#include "ui.h"
#include "sha256.h"
#include "auth.h"

#define PASSWORD_MAX 128
#define HASH_HEX_LEN 65

static void hash_senha(const char *senha, char *hash_out) {
    sha256_hex(senha, hash_out);
}

void auth_cadastro(void) {
    char username[USERNAME_MAX];
    char senha[PASSWORD_MAX];
    char confirmacao[PASSWORD_MAX];
    char hash[HASH_HEX_LEN];

    ui_clear();
    ui_header("Cadastro de Usuario");

    ui_read_str("Nome de usuario: ", username, USERNAME_MAX);
    if (strlen(username) == 0) {
        ui_error("Nome de usuario nao pode ser vazio.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    ui_read_str("Senha: ", senha, PASSWORD_MAX);
    if (strlen(senha) == 0) {
        ui_error("Senha nao pode ser vazia.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    ui_read_str("Confirme a senha: ", confirmacao, PASSWORD_MAX);
    if (strlen(confirmacao) == 0) {
        ui_error("Confirmacao nao pode ser vazia.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    if (strcmp(senha, confirmacao) != 0) {
        ui_error("As senhas nao coincidem.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    hash_senha(senha, hash);

    sqlite3 *conn = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "INSERT INTO users (username, password_hash) VALUES (?, ?);";

    if (sqlite3_prepare_v2(conn, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar cadastro.");
        return;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash,     -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (result == SQLITE_DONE) {
        ui_success("Usuario cadastrado com sucesso!");
    } else if (result == SQLITE_CONSTRAINT) {
        ui_error("Nome de usuario ja existe. Escolha outro.");
    } else {
        ui_error("Erro ao cadastrar usuario.");
    }

    printf("\nPressione Enter para continuar...");
    getchar();
}

void auth_login(void) {
    char username[USERNAME_MAX];
    char senha[PASSWORD_MAX];
    char hash[HASH_HEX_LEN];

    ui_clear();
    ui_header("Login");

    int tentativas = 0;

    while (tentativas < 3) {
        ui_read_str("Usuario: ", username, USERNAME_MAX);
        ui_read_str("Senha: ",   senha,    PASSWORD_MAX);

        hash_senha(senha, hash);

        sqlite3 *conn = returnConnection();
        sqlite3_stmt *stmt;

        const char *sql = "SELECT id, username FROM users WHERE username = ? AND password_hash = ?;";

        if (sqlite3_prepare_v2(conn, sql, -1, &stmt, NULL) != SQLITE_OK) {
            ui_error("Erro interno. Tente novamente.");
            printf("\nPressione Enter para continuar...");
            getchar();
            return;
        }

        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, hash,     -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int uid = sqlite3_column_int(stmt, 0);
            const char *uname = (const char *)sqlite3_column_text(stmt, 1);
            session_start(uid, uname);
            sqlite3_finalize(stmt);
            ui_success("Login realizado com sucesso!");
            printf("  Bem-vindo, %s!\n", session.username);
            printf("\nPressione Enter para continuar...");
            getchar();
            return;
        }

        sqlite3_finalize(stmt);
        tentativas++;

        if (tentativas < 3) {
            printf("\n[ERRO] Usuario ou senha invalidos. Tentativa %d/3.\n\n", tentativas);
        }
    }

    ui_error("Numero maximo de tentativas atingido.");
    printf("\nPressione Enter para continuar...");
    getchar();
}