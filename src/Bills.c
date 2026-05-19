#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ui.h"
#include "../include/Bills.h"
#include "../include/Accounts.h"
#include "session.h"

void menu_bills() {
    int opcao;

    do {
        printf("\n=== MENU DE CONTAS A PAGAR ===\n");
        printf("1. Listar Contas a Pagar\n");
        printf("2. Adicionar conta a pagar\n");
        printf("3. Editar conta a pagar\n");
        printf("4. Excluir conta a pagar\n");
        printf("5. Baixar conta (marcar como paga)\n");
        printf("0. Voltar\n");

        opcao = ui_read_int("Opcao: ");

        switch (opcao) {
            case 1:
                listar_bills(session.user_id);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;

            case 2: {
                char description[100];
                printf("Digite a descricao da nova conta a pagar: ");
                fgets(description, sizeof(description), stdin);
                description[strcspn(description, "\n")] = 0;

                double amount;
                printf("Digite o valor da nova conta a pagar: ");
                scanf("%lf", &amount);

                int due_day;
                printf("Digite o dia de vencimento (1 a 31): ");
                scanf("%d", &due_day);
                while (getchar() != '\n');

                if (!validate_due_day(due_day)) {
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }

                adicionar_bills(session.user_id, description, amount, due_day);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 3: {
                listar_bills(session.user_id);
                int id = ui_read_int("Digite o ID da conta a pagar: ");

                char description[100];
                printf("Digite a nova descricao: ");
                fgets(description, sizeof(description), stdin);
                description[strcspn(description, "\n")] = 0;

                double amount;
                printf("Digite o novo valor: ");
                scanf("%lf", &amount);

                int due_day;
                printf("Digite o novo dia de vencimento (1 a 31): ");
                scanf("%d", &due_day);
                while (getchar() != '\n');

                if (!validate_due_day(due_day)) {
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }

                editar_bills(id, description, amount, due_day);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 4: {
                listar_bills(session.user_id);
                int id = ui_read_int("Digite o ID da conta a pagar: ");
                excluir_bills(id);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 5:
                baixar_bill(session.user_id);
                break;

            case 0:
                printf("Voltando...\n");
                return;

            default:
                ui_error("Opcao invalida.");
                printf("\nPressione Enter para continuar...");
                getchar();
        }

    } while (opcao != 0);
}

int validate_due_day(int due_day) {
    if (due_day < 1 || due_day > 31) {
        ui_error("Dia de vencimento deve ser entre 1 e 31.");
        return 0;
    }
    return 1;
}

int validade_user_bills(int user_id, int bills_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT user_id FROM bills WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao validar acesso do usuario.");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, bills_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int user_id_curr = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (user_id_curr != user_id) {
            ui_error("Usuario nao tem acesso a este cadastro.");
            return 0;
        }
    } else {
        sqlite3_finalize(stmt);
        ui_error("Conta a pagar nao encontrada.");
        return 0;
    }

    return 1;
}

void listar_bills(int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "SELECT id, description, amount, due_day, paid FROM bills WHERE user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar contas a pagar.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    printf("\n=== CONTAS A PAGAR ===\n");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *description = sqlite3_column_text(stmt, 1);
        double amount = sqlite3_column_double(stmt, 2);
        int due_day = sqlite3_column_int(stmt, 3);
        int paid = sqlite3_column_int(stmt, 4);

        printf("%d - %s - R$ %.2f - Vencimento: %d - Pago: %s\n",
               id, description, amount, due_day, paid ? "Sim" : "Nao");
    }

    sqlite3_finalize(stmt);
}

void adicionar_bills(int user_id, const char *description, double amount, int due_day) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql =
        "INSERT INTO bills (user_id, description, amount, due_day, paid) VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Erro ao preparar INSERT. %s\n", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, description, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, amount);
    sqlite3_bind_int(stmt, 4, due_day);
    sqlite3_bind_int(stmt, 5, 0);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Erro ao adicionar conta a pagar. %s\n", sqlite3_errmsg(db));
    } else {
        printf("Conta a pagar adicionada com sucesso!\n");
    }

    sqlite3_finalize(stmt);
}

void editar_bills(int id, const char *description, double amount, int due_day) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    if (!validade_user_bills(session.user_id, id)) return;

    const char *sql =
        "UPDATE bills SET description = ?, amount = ?, due_day = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar UPDATE.");
        return;
    }

    sqlite3_bind_text(stmt, 1, description, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, amount);
    sqlite3_bind_int(stmt, 3, due_day);
    sqlite3_bind_int(stmt, 4, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Erro ao editar conta a pagar. %s\n", sqlite3_errmsg(db));
    } else {
        if (sqlite3_changes(db) > 0)
            printf("Conta a pagar editada com sucesso!\n");
        else
            printf("Nenhuma conta a pagar encontrada com ID %d\n", id);
    }

    sqlite3_finalize(stmt);
}

