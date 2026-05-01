#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "../include/Bills.h"

void menu_bills() {
    int opcao;

    do {
        printf("\n=== MENU DE CONTAS A PAGAR ===\n");
        printf("1. Listar Contas a Pagar\n");
        printf("2. Adicionar conta a pagar\n");
        printf("3. Editar conta a pagar\n");
        printf("4. Excluir conta a pagar\n");
        printf("0. Voltar\n");

        opcao = ui_read_int("Opcao: ");

        switch (opcao) {
            case 1:
                listar_bills();
                break;

            case 2: {
                char description[100];
                printf("Digite a descrição da nova conta a pagar: ");
                fgets(description, sizeof(description), stdin);
                description[strcspn(description, "\n")] = 0;

                double amount;
                printf("Digite o valor da nova conta a pagar: ");
                scanf("%lf", &amount);

                int due_day;
                printf("Digite o dia de vencimento digite numeros entre 1 e 31: ");
                scanf("%d", &due_day);

                if (!validate_due_day(due_day)) {
                    break;
                }

                adicionar_bills(1, description, amount, due_day);
                break;
            }

            case 3: {
                listar_bills();
                int id;
                char new_name[100];

                id = ui_read_int("Digite o ID da conta a pagar: ");

                char description[100];
                printf("Digite a descrição da nova conta a pagar: ");
                fgets(description, sizeof(description), stdin);
                description[strcspn(description, "\n")] = 0;

                double amount;
                printf("Digite o valor da nova conta a pagar: ");
                scanf("%lf", &amount);

                int due_day;
                printf("Digite o dia de vencimento digite numeros entre 1 e 31: ");
                scanf("%d", &due_day);

                if (!validate_due_day(due_day)) {
                    break;
                }

                editar_bills(id, new_name, description, amount, due_day);
                break;
            }

            case 4: {
                listar_bills();
                int id = ui_read_int("Digite o ID da conta a pagar: ");
                excluir_bills(id);
                break;
            }

            case 0:
                printf("Voltando...\n");
                return;

            default:
                ui_error("Opcao invalida.");
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

void listar_bills() {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "SELECT id, description, amount, due_day, paid FROM bills;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar contas a pagar.");
        return;
    }

    printf("\n=== CONTAS A PAGAR ===\n");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);        
        const unsigned char *description = sqlite3_column_text(stmt, 1);
        double amount = sqlite3_column_double(stmt, 2);
        int due_day = sqlite3_column_int(stmt, 3);
        int paid = sqlite3_column_int(stmt, 4);

        printf("%d - %s - R$ %.2f - Vencimento: %d - Pago: %s\n", id, description, amount, due_day, paid ? "Sim" : "Nao");
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
    sqlite3_bind_int(stmt, 5, 0); // paid = 0 (not paid)

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Erro ao adicionar conta a pagar. %s\n", sqlite3_errmsg(db));
    } else {
        printf("Conta a pagar adicionada com sucesso!\n");
    }

    sqlite3_finalize(stmt);
}

void editar_bills(int id, const char *name, const char *description, double amount, int due_day) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

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

    const char *sql =
        "DELETE FROM bills WHERE id = ?;";

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