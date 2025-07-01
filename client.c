#include <stdio.h>
#include "tui.h"
#include "text_chat.h"
#include "audio_chat.h"

int main() {

#ifdef __APPLE__
	printf("[INFO] You are running on macOS.\n");
	printf("[WARNING] Make sure this app has microphone permissions and firewall access:\n");
	printf("Mic:  → System Settings > Privacy & Security > Microphone\n");
	printf("Firewall:  → System Settings > Network > Firewall > Options...\n");
	printf("Allow incoming connections for this app\n");
#endif

TuiInput user_input = tui_get_user_input();

    if (user_input.mode == 1) {
        start_text_chat(user_input.server_ip, user_input.port);
    } else if (user_input.mode == 2) {
        start_audio_chat_with_ui(user_input.server_ip, user_input.port);
    } else {
        fprintf(stderr, "Invalid mode selected.\n");
        return 1;
    }

    return 0;
}

