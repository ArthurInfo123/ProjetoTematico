#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "db.h"
#include "session.h"
#include "ui.h"
#include "../include/CreditCards.h"

void menu_cartoes(void) {
    int opcao;

    do {
        ui_clear();
        ui_header("Cartoes de Credito");
        printf("  1. Listar cartoes\n");
        printf("  2. Adicionar cartao\n");
        printf("  3. Editar cartao\n");
        printf("  4. Excluir cartao\n");
        printf("  0. Voltar\n");
        ui_divider();
        opcao = ui_read_int("Opcao: ");

        switch (opcao) {
            case 1:
                listar_cartoes(session.user_id);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;

            case 2: {
                char name[100];
                ui_clear();
                ui_header("Novo Cartao de Credito");
                ui_read_str("Nome do cartao: ", name, sizeof(name));
                if (strlen(name) == 0) {
                    ui_error("Nome nao pode ser vazio.");
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                double limit = ui_read_double("Limite total: R$ ");
                if (limit < 0) {
                    ui_error("Limite nao pode ser negativo.");
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                int due_day = ui_read_int("Dia de vencimento (1 a 28): ");
                if (due_day < 1 || due_day > 28) {
                    ui_error("Dia de vencimento deve ser entre 1 e 28.");
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                adicionar_cartao(session.user_id, name, limit, due_day);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 3: {
                ui_clear();
                ui_header("Editar Cartao");
                listar_cartoes(session.user_id);
                int id = ui_read_int("\nID do cartao a editar: ");
                if (!validate_user_credit_card(session.user_id, id)) {
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
                double limit = ui_read_double("Novo limite: R$ ");
                if (limit < 0) {
                    ui_error("Limite nao pode ser negativo.");
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                int due_day = ui_read_int("Novo dia de vencimento (1 a 28): ");
                if (due_day < 1 || due_day > 28) {
                    ui_error("Dia de vencimento deve ser entre 1 e 28.");
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                editar_cartao(id, session.user_id, name, limit, due_day);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 4: {
                ui_clear();
                ui_header("Excluir Cartao");
                listar_cartoes(session.user_id);
                int id = ui_read_int("\nID do cartao a excluir: ");
                if (!validate_user_credit_card(session.user_id, id)) {
                    printf("\nPressione Enter para continuar...");
                    getchar();
                    break;
                }
                excluir_cartao(id, session.user_id);
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

void listar_cartoes(int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    time_t now = time(NULL);
    struct tm *tm_ = localtime(&now);
    char mes_ano[8], like_pat[14];
    strftime(mes_ano, sizeof(mes_ano), "%m/%Y", tm_);
    snprintf(like_pat, sizeof(like_pat), "%%/%s", mes_ano);

    const char *sql =
        "SELECT c.id, c.name, c.\"limit\", c.due_day, "
        "       (SELECT COALESCE(SUM(t.amount), 0.0) FROM transactions t "
        "        WHERE t.credit_card_id = c.id AND t.date LIKE ? AND t.type = 'expense') AS spent "
        "FROM credit_cards c "
        "WHERE c.user_id = ? "
        "ORDER BY c.name;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar cartoes.");
        return;
    }
    sqlite3_bind_text(stmt, 1, like_pat, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);

    printf("\n=== CARTOES DE CREDITO ===\n");
    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id       = sqlite3_column_int(stmt, 0);
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        double limit = sqlite3_column_double(stmt, 2);
        int due_day  = sqlite3_column_int(stmt, 3);
        double spent = sqlite3_column_double(stmt, 4);
        double avail = limit - spent;
        const char *alert = (avail < 0.2 * limit) ? " [!]" : "";

        printf("  [%d] %-20s Limite: R$ %8.2f | Disponivel: R$ %8.2f%s (Vencimento: dia %d)\n",
               id, name, limit, avail, alert, due_day);
        found = 1;
    }

    if (!found)
        printf("  Nenhum cartao cadastrado.\n");

    sqlite3_finalize(stmt);
}

void adicionar_cartao(int user_id, const char *name, double limit, int due_day) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "INSERT INTO credit_cards (user_id, name, \"limit\", due_day) VALUES (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar INSERT de cartao.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, limit);
    sqlite3_bind_int(stmt, 4, due_day);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao adicionar cartao.");
    } else {
        ui_success("Cartao adicionado com sucesso!");
    }

    sqlite3_finalize(stmt);
}

void editar_cartao(int id, int user_id, const char *name, double limit, int due_day) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "UPDATE credit_cards SET name = ?, \"limit\" = ?, due_day = ? WHERE id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar UPDATE de cartao.");
        return;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, due_day);
    sqlite3_bind_int(stmt, 4, id);
    sqlite3_bind_int(stmt, 5, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao editar cartao.");
    } else {
        if (sqlite3_changes(db) > 0)
            ui_success("Cartao editado com sucesso!");
        else
            ui_error("Cartao nao encontrado.");
    }

    sqlite3_finalize(stmt);
}

void excluir_cartao(int id, int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    // Verificar se tem transações vinculadas
    const char *sql_check = "SELECT COUNT(*) FROM transactions WHERE credit_card_id = ?;";
    if (sqlite3_prepare_v2(db, sql_check, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            if (count > 0) {
                ui_error("Nao e possivel excluir cartao com movimentacoes vinculadas.");
                return;
            }
        } else {
            sqlite3_finalize(stmt);
        }
    }

    const char *sql = "DELETE FROM credit_cards WHERE id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar DELETE de cartao.");
        return;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao excluir cartao.");
    } else {
        if (sqlite3_changes(db) > 0)
            ui_success("Cartao excluido com sucesso!");
        else
            ui_error("Cartao nao encontrado.");
    }

    sqlite3_finalize(stmt);
}

int validate_user_credit_card(int user_id, int credit_card_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT user_id FROM credit_cards WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao validar cartao.");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, credit_card_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int owner = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (owner != user_id) {
            ui_error("Acesso negado a este cartao.");
            return 0;
        }
        return 1;
    }

    sqlite3_finalize(stmt);
    ui_error("Cartao de credito nao encontrado.");
    return 0;
}
