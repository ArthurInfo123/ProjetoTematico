#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "db.h"
#include "session.h"
#include "ui.h"
#include "Dashboard.h"

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

//coluna esquerda
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
        /* "  Nome                R$ 9999.99" = 2+20+4+7 = 33 chars */
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

//coluan direita
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
    /* "  Receitas       R$ 9999999.99" = 2+15+4+10 = 31 chars */
    col_add(b, "  Receitas       R$ %10.2f", rec);
    col_add(b, "  Despesas       R$ %10.2f", desp);
    col_add(b, "  ......................................");
    if (bal >= 0)
        col_add(b, "  Balanco   (+)  R$ %10.2f", bal);
    else
        col_add(b, "  Balanco   (-)  R$ %10.2f", bal);
}

//direita
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

        /* [PEND] d.05 Descricao123  R$999.99
           6     + 5  + 13          + 9 = 33 chars + 2 indent = 35 */
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

//direita
static void bloco_placeholder(ColBuf *b, const char *titulo) {
    col_add(b, "");
    col_add(b, "  %s", titulo);
    col_add(b, DIV_COL);
    col_add(b, "  [Em breve] Proximas milestones.");
}

//principal
void dashboard(void) {
    ui_clear();

    time_t now = time(NULL);
    struct tm *tm_ = localtime(&now);
    char data_hora[32];
    strftime(data_hora, sizeof(data_hora), "%d/%m/%Y  %H:%M", tm_);

    /* Cabecalho ASCII */
    printf("%s\n", DIV_FULL);
    printf("  MoneyFlow  |  %s  |  Ola, %s\n", data_hora, session.username);
    printf("%s\n", DIV_FULL);

    /* Monta colunas */
    ColBuf esq = { .count = 0 };
    double total = bloco_contas(&esq, session.user_id);
    bloco_movimentacoes(&esq, session.user_id);

    ColBuf dir = { .count = 0 };
    bloco_bills(&dir, session.user_id);
    bloco_placeholder(&dir, "METAS FINANCEIRAS");
    bloco_placeholder(&dir, "EMPRESTIMOS");
    bloco_placeholder(&dir, "CARTOES DE CREDITO");

    /* Renderiza */
    printf("\n");
    col_render(&esq, &dir);

    /* Rodape */
    printf("\n%s\n", DIV_FULL);
    printf("  Saldo consolidado total: R$ %.2f\n", total);
    printf("%s\n", DIV_FULL);

    printf("\nPressione Enter para voltar ao menu...");
    getchar();
}