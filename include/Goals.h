#ifndef GOALS_H
#define GOALS_H

void menu_metas(void);
void listar_metas(int user_id);
void adicionar_meta(int user_id, const char *name, double target_amount, const char *deadline);
void alocar_valor_meta(int goal_id, int user_id, int account_id, double amount);
void excluir_meta(int id, int user_id);
int  validate_user_goal(int user_id, int goal_id);

#endif
