#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"

void ui_clear(void) {
    printf("\033[2J\033[H");
}

void ui_header(const char *title) {
    ui_divider();
    printf("  %s\n", title);
    ui_divider();
}

void ui_success(const char *msg) {
    printf("[OK] %s\n", msg);
}

void ui_error(const char *msg) {
    printf("[ERRO] %s\n", msg);
}

void ui_divider(void) {
    printf("----------------------------------------\n");
}

int ui_read_int(const char *prompt) {
    int val;
    printf("%s", prompt);
    if (scanf("%d", &val) != 1) {
        while (getchar() != '\n');
        return -1;
    }
    while (getchar() != '\n');
    return val;
}

void ui_read_str(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    if (fgets(buf, size, stdin)) {
        buf[strcspn(buf, "\n")] = '\0';
    }
}

double ui_read_double(const char *prompt) {
    double val;
    printf("%s", prompt);
    if (scanf("%lf", &val) != 1) {
        while (getchar() != '\n');
        return -1.0;
    }
    while (getchar() != '\n');
    return val;
}
