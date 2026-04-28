#include <stdio.h>
#include <string.h>
#include "session.h"

Session session = {0, 0, ""};

void session_start(int user_id, const char *username) {
    session.logged_in = 1;
    session.user_id   = user_id;
    strncpy(session.username, username, USERNAME_MAX - 1);
    session.username[USERNAME_MAX - 1] = '\0';
}

void session_end(void) {
    session.logged_in = 0;
    session.user_id   = 0;
    session.username[0] = '\0';
}

int session_require(void) {
    if (!session.logged_in) {
        printf("[ERRO] Acesso negado. Faca login primeiro.\n");
        return 0;
    }
    return 1;
}
