#ifndef ACCOUNTS_H
#define ACCOUNTS_H

void menu_contas(void);
void listar_contas(int user_id);
void adicionar_conta(int user_id, const char *name, double balance);
void editar_conta(int id, int user_id, const char *name);
void excluir_conta(int id, int user_id);
int  validate_user_account(int user_id, int account_id);

#endif
