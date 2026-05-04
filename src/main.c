#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "session.h"
#include "ui.h"
#include "Categories.h"
#include "Bills.h"
#include "auth.h"


#define DB_PATH "db/moneyflow.db"

static void menu_principal(void);
static void menu_inicial(void);

int main(void) {
    if (!db_open(DB_PATH)) {
        return EXIT_FAILURE;
    }
    db_init();

    menu_inicial();

    db_close();
    return EXIT_SUCCESS;
}

static void menu_inicial(void) {
    int opcao;
    do {
        ui_clear();
        ui_header("MoneyFlow");
        printf("  1. Login\n");
        printf("  2. Cadastrar conta\n");
        printf("  0. Sair\n");
        ui_divider();
        opcao = ui_read_int("Opcao: ");

        switch (opcao) {
            case 1:
                auth_login();
                if (session.logged_in) menu_principal();
                break;
            case 2:
                auth_cadastro();
                break;
            case 0:
                printf("Ate logo!\n");
                break;
            default:
                ui_error("Opcao invalida.");
        }
    } while (opcao != 0);
}

static void menu_principal(void) {
    int opcao;
    do {
        ui_clear();
        ui_header("Menu Principal");
        printf("  Logado como: %s\n\n", session.username);
        printf("  1. Visao geral\n");
        printf("  2. Contas\n");
        printf("  3. Categorias\n");
        printf("  4. Movimentacoes\n");
        printf("  5. Cartoes de credito\n");
        printf("  6. Metas\n");
        printf("  7. Contas a pagar\n");
        printf("  8. Emprestimos\n");
        printf("  0. Logout\n");
        ui_divider();
        opcao = ui_read_int("Opcao: ");

        if (!session_require() && opcao != 0) continue;

        switch (opcao) {
            case 1: /* dashboard(); */   break;
            case 2: /* menu_contas(); */ break;
            case 3:  menu_categorias();  break;
            case 4: /* menu_transacoes(); */ break;
            case 5: /* menu_cartoes(); */ break;
            case 6: /* menu_metas(); */  break;
            case 7:  menu_bills();   break;
            case 8: /* menu_loans(); */  break;
            case 0:
                session_end();
                printf("Logout realizado.\n");
                break;
            default:
                ui_error("Opcao invalida.");
        }
    } while (opcao != 0 && session.logged_in);
}
