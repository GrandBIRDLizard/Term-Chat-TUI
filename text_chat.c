#include "text_chat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ncurses.h>


#define BUFFER_SIZE 1024
#define CHAT_HISTORY_LINES 200
#define MAX_DISPLAY_LINES 512  
#define MIN_ROWS 25
#define MIN_COLS 45

ChatLine chat_history[CHAT_HISTORY_LINES];
int chat_count = 0;
int chat_start = 0;
int chat_view_offset = 0; 

WINDOW *recv_win, *input_win;
pthread_mutex_t win_mutex = PTHREAD_MUTEX_INITIALIZER;

void wrap_text(const char *text, int width, char lines[][BUFFER_SIZE], int *nlines, int no_wrap) {
    if (no_wrap) {
        strncpy(lines[0], text, BUFFER_SIZE - 1);
        lines[0][BUFFER_SIZE-1] = '\0';
        *nlines = 1;
        return;
    }
    int len = strlen(text);
    int start = 0;
    *nlines = 0;
    while (start < len && *nlines < MAX_DISPLAY_LINES) {
        int chunk = width;
        if (start + chunk > len) chunk = len - start;
        int breakpos = chunk;
        for (int i = chunk - 1; i > 0; i--) {
            if (text[start + i] == ' ') {
                breakpos = i;
                break;
            }
        }
        if (breakpos < chunk && (start + breakpos) < len) chunk = breakpos;
        strncpy(lines[*nlines], text + start, chunk);
        lines[*nlines][chunk] = '\0';
        int skip = 0;
        while (text[start + chunk + skip] == ' ') skip++;
        start += chunk + skip;
        (*nlines)++;
    }
}

void add_chat_line(const char *msg, int is_sent) {
    int idx = (chat_start + chat_count) % CHAT_HISTORY_LINES;
    strncpy(chat_history[idx].text, msg, BUFFER_SIZE - 1);
    chat_history[idx].text[BUFFER_SIZE - 1] = '\0';
    chat_history[idx].is_sent = is_sent;

    if (chat_count < CHAT_HISTORY_LINES) {
        chat_count++;
    } else {
        chat_start = (chat_start + 1) % CHAT_HISTORY_LINES;
    }

    if (chat_view_offset == 0) {
    } else if (chat_view_offset < chat_count - 1) {
        chat_view_offset++;
        if (chat_view_offset > chat_count - 1) chat_view_offset = chat_count - 1;
    }
}

void redraw_chat_history() {
    int max_y, max_x;
    getmaxyx(recv_win, max_y, max_x);

    int col_mid = max_x / 2;
    int left_start = 1;
    int left_width = col_mid - 2;
    int right_start = col_mid + 1;
    int right_width = max_x - right_start - 1;

    werase(recv_win);
    box(recv_win, 0, 0);

    typedef struct {
        int buf_idx;  
        int wrap_idx;
        int is_sent;
        char line[BUFFER_SIZE];
    } DisplayLine;
    DisplayLine display_lines[MAX_DISPLAY_LINES];
    int display_line_count = 0;

    for (int c = 0; c < chat_count && display_line_count < MAX_DISPLAY_LINES; ++c) {
        int buf_idx = (chat_start + c) % CHAT_HISTORY_LINES;
        char wrap_lines[MAX_DISPLAY_LINES][BUFFER_SIZE];
        int nlines = 0;
        if (chat_history[buf_idx].is_sent == 1) {
            wrap_text(chat_history[buf_idx].text, left_width, wrap_lines, &nlines, 0);
        } else if (chat_history[buf_idx].is_sent == 0) {
            wrap_text(chat_history[buf_idx].text, right_width, wrap_lines, &nlines, 0);
        } else if (chat_history[buf_idx].is_sent == 2) {
            wrap_text(chat_history[buf_idx].text, max_x - 2, wrap_lines, &nlines, 1);
        }
        for (int w = 0; w < nlines && display_line_count < MAX_DISPLAY_LINES; ++w) {
            display_lines[display_line_count].buf_idx = buf_idx;
            display_lines[display_line_count].wrap_idx = w;
            display_lines[display_line_count].is_sent = chat_history[buf_idx].is_sent;
            strncpy(display_lines[display_line_count].line, wrap_lines[w], BUFFER_SIZE - 1);
            display_lines[display_line_count].line[BUFFER_SIZE - 1] = '\0';
            display_line_count++;
        }
    }

    int lines_to_show = max_y - 2;
    int start_line = (display_line_count <= lines_to_show) ? 0 : display_line_count - lines_to_show - chat_view_offset;

    for (int i = 0; i < lines_to_show; ++i) {
        int disp_idx = start_line + i;
        if (disp_idx >= display_line_count) break;
        DisplayLine *dl = &display_lines[disp_idx];
        if (dl->is_sent == 1) {
            mvwprintw(recv_win, i + 1, left_start, "%-*s", left_width, dl->line);
        } else if (dl->is_sent == 0) {
            mvwprintw(recv_win, i + 1, right_start, "%-*s", right_width, dl->line);
        } else if (dl->is_sent == 2) {
            int msg_len = strlen(dl->line);
            int center_col = (max_x - msg_len) / 2;
            if (center_col < 1) center_col = 1;
            mvwprintw(recv_win, i + 1, center_col, "%s", dl->line);
        }
    }

    if (display_line_count > lines_to_show) {
        mvwprintw(recv_win, max_y - 1, max_x - 12, "[%d/%d]", display_line_count - chat_view_offset, display_line_count);
    }
    wrefresh(recv_win);
}

