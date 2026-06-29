#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "db.h"
#include "session.h"
#include "ui.h"
#include "Dashboard.h"
#include "CreditCards.h"

/* ================================================================
   SISTEMA DE BUFFER DE COLUNAS
   COL_W = 38 chars por coluna
   Layout: |<38>| | |<38>|  => 79 chars total (safe em 80-col)
   ================================================================ */

#define COL_W      38
#define MAX_LINES  80
#define DIV_COL    "  --------------------------------------"
#define DIV_FULL   "==============================================================================="

typedef struct {
    char lines[MAX_LINES][COL_W + 1];
    int  count;
} ColBuf;

static void col_add(ColBuf *b, const char *fmt, ...) {
    if (b->count >= MAX_LINES) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b->lines[b->count], COL_W + 1, fmt, ap);
    va_end(ap);
    b->count++;
}

static void col_render(const ColBuf *L, const ColBuf *R) {
    int rows = L->count > R->count ? L->count : R->count;
    for (int i = 0; i < rows; i++) {
        const char *l = (i < L->count) ? L->lines[i] : "";
        const char *r = (i < R->count) ? R->lines[i] : "";
        printf("%-38s | %-38s\n", l, r);
    }
}

// coluna esquerda
static double bloco_contas(ColBuf *b, int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;
    double total = 0.0;

    col_add(b, "  SALDO DAS CONTAS");
    col_add(b, DIV_COL);

    const char *sql =
        "SELECT name, balance FROM accounts WHERE user_id=? ORDER BY name;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        col_add(b, "  [ERRO] Falha ao carregar contas.");
        return 0.0;
    }
    sqlite3_bind_int(stmt, 1, user_id);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name = (const char *)sqlite3_column_text(stmt, 0);
        double bal       = sqlite3_column_double(stmt, 1);
        col_add(b, "  %-20.20s  R$ %8.2f", name, bal);
        total += bal;
        found  = 1;
    }
    sqlite3_finalize(stmt);

    if (!found) col_add(b, "  Nenhuma conta cadastrada.");

    col_add(b, "  ......................................");
    col_add(b, "  %-20s  R$ %8.2f", "TOTAL", total);
    return total;
}

// coluna esquerda
static void bloco_movimentacoes(ColBuf *b, int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    time_t now = time(NULL);
    struct tm *tm_ = localtime(&now);
    char mes_ano[8], like_pat[14];
    strftime(mes_ano,  sizeof(mes_ano),  "%m/%Y", tm_);
    snprintf(like_pat, sizeof(like_pat), "%%/%s",  mes_ano);

    col_add(b, "");
    col_add(b, "  MOVIMENTACOES  %s", mes_ano);
    col_add(b, DIV_COL);

    const char *sql =
        "SELECT"
        "  COALESCE(SUM(CASE WHEN type='income'  THEN amount ELSE 0 END),0),"
        "  COALESCE(SUM(CASE WHEN type='expense' THEN amount ELSE 0 END),0) "
        "FROM transactions "
        "WHERE user_id=? AND date LIKE ? AND type!='transfer';";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        col_add(b, "  [ERRO] Falha ao carregar movimentacoes.");
        return;
    }
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, like_pat, -1, SQLITE_TRANSIENT);

    double rec = 0.0, desp = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        rec  = sqlite3_column_double(stmt, 0);
        desp = sqlite3_column_double(stmt, 1);
    }
    sqlite3_finalize(stmt);

    double bal = rec - desp;
    col_add(b, "  Receitas       R$ %10.2f", rec);
    col_add(b, "  Despesas       R$ %10.2f", desp);
    col_add(b, "  ......................................");
    if (bal >= 0)
        col_add(b, "  Balanco   (+)  R$ %10.2f", bal);
    else
        col_add(b, "  Balanco   (-)  R$ %10.2f", bal);
}

// coluna direita
static void bloco_bills(ColBuf *b, int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    col_add(b, "  CONTAS A PAGAR");
    col_add(b, DIV_COL);

    const char *sql =
        "SELECT description, amount, due_day, paid "
        "FROM bills WHERE user_id=? "
        "ORDER BY paid ASC, due_day ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        col_add(b, "  [ERRO] Falha ao carregar bills.");
        return;
    }
    sqlite3_bind_int(stmt, 1, user_id);

    double pendente = 0.0, pago = 0.0;
    int found = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *desc = (const char *)sqlite3_column_text(stmt, 0);
        double      amt  = sqlite3_column_double(stmt, 1);
        int         day  = sqlite3_column_int(stmt, 2);
        int         paid = sqlite3_column_int(stmt, 3);

        col_add(b, "  %s d.%02d %-13.13s R$%6.2f",
                paid ? "[PAGO]" : "[PEND]", day, desc, amt);

        if (paid) pago     += amt;
        else      pendente += amt;
        found = 1;
    }
    sqlite3_finalize(stmt);

    if (!found) {
        col_add(b, "  Nenhuma conta cadastrada.");
        return;
    }
    col_add(b, "  ......................................");
    col_add(b, "  Pendente       R$ %10.2f", pendente);
    col_add(b, "  Pago           R$ %10.2f", pago);
}

