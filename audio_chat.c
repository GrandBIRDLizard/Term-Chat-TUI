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

#include "audio_chat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <portaudio.h>
#include <ncurses.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define CHANNELS 1

static WINDOW *status_win, *controls_win;
static pthread_mutex_t ui_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int sock;
    struct sockaddr_in server_addr;
    PaStream *stream;
    volatile bool *running;
    volatile bool *muted;
} SharedArgs;

volatile bool muted = false, running = true, connected = true;

void draw_controls_bar() {
    pthread_mutex_lock(&ui_mutex);
    werase(controls_win);
    box(controls_win, 0, 0);
    int width = getmaxx(controls_win);

    if (muted)
        mvwprintw(controls_win, 1, 2, "[M] Mic: MUTED");
    else
        mvwprintw(controls_win, 1, 2, "[M] Mic: ON");

    if (connected)
        mvwprintw(controls_win, 1, width - 25, "[Q] Disconnect: CONNECTED");
    else
        mvwprintw(controls_win, 1, width - 20, "Disconnected");

    wrefresh(controls_win);
    pthread_mutex_unlock(&ui_mutex);
}

void draw_status(const char *msg) {
    pthread_mutex_lock(&ui_mutex);
    werase(status_win);
    box(status_win, 0, 0);
    mvwprintw(status_win, 1, 2, "Status: %s", msg);
    wrefresh(status_win);
    pthread_mutex_unlock(&ui_mutex);
}

void *sender_thread(void *arg) {
    SharedArgs *args = (SharedArgs *)arg;
    PaStream *stream;
    PaStreamParameters input;
    input.device = Pa_GetDefaultInputDevice();
    input.channelCount = CHANNELS;
    input.sampleFormat = paFloat32;
    input.suggestedLatency = Pa_GetDeviceInfo(input.device)->defaultLowInputLatency;
    input.hostApiSpecificStreamInfo = NULL;

    if (Pa_OpenStream(&stream, &input, NULL, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, NULL, NULL) != paNoError) {
        draw_status("Error opening input stream!");
        return NULL;
    }
    Pa_StartStream(stream);

    float buffer[FRAMES_PER_BUFFER * CHANNELS];

    while (*(args->running)) {
        if (!*(args->muted)) {
            Pa_ReadStream(stream, buffer, FRAMES_PER_BUFFER);
            sendto(args->sock, buffer, sizeof(buffer), 0,
                   (struct sockaddr *)&args->server_addr, sizeof(args->server_addr));
        } else {
            usleep(10000);
        }
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    draw_status("Sender thread stopped.");
    return NULL;
}

void *receiver_thread(void *arg) {
    SharedArgs *args = (SharedArgs *)arg;
    float buffer[FRAMES_PER_BUFFER * CHANNELS];

    while (*(args->running)) {
        ssize_t recvlen = recv(args->sock, buffer, sizeof(buffer), MSG_DONTWAIT);
        if (recvlen > 0) {
            Pa_WriteStream(args->stream, buffer, FRAMES_PER_BUFFER);
        } else {
            usleep(5000);
        }
    }
    draw_status("Receiver thread stopped.");
    return NULL;
}

void *ui_thread(void *arg) {
    volatile bool *run_ptr = ((volatile bool **)arg)[0];
    volatile bool *mute_ptr = ((volatile bool **)arg)[1];

    int ch;
    while (*run_ptr) {
        ch = getch();
        if (ch == 'q' || ch == 'Q') {
            connected = false;
            *run_ptr = false;
            draw_status("Disconnecting...");
            draw_controls_bar();
            break;
        }
        if (ch == 'm' || ch == 'M') {
            *mute_ptr = !(*mute_ptr);
            muted = *mute_ptr;
            draw_controls_bar();
            draw_status(muted ? "Microphone muted." : "Microphone unmuted.");
        }
        usleep(50000);
    }
    return NULL;
}

void start_audio_chat_with_ui(const char *server_ip, int port) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int controls_height = 3;

    status_win = newwin(rows - controls_height, cols, 0, 0);
    controls_win = newwin(controls_height, cols, rows - controls_height, 0);

    draw_status("Connecting to server...");
    draw_controls_bar();

    // Networking
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { draw_status("Socket error!"); endwin(); exit(1); }

    struct sockaddr_in local_addr = {0};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(0);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    Pa_Initialize();
    PaStream *out_stream;
    PaStreamParameters output;
    output.device = Pa_GetDefaultOutputDevice();
    output.channelCount = CHANNELS;
    output.sampleFormat = paFloat32;
    output.suggestedLatency = Pa_GetDeviceInfo(output.device)->defaultLowOutputLatency;
    output.hostApiSpecificStreamInfo = NULL;
    Pa_OpenStream(&out_stream, NULL, &output, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, NULL, NULL);
    Pa_StartStream(out_stream);

	clear();
	refresh();
	draw_status("Connected. Audio mode active.");
	draw_controls_bar();

    pthread_t s_tid, r_tid, ui_tid;
    SharedArgs s_args = {sock, server_addr, NULL, &running, &muted};
    SharedArgs r_args = {sock, server_addr, out_stream, &running, &muted};
    volatile bool *ui_args[2] = {&running, &muted};

    pthread_create(&s_tid, NULL, sender_thread, &s_args);
    pthread_create(&r_tid, NULL, receiver_thread, &r_args);
    pthread_create(&ui_tid, NULL, ui_thread, ui_args);

    draw_status("Connected. Audio mode active.");
    draw_controls_bar();

    pthread_join(ui_tid, NULL);

    running = false;
    pthread_join(s_tid, NULL);
    pthread_join(r_tid, NULL);

    Pa_StopStream(out_stream);
    Pa_CloseStream(out_stream);
    Pa_Terminate();
    close(sock);

    delwin(status_win);
    delwin(controls_win);
    endwin();
}

