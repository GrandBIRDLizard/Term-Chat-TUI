CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lpthread -lportaudio
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

all: client

client: $(OBJS)
	$(CC) $(CFLAGS) $(PORTAUDIO_CFLAGS) -o client $(OBJS) $(PORTAUDIO_LIBS) $(LIBS) $(NCURSES)

%.o: %.c
	$(CC) $(CFLAGS) $(PORTAUDIO_CFLAGS) -c $< -o $@

clean:
	rm -f *.o client

check_deps:
	@echo "Checking dependencies..."
	@which portaudio-config > /dev/null || echo "Missing: PortAudio"
	@which pkg-config > /dev/null || echo "Missing: pkg-config"
	@pkg-config --exists ncurses || echo "Missing: ncurses"


