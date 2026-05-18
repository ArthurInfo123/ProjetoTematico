#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "session.h"
#include "ui.h"
#include "../include/Accounts.h"

void menu_contas(void) {
    int opcao;

    do {
        ui_clear();
        ui_header("Contas Financeiras");
        printf("  1. Listar contas\n");
        printf("  2. Adicionar conta\n");
        printf("  3. Editar conta\n");
        printf("  4. Excluir conta\n");
        printf("  0. Voltar\n");
        ui_divider();
        opcao = ui_read_int("Opcao: ");

        switch (opcao) {
            case 1:
                listar_contas(session.user_id);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;

            case 2: {
                char name[100];
                ui_clear();
                ui_header("Nova Conta");
                ui_read_str("Nome da conta: ", name, sizeof(name));
                if (strlen(name) == 0) {
                    ui_error("Nome nao pode ser vazio.");
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                double balance = ui_read_double("Saldo inicial: R$ ");
                if (balance < 0) {
                    ui_error("Saldo inicial nao pode ser negativo.");
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                adicionar_conta(session.user_id, name, balance);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 3: {
                ui_clear();
                ui_header("Editar Conta");
                listar_contas(session.user_id);
                int id = ui_read_int("\nID da conta a editar: ");
                if (!validate_user_account(session.user_id, id)) {
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                char name[100];
                ui_read_str("Novo nome: ", name, sizeof(name));
                if (strlen(name) == 0) {
                    ui_error("Nome nao pode ser vazio.");
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                editar_conta(id, session.user_id, name);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 4: {
                ui_clear();
                ui_header("Excluir Conta");
                listar_contas(session.user_id);
                int id = ui_read_int("\nID da conta a excluir: ");
                if (!validate_user_account(session.user_id, id)) {
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                excluir_conta(id, session.user_id);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 0:
                break;

            default:
                ui_error("Opcao invalida.");
                printf("\nPressione Enter para continuar...");
                getchar();
        }

    } while (opcao != 0);
}

void listar_contas(int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "SELECT id, name, balance FROM accounts WHERE user_id = ? ORDER BY name;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar contas.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    printf("\n=== CONTAS ===\n");

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id                     = sqlite3_column_int(stmt, 0);
        const unsigned char *name  = sqlite3_column_text(stmt, 1);
        double balance             = sqlite3_column_double(stmt, 2);
        printf("  [%d] %-20s R$ %.2f\n", id, name, balance);
        found = 1;
    }

    if (!found)
        printf("  Nenhuma conta cadastrada.\n");

    sqlite3_finalize(stmt);
}

void adicionar_conta(int user_id, const char *name, double balance) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "INSERT INTO accounts (user_id, name, balance) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar INSERT.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, balance);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao adicionar conta.");
    } else {
        ui_success("Conta adicionada com sucesso!");
    }

    sqlite3_finalize(stmt);
}

void editar_conta(int id, int user_id, const char *name) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "UPDATE accounts SET name = ? WHERE id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar UPDATE.");
        return;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);
    sqlite3_bind_int(stmt, 3, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao editar conta.");
    } else {
        if (sqlite3_changes(db) > 0)
            ui_success("Conta editada com sucesso!");
        else
            ui_error("Conta nao encontrada.");
    }

    sqlite3_finalize(stmt);
}

void excluir_conta(int id, int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    // verificar se tem transacoes vinculadas
    const char *sql_check = "SELECT COUNT(*) FROM transactions WHERE account_id = ?;";
    if (sqlite3_prepare_v2(db, sql_check, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            if (count > 0) {
                ui_error("Nao e possivel excluir conta com transacoes vinculadas.");
                return;
            }
        } else {
            sqlite3_finalize(stmt);
        }
    }

    const char *sql = "DELETE FROM accounts WHERE id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar DELETE.");
        return;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao excluir conta.");
    } else {
        if (sqlite3_changes(db) > 0)
            ui_success("Conta excluida com sucesso!");
        else
            ui_error("Conta nao encontrada.");
    }

    sqlite3_finalize(stmt);
}

int validate_user_account(int user_id, int account_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "SELECT user_id FROM accounts WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao validar conta.");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, account_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int owner = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (owner != user_id) {
            ui_error("Acesso negado a esta conta.");
            return 0;
        }
        return 1;
    }

    sqlite3_finalize(stmt);
    ui_error("Conta nao encontrada.");
    return 0;
}