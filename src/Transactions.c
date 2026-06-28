#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "session.h"
#include "ui.h"
#include "Validation.h"
#include "Accounts.h"
#include "Categories.h"
#include "../include/Transactions.h"
#include "CreditCards.h"

/* Atualiza o saldo de uma conta no banco. delta positivo = crédito, negativo = débito. */
static int atualizar_saldo(int account_id, double delta) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE accounts SET balance = balance + ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro interno ao atualizar saldo.");
        return 0;
    }
    sqlite3_bind_double(stmt, 1, delta);
    sqlite3_bind_int(stmt, 2, account_id);
    int ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

/* Busca dados de uma transação garantindo que pertence ao usuário logado. */
static int buscar_transacao(int t_id, int *account_id, char *type,
                             double *amount, int *transfer_id, int *credit_card_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT account_id, type, amount, COALESCE(transfer_id, 0), COALESCE(credit_card_id, 0) "
        "FROM transactions WHERE id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao buscar movimentacao.");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, t_id);
    sqlite3_bind_int(stmt, 2, session.user_id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        ui_error("Movimentacao nao encontrada ou sem permissao de acesso.");
        return 0;
    }

    *account_id  = sqlite3_column_int(stmt, 0);
    strncpy(type, (const char *)sqlite3_column_text(stmt, 1), 15);
    type[15]     = '\0';
    *amount      = sqlite3_column_double(stmt, 2);
    *transfer_id = sqlite3_column_int(stmt, 3);
    *credit_card_id = sqlite3_column_int(stmt, 4);
    sqlite3_finalize(stmt);
    return 1;
}

/* ─────────────────── LISTAR MOVIMENTAÇÕES DO USUÁRIO ─────────── */

static void listar_transacoes_usuario(void) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT t.id, t.type, t.amount, t.date, t.description, "
        "       COALESCE(c.name, '-') AS categoria "
        "FROM transactions t "
        "LEFT JOIN categories c ON t.category_id = c.id "
        "WHERE t.user_id = ? "
        "ORDER BY t.date DESC, t.id DESC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar movimentacoes.");
        return;
    }
    sqlite3_bind_int(stmt, 1, session.user_id);

    printf("\n%-4s %-14s %-12s %-12s %-22s %s\n",
           "ID", "Tipo", "Valor", "Data", "Descricao", "Categoria");
    ui_divider();

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int         id  = sqlite3_column_int(stmt, 0);
        const char *tp  = (const char *)sqlite3_column_text(stmt, 1);
        double      amt = sqlite3_column_double(stmt, 2);
        const char *dt  = (const char *)sqlite3_column_text(stmt, 3);
        const char *dsc = (const char *)sqlite3_column_text(stmt, 4);
        const char *cat = (const char *)sqlite3_column_text(stmt, 5);

        const char *label =
            strcmp(tp, "income")  == 0 ? "Receita"       :
            strcmp(tp, "expense") == 0 ? "Despesa"       : "Transferencia";

        printf("%-4d %-14s R$ %-9.2f %-12s %-22s %s\n",
               id, label, amt, dt, dsc, cat);
        found = 1;
    }
    sqlite3_finalize(stmt);

    if (!found)
        printf("  Nenhuma movimentacao cadastrada.\n");

    ui_divider();
}

/* ───────────────────────────── MENU ───────────────────────────── */

void menu_transacoes(void) {
    int opcao;
    do {
        ui_clear();
        ui_header("Movimentacoes");
        printf("  1. Registrar receita ou despesa\n");
        printf("  2. Registrar transferencia entre contas\n");
        printf("  3. Editar movimentacao\n");
        printf("  4. Excluir movimentacao\n");
        printf("  5. Filtrar historico\n");
        printf("  0. Voltar\n");
        ui_divider();
        opcao = ui_read_int("Opcao: ");

        switch (opcao) {
            case 1: registrar_receita_despesa(); break;
            case 2: registrar_transferencia();   break;
            case 3: editar_movimentacao();       break;
            case 4: excluir_movimentacao();      break;
            case 5: filtrar_historico();         break;
            case 0: break;
            default:
                ui_error("Opcao invalida.");
                printf("\nPressione Enter para continuar...");
                getchar();
        }
    } while (opcao != 0);
}

/* ─────────────────── REGISTRAR RECEITA / DESPESA ─────────────── */

