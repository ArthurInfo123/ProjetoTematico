#ifndef BILLS_H
#define BILLS_H

void listar_bills();
void adicionar_bills(int user_id, const char *description, double amount, int due_day);
void editar_bills(int id, const char *name, const char *description, double amount, int due_day);
void excluir_bills(int id);
void menu_bills();
int validate_due_day(int due_day);

#endif // BILLS_H