// coluna direita
static void bloco_metas(ColBuf *b, int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    col_add(b, "");
    col_add(b, "  METAS FINANCEIRAS");
    col_add(b, DIV_COL);

    const char *sql =
        "SELECT name, target_amount, current_amount, deadline "
        "FROM goals WHERE user_id=? ORDER BY deadline ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        col_add(b, "  [ERRO] Falha ao carregar metas.");
        return;
    }
    sqlite3_bind_int(stmt, 1, user_id);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name    = (const char *)sqlite3_column_text(stmt, 0);
        double target        = sqlite3_column_double(stmt, 1);
        double current       = sqlite3_column_double(stmt, 2);
        const char *deadline = (const char *)sqlite3_column_text(stmt, 3);

        double pct = (target > 0) ? (current / target * 100.0) : 0.0;
        if (pct > 100.0) pct = 100.0;

        // barra de progresso visual com 10 blocos
        int blocos = (int)(pct / 10.0);
        char barra[12];
        for (int i = 0; i < 10; i++)
            barra[i] = (i < blocos) ? '#' : '-';
        barra[10] = '\0';

        col_add(b, "  %-14.14s %s %3.0f%%", name, barra, pct);
        col_add(b, "  R$%7.2f / R$%7.2f  ate %s", current, target,
                deadline ? deadline : "s/prazo");
        found = 1;
    }
    sqlite3_finalize(stmt);

    if (!found) col_add(b, "  Nenhuma meta cadastrada.");
}

// coluna direita
static void bloco_emprestimos(ColBuf *b, int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    col_add(b, "");
    col_add(b, "  EMPRESTIMOS");
    col_add(b, DIV_COL);

    const char *sql =
        "SELECT creditor, total_amount, remaining_amount "
        "FROM loans WHERE user_id=? ORDER BY remaining_amount DESC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        col_add(b, "  [ERRO] Falha ao carregar emprestimos.");
        return;
    }
    sqlite3_bind_int(stmt, 1, user_id);

    double total_restante = 0.0;
    int found = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *credor    = (const char *)sqlite3_column_text(stmt, 0);
        double total           = sqlite3_column_double(stmt, 1);
        double remaining       = sqlite3_column_double(stmt, 2);
        double pago            = total - remaining;
        double pct_pago        = (total > 0) ? (pago / total * 100.0) : 0.0;

        col_add(b, "  %-14.14s %3.0f%% pago", credor, pct_pago);
        col_add(b, "  Restante: R$ %8.2f / R$ %.2f", remaining, total);
        total_restante += remaining;
        found = 1;
    }
    sqlite3_finalize(stmt);

    if (!found) {
        col_add(b, "  Nenhum emprestimo cadastrado.");
        return;
    }
    col_add(b, "  ......................................");
    col_add(b, "  Total restante  R$ %10.2f", total_restante);
}

// coluna esquerda
static void bloco_cartoes(ColBuf *b, int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    time_t now = time(NULL);
    struct tm *tm_ = localtime(&now);
    char mes_ano[8], like_pat[14];
    strftime(mes_ano, sizeof(mes_ano), "%m/%Y", tm_);
    snprintf(like_pat, sizeof(like_pat), "%%/%s", mes_ano);

    col_add(b, "");
    col_add(b, "  CARTOES DE CREDITO");
    col_add(b, DIV_COL);

    const char *sql =
        "SELECT c.id, c.name, c.\"limit\", "
        "       (SELECT COALESCE(SUM(t.amount), 0.0) FROM transactions t "
        "        WHERE t.credit_card_id = c.id AND t.date LIKE ? AND t.type = 'expense') AS spent "
        "FROM credit_cards c "
        "WHERE c.user_id = ? "
        "ORDER BY c.name;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        col_add(b, "  [ERRO] Falha ao carregar cartoes.");
        return;
    }
    sqlite3_bind_text(stmt, 1, like_pat, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        double limit     = sqlite3_column_double(stmt, 2);
        double spent     = sqlite3_column_double(stmt, 3);
        double avail     = limit - spent;
        const char *alert = (avail < 0.2 * limit) ? " [!]" : "";

        col_add(b, "  %-10.10s R$%7.2f / R$%7.2f%s", name, avail, limit, alert);
        found = 1;
    }
    sqlite3_finalize(stmt);

    if (!found) col_add(b, "  Nenhum cartao cadastrado.");
}

// principal
void dashboard(void) {
    ui_clear();

    time_t now = time(NULL);
    struct tm *tm_ = localtime(&now);
    char data_hora[32];
    strftime(data_hora, sizeof(data_hora), "%d/%m/%Y  %H:%M", tm_);

    printf("%s\n", DIV_FULL);
    printf("  MoneyFlow  |  %s  |  Ola, %s\n", data_hora, session.username);
    printf("%s\n", DIV_FULL);

    ColBuf esq = { .count = 0 };
    double total = bloco_contas(&esq, session.user_id);
    bloco_movimentacoes(&esq, session.user_id);
    bloco_cartoes(&esq, session.user_id);

    ColBuf dir = { .count = 0 };
    bloco_bills(&dir, session.user_id);
    bloco_metas(&dir, session.user_id);
    bloco_emprestimos(&dir, session.user_id);

    printf("\n");
    col_render(&esq, &dir);

    printf("\n%s\n", DIV_FULL);
    printf("  Saldo consolidado total: R$ %.2f\n", total);
    printf("%s\n", DIV_FULL);

    printf("\nPressione Enter para voltar ao menu...");
    getchar();
}