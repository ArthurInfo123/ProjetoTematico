#ifndef SESSION_H
#define SESSION_H

#define USERNAME_MAX 64

typedef struct {
    int  logged_in;
    int  user_id;
    char username[USERNAME_MAX];
} Session;

extern Session session;

void session_start(int user_id, const char *username);
void session_end(void);
int  session_require(void);

#endif