void *receive_messages(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];

    while (1) {
        int valread = read(sock, buffer, BUFFER_SIZE - 1);
        if (valread <= 0) break;
        buffer[valread] = '\0';
        pthread_mutex_lock(&win_mutex);

        char formatted[BUFFER_SIZE];
        const char *prefix = "Received: ";
        size_t prefix_len = strlen(prefix);
        snprintf(formatted, sizeof(formatted), "%s%.*s", prefix, (int)(sizeof(formatted) - prefix_len - 1), buffer);

        add_chat_line(formatted, 0);
        redraw_chat_history();
        pthread_mutex_unlock(&win_mutex);
    }
    return NULL;
}

void print_sender_message(const char *msg) {
    pthread_mutex_lock(&win_mutex);
    char formatted[BUFFER_SIZE];
    const char *prefix = "You: ";
    size_t prefix_len = strlen(prefix);
    snprintf(formatted, sizeof(formatted), "%s%.*s", prefix, (int)(sizeof(formatted) - prefix_len - 1), msg);
    add_chat_line(formatted, 1);
    redraw_chat_history();
    pthread_mutex_unlock(&win_mutex);
}

void clear_input_line() {
    pthread_mutex_lock(&win_mutex);
    wclear(input_win);
    box(input_win, 0, 0);
    wrefresh(input_win);
    pthread_mutex_unlock(&win_mutex);
}

void redraw_input_line(const char *buffer, int input_len, int cursor_pos) {
    pthread_mutex_lock(&win_mutex);
    int max_y, max_x;
    getmaxyx(input_win, max_y, max_x);
    int max_input = max_x - 4; 

    int start = 0;
    if (cursor_pos >= max_input) {
        start = cursor_pos - max_input + 1;
    } else if (input_len >= max_input && cursor_pos == input_len) {
        start = input_len - max_input;
    }

    wmove(input_win, 1, 2);
    wclrtoeol(input_win);

    waddnstr(input_win, buffer + start, max_input);

    int cursor_col = 2 + (cursor_pos - start);
    wmove(input_win, 1, cursor_col);

    wrefresh(input_win);
    pthread_mutex_unlock(&win_mutex);
}

