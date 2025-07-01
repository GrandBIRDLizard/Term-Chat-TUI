#ifndef TUI_H
#define TUI_H

typedef struct {
    char server_ip[64];
    int port;
    int mode; 
} TuiInput;

void tui_init();
void tui_end();
TuiInput tui_get_user_input();

#endif

