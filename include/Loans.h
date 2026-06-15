#ifndef LOANS_H
#define LOANS_H

// Declarações locais para evitar warnings de compilação
void menu_emprestimos();
int validade_user_emprestimo(int user_id, int loan_id);
void listar_emprestimos(int user_id);
void adicionar_emprestimo(int user_id, const char *credor, float total_amount, float valorParcelas[], int quantParcelas);
void editar_emprestimo(int id, const char *credor, float total_amount, float valorParcelas[], int quantParcelas);
void excluir_emprestimo(int id);
int pagar_parcelas(int installments_id, int loan_id); // Ajustado para int e 2 parâmetros
void listar_parcelas(int loan_id);




#endif // CATEGORIES_H