void start_text_chat(const char *server_ip, int port) {
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t thread_id;
    char buffer[BUFFER_SIZE];
    int input_len, cursor_pos;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed!");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    initscr();
    cbreak();
    noecho();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int input_h = 3;
    int recv_h = rows - input_h;

    recv_win = newwin(recv_h, cols, 0, 0);
    input_win = newwin(input_h, cols, recv_h, 0);
    scrollok(recv_win, FALSE);
    idlok(recv_win, FALSE);
    keypad(recv_win, TRUE);
    keypad(input_win, TRUE);

    pthread_mutex_lock(&win_mutex);
    add_chat_line("=== Text Mode ===", 2);
    add_chat_line("Connected to Server", 2);
    add_chat_line("=== Chat Away ===", 2);
    add_chat_line("*Type '/quit' to exit*", 2);
    redraw_chat_history();
    pthread_mutex_unlock(&win_mutex);

    box(input_win, 0, 0);
    wrefresh(input_win);

    pthread_create(&thread_id, NULL, receive_messages, &sock);

	while (1) {
    pthread_mutex_lock(&win_mutex);
    wmove(input_win, 1, 2);
    wrefresh(input_win);
    pthread_mutex_unlock(&win_mutex);

    input_len = 0;
    cursor_pos = 0;
    memset(buffer, 0, sizeof(buffer));
    int done = 0;

    while (!done) {
        redraw_input_line(buffer, input_len, cursor_pos);
        int ch = wgetch(input_win);

        if (ch == '\n' || ch == KEY_ENTER) {
            done = 1;
        }
        else if (ch == KEY_LEFT) {
            if (cursor_pos > 0) cursor_pos--;
        } else if (ch == KEY_RIGHT) {
            if (cursor_pos < input_len) cursor_pos++;
        } else if (ch == KEY_HOME) {
            cursor_pos = 0;
        } else if (ch == KEY_END) {
            cursor_pos = input_len;
        }
        else if (ch == KEY_UP) {
            pthread_mutex_lock(&win_mutex);
            int recv_max_y, recv_max_x;
            getmaxyx(recv_win, recv_max_y, recv_max_x);
            int lines_to_show = recv_max_y - 2;
            int display_line_count = 0;
            for (int c = 0; c < chat_count && display_line_count < MAX_DISPLAY_LINES; ++c) {
                int buf_idx = (chat_start + c) % CHAT_HISTORY_LINES;
                char wrap_lines[MAX_DISPLAY_LINES][BUFFER_SIZE];
                int nlines = 0;
                if (chat_history[buf_idx].is_sent)
                    wrap_text(chat_history[buf_idx].text, recv_max_x/2-2, wrap_lines, &nlines, 0);
                else
                    wrap_text(chat_history[buf_idx].text, recv_max_x-recv_max_x/2-2, wrap_lines, &nlines, 0);
                display_line_count += nlines;
            }
            int max_offset = (display_line_count > lines_to_show) ? (display_line_count - lines_to_show) : 0;
            if (chat_view_offset < max_offset) {
                chat_view_offset++;
                redraw_chat_history();
            }
            pthread_mutex_unlock(&win_mutex);
        } else if (ch == KEY_DOWN) {
            pthread_mutex_lock(&win_mutex);
            if (chat_view_offset > 0) {
                chat_view_offset--;
                redraw_chat_history();
            }
            pthread_mutex_unlock(&win_mutex);
        } else if (ch == KEY_PPAGE) { 
            pthread_mutex_lock(&win_mutex);
            int recv_max_y, recv_max_x;
            getmaxyx(recv_win, recv_max_y, recv_max_x);
            int lines_to_show = recv_max_y - 2;
            int display_line_count = 0;
            for (int c = 0; c < chat_count && display_line_count < MAX_DISPLAY_LINES; ++c) {
                int buf_idx = (chat_start + c) % CHAT_HISTORY_LINES;
                char wrap_lines[MAX_DISPLAY_LINES][BUFFER_SIZE];
                int nlines = 0;
                if (chat_history[buf_idx].is_sent)
                    wrap_text(chat_history[buf_idx].text, recv_max_x/2-2, wrap_lines, &nlines, 0);
                else
                    wrap_text(chat_history[buf_idx].text, recv_max_x-recv_max_x/2-2, wrap_lines, &nlines, 0);
                display_line_count += nlines;
            }
            int max_offset = (display_line_count > lines_to_show) ? (display_line_count - lines_to_show) : 0;
            chat_view_offset += recv_max_y - 2;
            if (chat_view_offset > max_offset) chat_view_offset = max_offset;
            redraw_chat_history();
            pthread_mutex_unlock(&win_mutex);
        } else if (ch == KEY_NPAGE) { 
            pthread_mutex_lock(&win_mutex);
            int recv_max_y, recv_max_x;
            getmaxyx(recv_win, recv_max_y, recv_max_x);
            chat_view_offset -= recv_max_y - 2;
            if (chat_view_offset < 0) chat_view_offset = 0;
            redraw_chat_history();
            pthread_mutex_unlock(&win_mutex);
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (cursor_pos > 0) {
                memmove(buffer + cursor_pos - 1, buffer + cursor_pos, input_len - cursor_pos + 1);
                input_len--;
                cursor_pos--;
            }
        } else if (ch == KEY_DC) { 
            if (cursor_pos < input_len) {
                memmove(buffer + cursor_pos, buffer + cursor_pos + 1, input_len - cursor_pos);
                input_len--;
            }
        } else if (ch >= 32 && ch < 127 && input_len < BUFFER_SIZE - 2) { 
            memmove(buffer + cursor_pos + 1, buffer + cursor_pos, input_len - cursor_pos + 1);
            buffer[cursor_pos] = ch;
            input_len++;
            cursor_pos++;
        }
    }
    buffer[input_len] = '\0';

    if (strncmp(buffer, "/quit", 5) == 0) break;

    send(sock, buffer, strlen(buffer), 0);
    print_sender_message(buffer);
    clear_input_line();
	}

pthread_cancel(thread_id);
pthread_join(thread_id, NULL);
close(sock);
delwin(recv_win);
delwin(input_win);
endwin();
}



