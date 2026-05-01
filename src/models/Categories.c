#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
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
                listar_categorias();
                break;

            case 2: {
                char name[100];

                printf("Digite o nome da nova categoria: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;

                adicionar_categoria(1, name);
                break;
            }

            case 3: {
                listar_categorias();
                int id;
                char new_name[100];

                id = ui_read_int("Digite o ID da categoria: ");

                printf("Digite o novo nome: ");
                fgets(new_name, sizeof(new_name), stdin);
                new_name[strcspn(new_name, "\n")] = 0;

                editar_categoria(id, new_name);
                break;
            }

            case 4: {
                listar_categorias();
                int id = ui_read_int("Digite o ID da categoria: ");
                excluir_categoria(id);
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

void listar_categorias() {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "SELECT id, name FROM categories;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar categorias.");
        return;
    }

    printf("\n=== CATEGORIAS ===\n");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *name = sqlite3_column_text(stmt, 1);

        printf("%d - %s\n", id, name);
    }

    sqlite3_finalize(stmt);
}

void adicionar_categoria(int user_id, const char *name) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql =
        "INSERT INTO categories (user_id, name) VALUES (?, ?);";

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

void editar_categoria(int id, const char *name) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql =
        "UPDATE categories SET name = ? WHERE id = ?;";

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

    const char *sql =
        "DELETE FROM categories WHERE id = ?;";

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