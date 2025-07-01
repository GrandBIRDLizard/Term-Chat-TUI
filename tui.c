#include "tui.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>

void tui_init() {
    initscr();
    cbreak();
    noecho();
    curs_set(TRUE);
}

void tui_end() {
    endwin();
}

TuiInput tui_get_user_input() {
    tui_init();
    TuiInput input = { .port = 0, .mode = 0 };

    char ip_buffer[64];
    char port_buffer[16];

    mvprintw(2, 2, "Enter server IP: ");
    echo();
    getnstr(ip_buffer, sizeof(ip_buffer) - 1);
    strncpy(input.server_ip, ip_buffer, sizeof(input.server_ip));

    mvprintw(4, 2, "Enter server port: ");
    getnstr(port_buffer, sizeof(port_buffer) - 1);
    input.port = atoi(port_buffer);

    noecho();
    mvprintw(6, 2, "=== Communication Mode Select ===");
    mvprintw(8, 4, "1. Text Chat(TCP)");
    mvprintw(9, 4, "2. Voice Chat(UDP)");
    mvprintw(11, 2, "Enter your choice: ");
    refresh();

    int ch = getch();
    if (ch == '1') input.mode = 1;
    else if (ch == '2') input.mode = 2;

    tui_end();
    return input;
}

