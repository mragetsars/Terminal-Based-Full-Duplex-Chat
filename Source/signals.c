#include "signals.h"
#include "chat.h"

void handle_sigint(int sig) {
    (void)sig;
    terminate_program();
}

void handle_peer_exit(int sig) {
    (void)sig;
    show_chat_closed_screen();
    terminate_program();
}
