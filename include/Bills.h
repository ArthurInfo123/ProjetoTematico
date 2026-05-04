#ifndef BILLS_H
#define BILLS_H

void listar_bills(int user_id);
void adicionar_bills(int user_id, const char *description, double amount, int due_day);
void editar_bills(int id, const char *description, double amount, int due_day);
void excluir_bills(int id);
void menu_bills();
int validate_due_day(int due_day);
int validade_user_bills(int user_id, int bills_id); 

#endif // BILLS_H