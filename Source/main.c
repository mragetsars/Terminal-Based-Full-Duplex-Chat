#include "common.h"
#include "chat.h"
#include "session.h"
#include "signals.h"

int main(void) {
    signal(SIGINT, handle_sigint);
    signal(SIGUSR1, handle_peer_exit);

    clear_terminal();
    write_stdout(CHAT_HEADER);

    if (initialize_session() != 0) {
        return 1;
    }

    receiver_process_id = fork();

    if (receiver_process_id == 0) {
        run_receiver_loop();
    } else if (receiver_process_id > 0) {
        run_sender_loop();
    } else {
        write_stdout(SYSTEM_PREFIX);
        write_stdout("Failed to create receiver process.\n");
        terminate_program();
    }

    return 0;
}