void registrar_receita_despesa(void) {
    ui_clear();
    ui_header("Nova Movimentacao");

    printf("  1. Receita\n");
    printf("  2. Despesa\n");
    ui_divider();
    int tipo_escolha = ui_read_int("Tipo: ");

    const char *type;
    if      (tipo_escolha == 1) type = "income";
    else if (tipo_escolha == 2) type = "expense";
    else {
        ui_error("Tipo invalido.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    double amount = ui_read_double("Valor: R$ ");
    if (!validate_amount(amount)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    char date[11];
    ui_read_str("Data (DD/MM/AAAA): ", date, sizeof(date));
    if (!validate_date(date)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    char description[100];
    ui_read_str("Descricao: ", description, sizeof(description));
    if (strlen(description) == 0) {
        ui_error("Descricao nao pode ser vazia.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    listar_contas(session.user_id);
    int account_id = ui_read_int("\nID da conta: ");
    if (!validate_user_account(session.user_id, account_id)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    listar_categorias(session.user_id);
    int category_id = ui_read_int("\nID da categoria: ");
    if (!validade_user_category(session.user_id, category_id)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    int credit_card_id = 0;
    if (strcmp(type, "expense") == 0) {
        printf("\nDeseja vincular esta despesa a um cartao de credito?\n");
        printf("  1. Sim\n");
        printf("  0. Nao\n");
        int vincular = ui_read_int("Opcao: ");
        if (vincular == 1) {
            listar_cartoes(session.user_id);
            credit_card_id = ui_read_int("\nID do cartao de credito: ");
            if (credit_card_id > 0 && !validate_user_credit_card(session.user_id, credit_card_id)) {
                printf("\nPressione Enter para continuar...");
                getchar();
                return;
            }
        }
    }

    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;
    const char *sql =
        "INSERT INTO transactions "
        "(user_id, account_id, category_id, credit_card_id, type, amount, date, description) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar insercao.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3_bind_int(stmt, 1, session.user_id);
    sqlite3_bind_int(stmt, 2, account_id);
    sqlite3_bind_int(stmt, 3, category_id);
    if (credit_card_id > 0) {
        sqlite3_bind_int(stmt, 4, credit_card_id);
    } else {
        sqlite3_bind_null(stmt, 4);
    }
    sqlite3_bind_text(stmt, 5, type, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 6, amount);
    sqlite3_bind_text(stmt, 7, date, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, description, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        if (strcmp(type, "expense") != 0 || credit_card_id == 0) {
            double delta = (strcmp(type, "income") == 0) ? amount : -amount;
            atualizar_saldo(account_id, delta);
        }
        ui_success("Movimentacao registrada com sucesso!");
    } else {
        ui_error("Erro ao salvar movimentacao.");
    }

    sqlite3_finalize(stmt);
    printf("\nPressione Enter para continuar...");
    getchar();
}

/* ──────────────────────── TRANSFERÊNCIA ──────────────────────── */

void registrar_transferencia(void) {
    ui_clear();
    ui_header("Transferencia entre Contas");

    listar_contas(session.user_id);

    int origem_id = ui_read_int("\nID da conta de origem: ");
    if (!validate_user_account(session.user_id, origem_id)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    int destino_id = ui_read_int("ID da conta de destino: ");
    if (!validate_user_account(session.user_id, destino_id)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    if (origem_id == destino_id) {
        ui_error("Conta de origem e destino nao podem ser iguais.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    double amount = ui_read_double("Valor: R$ ");
    if (!validate_amount(amount)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    char date[11];
    ui_read_str("Data (DD/MM/AAAA): ", date, sizeof(date));
    if (!validate_date(date)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3 *db = returnConnection();
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    /* 1. Insere registro de saída na origem */
    const char *sql_saida =
        "INSERT INTO transactions "
        "(user_id, account_id, type, amount, date, description) "
        "VALUES (?, ?, 'transfer', ?, ?, 'Saida por transferencia');";
    sqlite3_stmt *stmt1;
    sqlite3_prepare_v2(db, sql_saida, -1, &stmt1, NULL);
    sqlite3_bind_int(stmt1, 1, session.user_id);
    sqlite3_bind_int(stmt1, 2, origem_id);
    sqlite3_bind_double(stmt1, 3, amount);
    sqlite3_bind_text(stmt1, 4, date, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt1) != SQLITE_DONE) {
        sqlite3_finalize(stmt1);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        ui_error("Erro ao registrar saida da transferencia.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    /* Usa o rowid do primeiro insert como transfer_id — vincula os dois registros */
    int transfer_id = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt1);

    /* Atualiza o próprio registro de saída com o transfer_id */
    sqlite3_stmt *stmt_tid;
    sqlite3_prepare_v2(db,
        "UPDATE transactions SET transfer_id = ? WHERE id = ?;",
        -1, &stmt_tid, NULL);
    sqlite3_bind_int(stmt_tid, 1, transfer_id);
    sqlite3_bind_int(stmt_tid, 2, transfer_id);
    sqlite3_step(stmt_tid);
    sqlite3_finalize(stmt_tid);

    /* 2. Insere registro de entrada no destino com o mesmo transfer_id */
    const char *sql_entrada =
        "INSERT INTO transactions "
        "(user_id, account_id, type, amount, date, description, transfer_id) "
        "VALUES (?, ?, 'transfer', ?, ?, 'Entrada por transferencia', ?);";
    sqlite3_stmt *stmt2;
    sqlite3_prepare_v2(db, sql_entrada, -1, &stmt2, NULL);
    sqlite3_bind_int(stmt2, 1, session.user_id);
    sqlite3_bind_int(stmt2, 2, destino_id);
    sqlite3_bind_double(stmt2, 3, amount);
    sqlite3_bind_text(stmt2, 4, date, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt2, 5, transfer_id);

    if (sqlite3_step(stmt2) != SQLITE_DONE) {
        sqlite3_finalize(stmt2);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        ui_error("Erro ao registrar entrada da transferencia.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }
    sqlite3_finalize(stmt2);

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    atualizar_saldo(origem_id, -amount);
    atualizar_saldo(destino_id,  amount);

    ui_success("Transferencia realizada com sucesso!");
    printf("\nPressione Enter para continuar...");
    getchar();
}

/* ──────────────────────── EDITAR ──────────────────────────────── */

void editar_movimentacao(void) {
    ui_clear();
    ui_header("Editar Movimentacao");

    listar_transacoes_usuario();
    int t_id = ui_read_int("ID da movimentacao: ");
    int account_id, transfer_id, credit_card_id;
    double amount;
    char type[16] = {0};

    if (!buscar_transacao(t_id, &account_id, type, &amount, &transfer_id, &credit_card_id)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    if (strcmp(type, "transfer") == 0) {
        ui_error("Transferencias nao podem ser editadas. Exclua e registre novamente.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    double novo_valor = ui_read_double("Novo valor: R$ ");
    if (!validate_amount(novo_valor)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    char nova_desc[100];
    ui_read_str("Nova descricao: ", nova_desc, sizeof(nova_desc));
    if (strlen(nova_desc) == 0) {
        ui_error("Descricao nao pode ser vazia.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    char nova_data[11];
    ui_read_str("Nova data (DD/MM/AAAA): ", nova_data, sizeof(nova_data));
    if (!validate_date(nova_data)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;
    const char *sql =
        "UPDATE transactions SET amount = ?, description = ?, date = ? "
        "WHERE id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar edicao.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3_bind_double(stmt, 1, novo_valor);
    sqlite3_bind_text(stmt, 2, nova_desc, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, nova_data, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, t_id);
    sqlite3_bind_int(stmt, 5, session.user_id);

    if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) > 0) {
        /* Reverte o efeito antigo e aplica o novo valor no saldo se nao for cartao de credito */
        if (strcmp(type, "expense") != 0 || credit_card_id == 0) {
            double delta = (strcmp(type, "income") == 0)
                ? (novo_valor - amount)
                : (amount - novo_valor);
            atualizar_saldo(account_id, delta);
        }
        ui_success("Movimentacao editada com sucesso!");
    } else {
        ui_error("Erro ao editar movimentacao.");
    }

    sqlite3_finalize(stmt);
    printf("\nPressione Enter para continuar...");
    getchar();
}

/* ──────────────────────── EXCLUIR ─────────────────────────────── */

void excluir_movimentacao(void) {
    ui_clear();
    ui_header("Excluir Movimentacao");

    listar_transacoes_usuario();
    int t_id = ui_read_int("ID da movimentacao: ");
    int account_id, transfer_id, credit_card_id;
    double amount;
    char type[16] = {0};

    if (!buscar_transacao(t_id, &account_id, type, &amount, &transfer_id, &credit_card_id)) {
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3 *db = returnConnection();

    if (strcmp(type, "transfer") == 0 && transfer_id > 0) {
        /* Busca as duas contas da transferência para reverter ambos os saldos */
        const char *sql_par =
            "SELECT account_id, description FROM transactions "
            "WHERE transfer_id = ? AND user_id = ?;";
        sqlite3_stmt *stmt_par;
        sqlite3_prepare_v2(db, sql_par, -1, &stmt_par, NULL);
        sqlite3_bind_int(stmt_par, 1, transfer_id);
        sqlite3_bind_int(stmt_par, 2, session.user_id);

        int acc_saida = -1, acc_entrada = -1;
        while (sqlite3_step(stmt_par) == SQLITE_ROW) {
            int acc        = sqlite3_column_int(stmt_par, 0);
            const char *dc = (const char *)sqlite3_column_text(stmt_par, 1);
            if (strstr(dc, "Saida") != NULL) acc_saida   = acc;
            else                              acc_entrada = acc;
        }
        sqlite3_finalize(stmt_par);

        sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

        sqlite3_stmt *stmt_del;
        sqlite3_prepare_v2(db,
            "DELETE FROM transactions WHERE transfer_id = ? AND user_id = ?;",
            -1, &stmt_del, NULL);
        sqlite3_bind_int(stmt_del, 1, transfer_id);
        sqlite3_bind_int(stmt_del, 2, session.user_id);
        sqlite3_step(stmt_del);
        sqlite3_finalize(stmt_del);

        sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

        /* Reverte os saldos: origem recebe de volta, destino devolve */
        if (acc_saida   != -1) atualizar_saldo(acc_saida,    amount);
        if (acc_entrada != -1) atualizar_saldo(acc_entrada, -amount);

    } else {
        /* Exclusão simples de receita ou despesa */
        sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

        sqlite3_stmt *stmt_del;
        sqlite3_prepare_v2(db,
            "DELETE FROM transactions WHERE id = ? AND user_id = ?;",
            -1, &stmt_del, NULL);
        sqlite3_bind_int(stmt_del, 1, t_id);
        sqlite3_bind_int(stmt_del, 2, session.user_id);
        sqlite3_step(stmt_del);
        sqlite3_finalize(stmt_del);

        sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

        /* Reverte o efeito no saldo se nao for despesa no cartao de credito */
        if (strcmp(type, "expense") != 0 || credit_card_id == 0) {
            double estorno = (strcmp(type, "income") == 0) ? -amount : amount;
            atualizar_saldo(account_id, estorno);
        }
    }

    ui_success("Movimentacao excluida com sucesso!");
    printf("\nPressione Enter para continuar...");
    getchar();
}

/* ──────────────────────── FILTRAR HISTÓRICO ───────────────────── */

void filtrar_historico(void) {
    ui_clear();
    ui_header("Historico de Movimentacoes");

    char filtro_data[8] = {0};
    ui_read_str("Mes/Ano (MM/AAAA) ou Enter para todos: ", filtro_data, sizeof(filtro_data));

    listar_categorias(session.user_id);
    int filtro_cat = ui_read_int("\nID da categoria (0 para todas): ");

    /* Monta o padrão LIKE: "DD/MM/AAAA" — busca pelo sufixo "/MM/AAAA" */
    char like_data[12] = {0};
    if (strlen(filtro_data) > 0)
        snprintf(like_data, sizeof(like_data), "%%/%s", filtro_data);

    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    /* Usa parâmetros para evitar SQL injection e tratar filtros opcionais */
    const char *sql =
        "SELECT t.id, t.type, t.amount, t.date, t.description, "
        "       COALESCE(c.name, '-') AS categoria "
        "FROM transactions t "
        "LEFT JOIN categories c ON t.category_id = c.id "
        "WHERE t.user_id = ? "
        "  AND (? = '' OR t.date LIKE ?) "
        "  AND (? = 0  OR t.category_id = ?) "
        "ORDER BY t.date DESC, t.id DESC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar consulta.");
        printf("\nPressione Enter para continuar...");
        getchar();
        return;
    }

    sqlite3_bind_int(stmt, 1, session.user_id);
    sqlite3_bind_text(stmt, 2, filtro_data, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, like_data,   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, filtro_cat);
    sqlite3_bind_int(stmt, 5, filtro_cat);

    printf("\n%-4s %-14s %-12s %-12s %-22s %s\n",
           "ID", "Tipo", "Valor", "Data", "Descricao", "Categoria");
    ui_divider();

    int found = 0;
    double total_receitas = 0.0, total_despesas = 0.0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int         id  = sqlite3_column_int(stmt, 0);
        const char *tp  = (const char *)sqlite3_column_text(stmt, 1);
        double      amt = sqlite3_column_double(stmt, 2);
        const char *dt  = (const char *)sqlite3_column_text(stmt, 3);
        const char *dsc = (const char *)sqlite3_column_text(stmt, 4);
        const char *cat = (const char *)sqlite3_column_text(stmt, 5);

        const char *label =
            strcmp(tp, "income")   == 0 ? "Receita"       :
            strcmp(tp, "expense")  == 0 ? "Despesa"       : "Transferencia";

        printf("%-4d %-14s R$ %-9.2f %-12s %-22s %s\n",
               id, label, amt, dt, dsc, cat);

        if (strcmp(tp, "income")  == 0) total_receitas += amt;
        if (strcmp(tp, "expense") == 0) total_despesas += amt;
        found = 1;
    }

    sqlite3_finalize(stmt);

    if (!found) {
        printf("  Nenhuma movimentacao encontrada para os filtros informados.\n");
    } else {
        ui_divider();
        printf("  Total Receitas : R$ %.2f\n", total_receitas);
        printf("  Total Despesas : R$ %.2f\n", total_despesas);
        printf("  Balanco Liquido: R$ %.2f\n", total_receitas - total_despesas);
    }

    printf("\nPressione Enter para continuar...");
    getchar();
}