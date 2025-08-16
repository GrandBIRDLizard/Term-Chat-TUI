/*MIT License

Copyright (c) 2025 GrandBIRDLizard

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

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

