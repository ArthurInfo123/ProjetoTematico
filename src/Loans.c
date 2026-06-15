#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "session.h"
#include "../include/Loans.h"

// Protótipos locais corrigidos e sincronizados
int validade_user_emprestimo(int user_id, int loan_id);
void listar_emprestimos(int user_id);
void adicionar_emprestimo(int user_id, const char *credor, float total_amount, float valorParcelas[], int quantParcelas);
void editar_emprestimo(int id, const char *credor, float total_amount, float valorParcelas[], int quantParcelas);
void excluir_emprestimo(int id);
int pagar_parcelas(int installments_id, int loan_id); // Ajustado para int e 2 parâmetros
void listar_parcelas(int loan_id);

void menu_emprestimos() {
    int opcao;

    do {
        printf("\n=== MENU DE EMPRESTIMOS ===\n");
        printf("1. Listar emprestimos\n");
        printf("2. Adicionar emprestimo\n");
        printf("3. Editar emprestimo\n");
        printf("4. Excluir emprestimo\n");
        printf("5. Pagar parcela\n");
        printf("0. Voltar\n");

        opcao = ui_read_int("Opcao: ");

        switch (opcao) {
            case 1:
                listar_emprestimos(session.user_id);
                printf("\nPressione Enter para continuar...");
                getchar(); getchar(); 
                break;

            case 2: {
                char credor[100];
                printf("Digite o nome do credor: ");
                getchar(); 
                fgets(credor, sizeof(credor), stdin);
                credor[strcspn(credor, "\n")] = 0;

                float total_amount;
                printf("Digite o valor total do empréstimo: ");
                scanf("%f", &total_amount);

                int quantParcelas;
                printf("Digite a quantidade de parcelas do empréstimo: ");
                scanf("%d", &quantParcelas);

                if (quantParcelas <= 0) {
                    ui_error("Quantidade de parcelas inválida.");
                    break;
                }

                float listaParcelas[quantParcelas];
                float valorBase = total_amount / quantParcelas;
                float soma = 0;

                for (int i = 0; i < quantParcelas - 1; i++) {
                    listaParcelas[i] = valorBase;
                    soma += valorBase;
                }
                listaParcelas[quantParcelas - 1] = total_amount - soma;
                
                adicionar_emprestimo(session.user_id, credor, total_amount, listaParcelas, quantParcelas);
                printf("\nPressione Enter para continuar...");
                getchar(); getchar();
                break;
            }

            case 3: {
                listar_emprestimos(session.user_id);
                int id = ui_read_int("Digite o ID do empréstimo que deseja editar: ");
                
                if (!validade_user_emprestimo(session.user_id, id)) break;

                char new_credor[100];
                printf("Digite o novo nome do credor: ");
                getchar(); 
                fgets(new_credor, sizeof(new_credor), stdin);
                new_credor[strcspn(new_credor, "\n")] = 0;

                float total_amount;
                printf("Digite o novo valor total do empréstimo: ");
                scanf("%f", &total_amount);

                int quantParcelas;
                printf("Digite a nova quantidade de parcelas: ");
                scanf("%d", &quantParcelas);

                if (quantParcelas <= 0) {
                    ui_error("Quantidade de parcelas inválida.");
                    break;
                }

                float listaParcelas[quantParcelas];
                float valorBase = total_amount / quantParcelas;
                float soma = 0;

                for (int i = 0; i < quantParcelas - 1; i++) {
                    listaParcelas[i] = valorBase;
                    soma += valorBase;
                }
                listaParcelas[quantParcelas - 1] = total_amount - soma;

                editar_emprestimo(id, new_credor, total_amount, listaParcelas, quantParcelas);
                printf("\nPressione Enter para continuar...");
                getchar(); getchar();
                break;
            }

            case 4: {
                listar_emprestimos(session.user_id);
                int id = ui_read_int("Digite o ID do emprestimo: ");
                excluir_emprestimo(id);
                printf("\nPressione Enter para continuar...");
                getchar(); getchar();
                break;
            }

            case 5: {
                listar_emprestimos(session.user_id);
                int id = ui_read_int("Digite o ID do emprestimo: ");
                
                if (!validade_user_emprestimo(session.user_id, id)) break;

                listar_parcelas(id);
                int parcela_id = ui_read_int("Digite o ID da parcela: ");
                    
                pagar_parcelas(parcela_id, id);
                
                printf("\nPressione Enter para continuar...");
                getchar(); getchar();
                break; // CORREÇÃO: Adicionado break para não cair no case 0
            }

            case 0:
                printf("Voltando...\n");
                return;

            default:
                ui_error("Opcao invalida.");
                printf("\nPressione Enter para continuar...");
                getchar(); getchar();
        }

    } while (opcao != 0);
}