void excluir_bills(int id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    if (!validade_user_bills(session.user_id, id)) return;

    /* Impede exclusão de conta já baixada (UC10) */
    const char *sql_check = "SELECT paid FROM bills WHERE id = ?;";
    sqlite3_prepare_v2(db, sql_check, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 1) {
        sqlite3_finalize(stmt);
        ui_error("Nao e possivel excluir uma conta ja baixada.");
        return;
    }
    sqlite3_finalize(stmt);

    const char *sql = "DELETE FROM bills WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar DELETE.");
        return;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao excluir conta a pagar.");
    } else {
        if (sqlite3_changes(db) > 0)
            printf("Conta a pagar excluida com sucesso!\n");
        else
            printf("Nenhuma conta a pagar encontrada com ID %d\n", id);
    }

    sqlite3_finalize(stmt);
}

/* ──────────────────────── BAIXAR CONTA ────────────────────────── */

void baixar_bill(int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    ui_clear();
    printf("=== BAIXAR CONTA A PAGAR ===\n");

    /* Lista apenas contas pendentes */
    const char *sql_list =
        "SELECT id, description, amount, due_day FROM bills "
        "WHERE user_id = ? AND paid = 0 "
        "ORDER BY due_day ASC;";

    if (sqlite3_prepare_v2(db, sql_list, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar contas pendentes.");
        return;
    }
    sqlite3_bind_int(stmt, 1, user_id);

    printf("\n%-4s %-30s %-12s %s\n", "ID", "Descricao", "Valor", "Vencimento");
    printf("------------------------------------------------------------\n");

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int         id      = sqlite3_column_int(stmt, 0);
        const char *desc    = (const char *)sqlite3_column_text(stmt, 1);
        double      amount  = sqlite3_column_double(stmt, 2);
        int         due_day = sqlite3_column_int(stmt, 3);
        printf("%-4d %-30s R$ %-9.2f Dia %02d\n", id, desc, amount, due_day);
        found = 1;
    }
    sqlite3_finalize(stmt);

    if (!found) {
        printf("  Nenhuma conta pendente no momento.\n");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    printf("------------------------------------------------------------\n");

    int bill_id = ui_read_int("\nID da conta a baixar: ");

    /* Valida posse e busca dados da bill */
    if (!validade_user_bills(user_id, bill_id)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    /* Verifica se já está paga */
    const char *sql_status =
        "SELECT paid, description, amount FROM bills WHERE id = ?;";
    sqlite3_prepare_v2(db, sql_status, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, bill_id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        ui_error("Conta nao encontrada.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    int    already_paid  = sqlite3_column_int(stmt, 0);
    char   bill_desc[100];
    strncpy(bill_desc, (const char *)sqlite3_column_text(stmt, 1), sizeof(bill_desc) - 1);
    bill_desc[sizeof(bill_desc) - 1] = '\0';
    double bill_amount = sqlite3_column_double(stmt, 2);
    sqlite3_finalize(stmt);

    if (already_paid) {
        ui_error("Esta conta ja foi baixada anteriormente.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    /* Escolhe conta financeira para débito */
    printf("\n");
    listar_contas(user_id);
    int account_id = ui_read_int("\nID da conta financeira para debito: ");

    if (!validate_user_account(user_id, account_id)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    /* Confirmação */
    printf("\nConfirmar baixa?\n");
    printf("  Conta a pagar : %s\n", bill_desc);
    printf("  Valor         : R$ %.2f\n", bill_amount);
    printf("  Debitar de    : conta ID %d\n", account_id);
    printf("  1. Confirmar  0. Cancelar\n");
    int confirm = ui_read_int("Opcao: ");
    if (confirm != 1) {
        printf("Operacao cancelada.\n");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    /* Monta a descrição da transação */
    char trans_desc[120];
    snprintf(trans_desc, sizeof(trans_desc), "Pagamento: %s", bill_desc);

    /* Obtém a data atual no formato DD/MM/AAAA */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char today[11];
    strftime(today, sizeof(today), "%d/%m/%Y", t);

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    /* 1. Insere a transação de despesa */
    const char *sql_trans =
        "INSERT INTO transactions "
        "(user_id, account_id, type, amount, date, description) "
        "VALUES (?, ?, 'expense', ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql_trans, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        ui_error("Erro ao preparar lancamento da despesa.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, account_id);
    sqlite3_bind_double(stmt, 3, bill_amount);
    sqlite3_bind_text(stmt, 4, today, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, trans_desc, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        ui_error("Erro ao registrar despesa.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    int new_trans_id = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    /* 2. Marca a bill como paga e vincula o transaction_id */
    const char *sql_pay =
        "UPDATE bills SET paid = 1, transaction_id = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql_pay, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        ui_error("Erro ao atualizar status da conta.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3_bind_int(stmt, 1, new_trans_id);
    sqlite3_bind_int(stmt, 2, bill_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        ui_error("Erro ao marcar conta como paga.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }
    sqlite3_finalize(stmt);

    /* 3. Debita o saldo da conta financeira */
    const char *sql_saldo =
        "UPDATE accounts SET balance = balance - ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql_saldo, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        ui_error("Erro ao atualizar saldo da conta.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3_bind_double(stmt, 1, bill_amount);
    sqlite3_bind_int(stmt, 2, account_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    ui_success("Conta baixada com sucesso! Despesa registrada automaticamente.");
    printf("\nPressione Enter para continuar...");
    getchar();
}