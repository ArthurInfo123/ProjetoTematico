#ifndef CATEGORIES_H
#define CATEGORIES_H

void listar_categorias(int user_id);
void adicionar_categoria(int user_id, const char *name);
void editar_categoria(int id, const char *name);
void excluir_categoria(int id);
void menu_categorias();
int validade_user_category(int user_id, int category_id); 


#endif // CATEGORIES_H