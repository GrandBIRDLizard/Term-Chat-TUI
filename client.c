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

