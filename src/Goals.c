#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "session.h"
#include "ui.h"
#include "Validation.h"
#include "Accounts.h"
#include "../include/Goals.h"

void menu_metas(void) {
    int opcao;

    do {
        ui_clear();
        ui_header("Metas de Economia");
        printf("  1. Listar metas\n");
        printf("  2. Nova meta\n");
        printf("  3. Alocar valor para meta\n");
        printf("  4. Excluir meta\n");
        printf("  0. Voltar\n");
        ui_divider();
        opcao = ui_read_int("Opcao: ");

        switch (opcao) {
            case 1:
                listar_metas(session.user_id);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;

            case 2: {
                ui_clear();
                ui_header("Nova Meta");

                char name[100];
                ui_read_str("Nome da meta: ", name, sizeof(name));
                if (strlen(name) == 0) {
                    ui_error("Nome nao pode ser vazio.");
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }

                double target = ui_read_double("Valor alvo: R$ ");
                if (!validate_amount(target)) {
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }

                char deadline[11];
                ui_read_str("Prazo (DD/MM/AAAA): ", deadline, sizeof(deadline));
                if (!validate_date(deadline)) {
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }

                adicionar_meta(session.user_id, name, target, deadline);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 3: {
                ui_clear();
                ui_header("Alocar Valor para Meta");

                listar_metas(session.user_id);
                int goal_id = ui_read_int("\nID da meta: ");
                if (!validate_user_goal(session.user_id, goal_id)) {
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }

                printf("\n");
                listar_contas(session.user_id);
                int account_id = ui_read_int("\nID da conta de origem: ");

                double amount = ui_read_double("Valor a alocar: R$ ");
                if (!validate_amount(amount)) {
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }

                alocar_valor_meta(goal_id, session.user_id, account_id, amount);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 4: {
                ui_clear();
                ui_header("Excluir Meta");
                listar_metas(session.user_id);
                int id = ui_read_int("\nID da meta a excluir: ");
                excluir_meta(id, session.user_id);
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

void listar_metas(int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT id, name, target_amount, current_amount, deadline "
        "FROM goals WHERE user_id = ? ORDER BY deadline;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar metas.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    printf("\n=== METAS ===\n");

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int    id             = sqlite3_column_int(stmt, 0);
        const unsigned char *name   = sqlite3_column_text(stmt, 1);
        double target         = sqlite3_column_double(stmt, 2);
        double current        = sqlite3_column_double(stmt, 3);
        const unsigned char *deadline = sqlite3_column_text(stmt, 4);

        double pct = (target > 0) ? (current / target * 100.0) : 0.0;
        if (pct > 100.0) pct = 100.0;

        printf("  [%d] %-20s Prazo: %s\n", id, name, deadline);
        printf("       R$ %.2f / R$ %.2f (%.1f%%)\n", current, target, pct);
        found = 1;
    }

    if (!found)
        printf("  Nenhuma meta cadastrada.\n");

    sqlite3_finalize(stmt);
}

void adicionar_meta(int user_id, const char *name, double target_amount, const char *deadline) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql =
        "INSERT INTO goals (user_id, name, target_amount, current_amount, deadline) "
        "VALUES (?, ?, ?, 0.0, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar INSERT.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, target_amount);
    sqlite3_bind_text(stmt, 4, deadline, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao cadastrar meta.");
    } else {
        ui_success("Meta cadastrada com sucesso!");
    }

    sqlite3_finalize(stmt);
}

void alocar_valor_meta(int goal_id, int user_id, int account_id, double amount) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    // verificar saldo da conta
    const char *sql_saldo = "SELECT balance FROM accounts WHERE id = ? AND user_id = ?;";
    if (sqlite3_prepare_v2(db, sql_saldo, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao verificar saldo.");
        return;
    }
    sqlite3_bind_int(stmt, 1, account_id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        ui_error("Conta nao encontrada.");
        return;
    }

    double saldo = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);

    if (saldo < amount) {
        printf("[AVISO] Saldo insuficiente (R$ %.2f disponivel).\n", saldo);
        printf("Deseja prosseguir mesmo assim? 1. Sim  0. Nao\n");
        int confirma = ui_read_int("Opcao: ");
        if (confirma != 1) {
            printf("Operacao cancelada.\n");
            return;
        }
    }

    // verificar se meta ja foi atingida
    const char *sql_meta = "SELECT target_amount, current_amount FROM goals WHERE id = ? AND user_id = ?;";
    if (sqlite3_prepare_v2(db, sql_meta, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao verificar meta.");
        return;
    }
    sqlite3_bind_int(stmt, 1, goal_id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        ui_error("Meta nao encontrada.");
        return;
    }

    double target  = sqlite3_column_double(stmt, 0);
    double current = sqlite3_column_double(stmt, 1);
    sqlite3_finalize(stmt);

    if (current >= target) {
        ui_error("Esta meta ja foi atingida.");
        return;
    }

    // executar alocacao atomicamente
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    int ok = 1;

    // debitar da conta
    const char *sql_conta = "UPDATE accounts SET balance = balance - ? WHERE id = ? AND user_id = ?;";
    if (sqlite3_prepare_v2(db, sql_conta, -1, &stmt, NULL) != SQLITE_OK) {
        ok = 0;
    } else {
        sqlite3_bind_double(stmt, 1, amount);
        sqlite3_bind_int(stmt, 2, account_id);
        sqlite3_bind_int(stmt, 3, user_id);
        if (sqlite3_step(stmt) != SQLITE_DONE) ok = 0;
        sqlite3_finalize(stmt);
    }

    // creditar na meta
    if (ok) {
        const char *sql_goal = "UPDATE goals SET current_amount = current_amount + ? WHERE id = ? AND user_id = ?;";
        if (sqlite3_prepare_v2(db, sql_goal, -1, &stmt, NULL) != SQLITE_OK) {
            ok = 0;
        } else {
            sqlite3_bind_double(stmt, 1, amount);
            sqlite3_bind_int(stmt, 2, goal_id);
            sqlite3_bind_int(stmt, 3, user_id);
            if (sqlite3_step(stmt) != SQLITE_DONE) ok = 0;
            sqlite3_finalize(stmt);
        }
    }

    if (ok) {
        sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
        ui_success("Valor alocado com sucesso!");

        // verificar se meta foi atingida
        if (current + amount >= target) {
            printf("[PARABENS] Meta \"%s\" atingida!\n", "");
        }
    } else {
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        ui_error("Erro ao alocar valor. Operacao cancelada.");
    }
}

void excluir_meta(int id, int user_id) {
    if (!validate_user_goal(user_id, id)) return;

    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "DELETE FROM goals WHERE id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar DELETE.");
        return;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao excluir meta.");
    } else {
        if (sqlite3_changes(db) > 0)
            ui_success("Meta excluida com sucesso!");
        else
            ui_error("Meta nao encontrada.");
    }

    sqlite3_finalize(stmt);
}

int validate_user_goal(int user_id, int goal_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "SELECT user_id FROM goals WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao validar meta.");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, goal_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int owner = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (owner != user_id) {
            ui_error("Acesso negado a esta meta.");
            return 0;
        }
        return 1;
    }

    sqlite3_finalize(stmt);
    ui_error("Meta nao encontrada.");
    return 0;
}
