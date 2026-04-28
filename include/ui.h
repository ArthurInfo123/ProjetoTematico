#ifndef UI_H
#define UI_H

void ui_clear(void);
void ui_header(const char *title);
void ui_success(const char *msg);
void ui_error(const char *msg);
void ui_divider(void);
int  ui_read_int(const char *prompt);
void ui_read_str(const char *prompt, char *buf, int size);
double ui_read_double(const char *prompt);

#endif
