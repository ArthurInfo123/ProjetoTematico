#ifndef CATEGORIES_H
#define CATEGORIES_H

void listar_categorias();
void adicionar_categoria(int user_id, const char *name);
void editar_categoria(int id, const char *name);
void excluir_categoria(int id);
void menu_categorias();

#endif // CATEGORIES_H