#ifndef CREDITCARDS_H
#define CREDITCARDS_H

void menu_cartoes(void);
void listar_cartoes(int user_id);
void adicionar_cartao(int user_id, const char *name, double limit, int due_day);
void editar_cartao(int id, int user_id, const char *name, double limit, int due_day);
void excluir_cartao(int id, int user_id);
int  validate_user_credit_card(int user_id, int credit_card_id);

#endif // CREDITCARDS_H
