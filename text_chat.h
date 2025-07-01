#ifndef TEXT_CHAT_H
#define TEXT_CHAT_H

#include <ncurses.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define CHAT_HISTORY_LINES 200
#define MAX_DISPLAY_LINES 512

typedef struct {
    char text[BUFFER_SIZE];
    int is_sent; // 0 = received, 1 = sent, 2 = system/centered
} ChatLine;

extern ChatLine chat_history[CHAT_HISTORY_LINES];
extern int chat_count;
extern int chat_start;
extern int chat_view_offset;

extern WINDOW *recv_win, *input_win;
extern pthread_mutex_t win_mutex;

// Message wrapping and UI
void wrap_text(const char *text, int width, char lines[][BUFFER_SIZE], int *nlines, int no_wrap);
void add_chat_line(const char *msg, int is_sent);
void redraw_chat_history(void);
void redraw_input_line(const char *buffer, int input_len, int cursor_pos);
void clear_input_line(void);

// Chat message handling
void print_sender_message(const char *msg);
void *receive_messages(void *socket_desc);

// Main entry point
void start_text_chat(const char *server_ip, int port);

#endif // TEXT_CHAT_H