void listar_emprestimos(int user_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "SELECT id, creditor, total_amount, remaining_amount FROM loans WHERE user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar empréstimos.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    printf("\n=== EMPRÉSTIMOS ===\n");

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *creditor = sqlite3_column_text(stmt, 1);
        double total = sqlite3_column_double(stmt, 2);
        double restante = sqlite3_column_double(stmt, 3);
        printf("ID: %d | Credor: %s | Total: R$ %.2f | Restante: R$ %.2f\n", id, creditor, total, restante);
        found = 1;
    }

    if (!found)
        printf("Nenhum empréstimo cadastrado.\n");

    sqlite3_finalize(stmt);
}

void listar_parcelas(int loan_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    // CORREÇÃO: Filtrando por loan_id em vez de id
    const char *sql = "SELECT id, installments_value FROM installments_loans WHERE loan_id = ? AND paid = 0;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao listar parcelas.");
        return;
    }

    // CORREÇÃO: Vinculando a variável correta (loan_id)
    sqlite3_bind_int(stmt, 1, loan_id);

    printf("\n=== PARCELAS NÃO PAGAS ===\n");

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        // CORREÇÃO: Obtendo o valor como double/float de maneira correta
        double valor = sqlite3_column_double(stmt, 1); 
        printf("ID Parcela: %d | Valor: R$ %.2f\n", id, valor);
        found = 1;
    }

    if (!found)
        printf("Nenhuma parcela pendente para este empréstimo.\n");

    sqlite3_finalize(stmt);
}

void adicionar_emprestimo(int user_id, const char *credor, float total_amount, float valorParcelas[], int quantParcelas) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    const char *sql = "INSERT INTO loans (user_id, creditor, total_amount, remaining_amount) VALUES (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar INSERT do empréstimo.");
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, credor, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, (double)total_amount);
    sqlite3_bind_double(stmt, 4, (double)total_amount); 

    int last_id = 0;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Erro ao adicionar emprestimo. %s\n", sqlite3_errmsg(db)); 
        sqlite3_finalize(stmt);
        return;
    } else {
        last_id = (int)sqlite3_last_insert_rowid(db);
    }
    sqlite3_finalize(stmt);

    if (last_id != 0) {
        sqlite3_stmt *stmtParcelas;
        const char *sqlParcelas = "INSERT INTO installments_loans (loan_id, installments_value, paid) VALUES (?, ?, ?);";
            
        if (sqlite3_prepare_v2(db, sqlParcelas, -1, &stmtParcelas, NULL) == SQLITE_OK) {
            for (int i = 0; i < quantParcelas; i++) {
                sqlite3_bind_int(stmtParcelas, 1, last_id);
                sqlite3_bind_double(stmtParcelas, 2, (double)valorParcelas[i]);
                sqlite3_bind_int(stmtParcelas, 3, 0);

                if (sqlite3_step(stmtParcelas) != SQLITE_DONE) {
                    printf("Erro ao adicionar parcela %d. %s\n", i + 1, sqlite3_errmsg(db));        
                }
                sqlite3_reset(stmtParcelas); 
            }
            sqlite3_finalize(stmtParcelas);
            printf("Empréstimo e parcelas salvos com sucesso.\n");
        } else {
            ui_error("Erro ao preparar INSERT das parcelas.");
        }
    }
}

