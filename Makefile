CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lpthread -lportaudio -lssl -lcrypto
NCURSES = -lncurses

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    PORTAUDIO_CFLAGS = -I/opt/homebrew/include
    PORTAUDIO_LIBS = -L/opt/homebrew/lib
else
    PORTAUDIO_CFLAGS =
    PORTAUDIO_LIBS =
endif

OBJS = client.o text_chat.o audio_chat.o tui.o

all: client server

client: $(OBJS)
	$(CC) $(CFLAGS) $(PORTAUDIO_CFLAGS) -o client $(OBJS) $(PORTAUDIO_LIBS) $(LIBS) $(NCURSES)

server: server.o
	$(CC) $(CFLAGS) -o server server.o $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(PORTAUDIO_CFLAGS) -c $< -o $@

clean:
	rm -f *.o client server

check_deps:
	@echo "Checking dependencies..."
	@which portaudio-config > /dev/null || echo "Missing: PortAudio"
	@which pkg-config > /dev/null || echo "Missing: pkg-config"
	@pkg-config --exists ncurses || echo "Missing: ncurses"

certs:
	openssl req	 -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes


