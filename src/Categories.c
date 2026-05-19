#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "session.h"
#include "../include/Categories.h"

void menu_categorias() {
    int opcao;

    do {
        printf("\n=== MENU DE CATEGORIAS ===\n");
        printf("1. Listar categorias\n");
        printf("2. Adicionar categoria\n");
        printf("3. Editar categoria\n");
        printf("4. Excluir categoria\n");
        printf("0. Voltar\n");

        opcao = ui_read_int("Opcao: ");

        switch (opcao) {
            case 1:
                listar_categorias(session.user_id);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;

            case 2: {
                char name[100];
                printf("Digite o nome da nova categoria: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;
                adicionar_categoria(session.user_id, name);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 3: {
                listar_categorias(session.user_id);
                int id = ui_read_int("Digite o ID da categoria: ");
                char new_name[100];
                printf("Digite o novo nome: ");
                fgets(new_name, sizeof(new_name), stdin);
                new_name[strcspn(new_name, "\n")] = 0;
                editar_categoria(id, new_name);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

            case 4: {
                listar_categorias(session.user_id);
                int id = ui_read_int("Digite o ID da categoria: ");
                excluir_categoria(id);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
            }

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

void listar_categorias(int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "SELECT id, name FROM categories WHERE user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar categorias.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    printf("\n=== CATEGORIAS ===\n");

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        printf("%d - %s\n", id, name);
        found = 1;
    }

    if (!found)
        printf("  Nenhuma categoria cadastrada.\n");

    sqlite3_finalize(stmt);
}

void adicionar_categoria(int user_id, const char *name) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "INSERT INTO categories (user_id, name) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar INSERT.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Erro ao adicionar categoria. %s\n", sqlite3_errmsg(db));
    } else {
        printf("Categoria adicionada com sucesso!\n");
    }

    sqlite3_finalize(stmt);
}

int validade_user_category(int user_id, int category_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT user_id FROM categories WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao validar acesso do usuario.");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, category_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int user_id_curr = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (user_id_curr != user_id) {
            ui_error("Usuario nao tem acesso a esta categoria.");
            return 0;
        }
        return 1;
    }

    sqlite3_finalize(stmt);
    ui_error("Categoria nao encontrada.");
    return 0;
}

void editar_categoria(int id, const char *name) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    if (!validade_user_category(session.user_id, id)) return;

    const char *sql = "UPDATE categories SET name = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar UPDATE.");
        return;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao editar categoria.");
    } else {
        if (sqlite3_changes(db) > 0)
            printf("Categoria editada com sucesso!\n");
        else
            printf("Nenhuma categoria encontrada com ID %d\n", id);
    }

    sqlite3_finalize(stmt);
}

void excluir_categoria(int id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    if (!validade_user_category(session.user_id, id)) return;

    const char *sql = "DELETE FROM categories WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar DELETE.");
        return;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao excluir categoria.");
    } else {
        if (sqlite3_changes(db) > 0)
            printf("Categoria excluida com sucesso!\n");
        else
            printf("Nenhuma categoria encontrada com ID %d\n", id);
    }

    sqlite3_finalize(stmt);
}