void editar_emprestimo(int id, const char *credor, float total_amount, float valorParcelas[], int quantParcelas) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    // BLOQUEIO DE SEGURANÇA: Verifica se já houve alguma parcela paga
    const char *sqlVerificar = "SELECT total_amount, remaining_amount FROM loans WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sqlVerificar, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            double total_orig = sqlite3_column_double(stmt, 0);
            double restante = sqlite3_column_double(stmt, 1);
            
            if (restante < total_orig) {
                ui_error("Não é possível editar um empréstimo que já possui parcelas pagas!");
                sqlite3_finalize(stmt);
                return;
            }
        }
        sqlite3_finalize(stmt);
    }

    // 2. Se passou no teste, atualiza os dados principais do empréstimo
    const char *sql = "UPDATE loans SET creditor = ?, total_amount = ?, remaining_amount = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar UPDATE do empréstimo.");
        return;
    }

    sqlite3_bind_text(stmt, 1, credor, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, (double)total_amount);
    sqlite3_bind_double(stmt, 3, (double)total_amount); 
    sqlite3_bind_int(stmt, 4, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao editar empréstimo.");
        sqlite3_finalize(stmt);
        return;
    }
    sqlite3_finalize(stmt);

    // 3. Remove as parcelas antigas
    const char *sqlDeleteParcelas = "DELETE FROM installments_loans WHERE loan_id = ?;";
    if (sqlite3_prepare_v2(db, sqlDeleteParcelas, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // 4. Insere as novas parcelas recalculadas
    sqlite3_stmt *stmtParcelas;
    const char *sqlParcelas = "INSERT INTO installments_loans (loan_id, installments_value, paid) VALUES (?, ?, ?);";
        
    if (sqlite3_prepare_v2(db, sqlParcelas, -1, &stmtParcelas, NULL) == SQLITE_OK) {
        for (int i = 0; i < quantParcelas; i++) {
            sqlite3_bind_int(stmtParcelas, 1, id);
            sqlite3_bind_double(stmtParcelas, 2, (double)valorParcelas[i]);
            sqlite3_bind_int(stmtParcelas, 3, 0);

            if (sqlite3_step(stmtParcelas) != SQLITE_DONE) {
                printf("Erro ao recriar parcela %d na edição. %s\n", i + 1, sqlite3_errmsg(db));        
            }
            sqlite3_reset(stmtParcelas);
        }
        sqlite3_finalize(stmtParcelas);
        printf("Empréstimo atualizado e parcelas recalculadas com sucesso!\n");
    } else {
        ui_error("Erro ao preparar recriação de parcelas.");
    }
}

void excluir_emprestimo(int id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;

    if (!validade_user_emprestimo(session.user_id, id)) return;

    // BLOQUEIO DE SEGURANÇA: Verifica se já houve alguma parcela paga antes de excluir
    const char *sqlVerificar = "SELECT total_amount, remaining_amount FROM loans WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sqlVerificar, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            double total_orig = sqlite3_column_double(stmt, 0);
            double restante = sqlite3_column_double(stmt, 1);
            
            if (restante < total_orig) {
                ui_error("Não é possível excluir um empréstimo em andamento com parcelas pagas!");
                sqlite3_finalize(stmt);
                return;
            }
        }
        sqlite3_finalize(stmt);
    }

    // Se passou, primeiro apaga as parcelas vinculadas
    const char *sqlParcelas = "DELETE FROM installments_loans WHERE loan_id = ?;";
    if (sqlite3_prepare_v2(db, sqlParcelas, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Depois apaga o empréstimo principal
    const char *sql = "DELETE FROM loans WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao preparar DELETE.");
        return;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ui_error("Erro ao excluir emprestimo.");
    } else {
        if (sqlite3_changes(db) > 0)
            printf("Empréstimo excluído com sucesso!\n");
        else
            printf("Nenhum empréstimo encontrado com ID %d\n", id);
    }

    sqlite3_finalize(stmt);
}

int validade_user_emprestimo(int user_id, int loan_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT user_id FROM loans WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ui_error("Erro ao validar acesso do usuario.");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, loan_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int user_id_curr = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (user_id_curr != user_id) {
            ui_error("Usuario nao tem acesso a este emprestimo.");
            return 0;
        }
        return 1;
    }

    sqlite3_finalize(stmt);
    ui_error("Emprestimo nao encontrado.");
    return 0;
}

int pagar_parcelas(int installments_id, int loan_id) {
    sqlite3 *db = returnConnection();
    sqlite3_stmt *stmt = NULL;
    double valorParcela = 0.0;

    const char *sqlBuscar = "SELECT installments_value FROM installments_loans WHERE id = ? AND paid = 0;";
    
    if (sqlite3_prepare_v2(db, sqlBuscar, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Erro ao preparar busca da parcela.\n");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, installments_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        valorParcela = sqlite3_column_double(stmt, 0); 
    } else {
        printf("Parcela não encontrada ou já está paga!\n");
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);

    const char *sqlAtualizarParcela = "UPDATE installments_loans SET paid = 1 WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sqlAtualizarParcela, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Erro ao preparar update da parcela.\n");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, installments_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Erro ao marcar parcela como paga: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);

    const char *sqlAtualizarEmprestimo = "UPDATE loans SET remaining_amount = remaining_amount - ? WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sqlAtualizarEmprestimo, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Erro ao preparar update do saldo do empréstimo.\n");
        return 0;
    }

    sqlite3_bind_double(stmt, 1, valorParcela);
    sqlite3_bind_int(stmt, 2, loan_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Erro ao atualizar saldo do empréstimo: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    printf("Parcela de R$ %.2f paga e saldo do empréstimo atualizado com sucesso!\n", valorParcela);
    return 1; 
}