#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "db.h"
#include "session.h"
#include "ui.h"
#include "sha256.h"
#include "auth.h"

#define PASSWORD_MAX 128
#define HASH_HEX_LEN 65
#define CODE_LEN     12   // MF-XXXX-XXXX

static void hash_senha(const char *senha, char *hash_out) {
    sha256_hex(senha, hash_out);
}

// Gera codigo no formato MF-XXXX-XXXX
static void gerar_codigo(char *out) {
    const char chars[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    int n = (int)(sizeof(chars) - 1);
    srand((unsigned int)time(NULL));

    out[0]='M'; out[1]='F'; out[2]='-';
    for (int i = 0; i < 4; i++) out[3+i] = chars[rand() % n];
    out[7] = '-';
    for (int i = 0; i < 4; i++) out[8+i] = chars[rand() % n];
    out[12] = '\0';
}

static void salvar_codigo(int user_id, const char *code) {
    char hash[HASH_HEX_LEN];
    sha256_hex(code, hash);

    sqlite3 *conn = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql =
        "INSERT OR REPLACE INTO recovery_codes (user_id, code_hash, used) "
        "VALUES (?, ?, 0);";

    if (sqlite3_prepare_v2(conn, sql, -1, &stmt, NULL) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, hash, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
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

        // codigo de recuperacao — opcional
        printf("\nDeseja configurar um codigo de recuperacao de senha?\n");
        printf("  1. Sim\n  0. Nao\n");
        int opcao = ui_read_int("Opcao: ");

        if (opcao == 1) {
            // buscar o user_id recem criado
            sqlite3_stmt *stmt2;
            const char *sql2 = "SELECT id FROM users WHERE username = ?;";
            if (sqlite3_prepare_v2(conn, sql2, -1, &stmt2, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt2, 1, username, -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt2) == SQLITE_ROW) {
                    int new_uid = sqlite3_column_int(stmt2, 0);
                    sqlite3_finalize(stmt2);

                    char code[CODE_LEN + 1];
                    gerar_codigo(code);
                    salvar_codigo(new_uid, code);

                    ui_divider();
                    printf("  CODIGO DE RECUPERACAO:\n\n");
                    printf("      %s\n\n", code);
                    printf("  IMPORTANTE: Guarde este codigo em lugar seguro.\n");
                    printf("  Ele nao sera exibido novamente.\n");
                    ui_divider();
                } else {
                    sqlite3_finalize(stmt2);
                }
            }
        }

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

        const char *sql =
            "SELECT id, username FROM users WHERE username = ? AND password_hash = ?;";

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

        printf("\n[ERRO] Usuario ou senha invalidos. Tentativa %d/3.\n", tentativas);

        if (tentativas < 3) {
            printf("\n  1. Tentar novamente\n  0. Voltar\n");
            int opcao = ui_read_int("Opcao: ");
            if (opcao == 0) return;
        }
    }

    ui_error("Numero maximo de tentativas atingido.");
    printf("\nPressione Enter para continuar...");
    getchar();
}

void auth_recuperar_senha(void) {
    char username[USERNAME_MAX];
    char code_input[CODE_LEN + 1];
    char code_hash[HASH_HEX_LEN];

    ui_clear();
    ui_header("Recuperar Senha");

    ui_read_str("Nome de usuario: ", username, USERNAME_MAX);
    if (strlen(username) == 0) {
        ui_error("Nome de usuario nao pode ser vazio.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    // buscar user_id e verificar se tem codigo configurado
    sqlite3 *conn = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql_user = "SELECT id FROM users WHERE username = ?;";
    if (sqlite3_prepare_v2(conn, sql_user, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro interno.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        // mensagem generica para nao revelar se usuario existe
        ui_error("Usuario ou codigo invalido.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    int uid = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    // verificar se tem codigo configurado
    const char *sql_check =
        "SELECT code_hash FROM recovery_codes WHERE user_id = ? AND used = 0;";
    if (sqlite3_prepare_v2(conn, sql_check, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro interno.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3_bind_int(stmt, 1, uid);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        ui_error("Recuperacao de senha nao configurada para este usuario.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    const char *stored_hash = (const char *)sqlite3_column_text(stmt, 0);
    char stored_hash_copy[HASH_HEX_LEN];
    strncpy(stored_hash_copy, stored_hash, HASH_HEX_LEN);
    sqlite3_finalize(stmt);

    // pedir o codigo
    ui_read_str("Codigo de recuperacao: ", code_input, sizeof(code_input));
    sha256_hex(code_input, code_hash);

    if (strcmp(code_hash, stored_hash_copy) != 0) {
        ui_error("Usuario ou codigo invalido.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    // codigo correto — pedir nova senha
    char nova_senha[PASSWORD_MAX];
    char confirmacao[PASSWORD_MAX];

    ui_read_str("Nova senha: ", nova_senha, PASSWORD_MAX);
    if (strlen(nova_senha) == 0) {
        ui_error("Senha nao pode ser vazia.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    ui_read_str("Confirme a nova senha: ", confirmacao, PASSWORD_MAX);
    if (strcmp(nova_senha, confirmacao) != 0) {
        ui_error("As senhas nao coincidem.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    char new_hash[HASH_HEX_LEN];
    sha256_hex(nova_senha, new_hash);

    // atualizar senha e marcar codigo como usado atomicamente
    sqlite3_exec(conn, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    int ok = 1;

    const char *sql_upd = "UPDATE users SET password_hash = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(conn, sql_upd, -1, &stmt, NULL) != SQLITE_OK) {
        ok = 0;
    } else {
        sqlite3_bind_text(stmt, 1, new_hash, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, uid);
        if (sqlite3_step(stmt) != SQLITE_DONE) ok = 0;
        sqlite3_finalize(stmt);
    }

    if (ok) {
        const char *sql_used =
            "UPDATE recovery_codes SET used = 1 WHERE user_id = ?;";
        if (sqlite3_prepare_v2(conn, sql_used, -1, &stmt, NULL) != SQLITE_OK) {
            ok = 0;
        } else {
            sqlite3_bind_int(stmt, 1, uid);
            if (sqlite3_step(stmt) != SQLITE_DONE) ok = 0;
            sqlite3_finalize(stmt);
        }
    }

    if (ok) {
        sqlite3_exec(conn, "COMMIT;", NULL, NULL, NULL);
        ui_success("Senha alterada com sucesso! Faca login com a nova senha.");
    } else {
        sqlite3_exec(conn, "ROLLBACK;", NULL, NULL, NULL);
        ui_error("Erro ao alterar senha. Tente novamente.");
    }

    printf("\nPressione Enter para continuar...");
    getchar();
}
