#include "chat.h"
#include "history.h"
#include "session.h"

int message_queue_id = -1;
int am_first_user = 0;
long outgoing_type = 0;
long incoming_type = 0;
pid_t receiver_process_id = -1;

void write_stdout(const char *text) {
    write(STDOUT_FD, text, strlen(text));
}

void clear_terminal(void) {
    write(STDOUT_FD, TERMINAL_CLEAR_SEQUENCE, strlen(TERMINAL_CLEAR_SEQUENCE));
}

void show_chat_closed_screen(void) {
    clear_terminal();
    write_stdout(CHAT_HEADER);
    write_stdout(SYSTEM_PREFIX);
    write_stdout(MSG_OTHER_USER_LEFT);
}

void terminate_program(void) {
    if (receiver_process_id > 0) {
        kill(receiver_process_id, SIGTERM);
        waitpid(receiver_process_id, NULL, 0);
    }

    if (am_first_user) {
        unlink(SESSION_FILE);

        if (message_queue_id != -1) {
            msgctl(message_queue_id, IPC_RMID, NULL);
        }
    }

    char history_filename[64];
    build_history_filename(history_filename);
    unlink(history_filename);

    exit(0);
}

void process_received_message(struct ChatMessage *received_message) {
    if (strncmp(received_message->mtext,
                COMMAND_SYS_EXIT,
                strlen(COMMAND_SYS_EXIT)) == 0) {
        kill(getppid(), SIGUSR1);
        exit(0);
    }

    write_stdout(GHOST_PREFIX);
    write_stdout(received_message->mtext);
    write_stdout("\n");

    append_to_history(GHOST_PREFIX, received_message->mtext);

    if (strncmp(received_message->mtext,
                COMMAND_BURN,
                strlen(COMMAND_BURN)) == 0) {
        schedule_burn_removal(GHOST_PREFIX, received_message->mtext);
    }
}

void run_receiver_loop(void) {
    struct ChatMessage received_message;

    while (1) {
        if (msgrcv(message_queue_id,
                   &received_message,
                   sizeof(received_message.mtext),
                   incoming_type,
                   0) != -1) {
            process_received_message(&received_message);
        }
    }
}

void process_outgoing_message(struct ChatMessage *outgoing_message, int bytes_read) {
    if (bytes_read > 0) {
        if (outgoing_message->mtext[bytes_read - 1] == '\n') {
            outgoing_message->mtext[bytes_read - 1] = '\0';
        } else {
            outgoing_message->mtext[bytes_read] = '\0';
        }

        if (strncmp(outgoing_message->mtext,
                    COMMAND_EXIT,
                    strlen(COMMAND_EXIT)) == 0) {
            strcpy(outgoing_message->mtext, COMMAND_SYS_EXIT);
            msgsnd(message_queue_id,
                   outgoing_message,
                   sizeof(outgoing_message->mtext),
                   0);
            show_chat_closed_screen();
            terminate_program();
        }

        msgsnd(message_queue_id,
               outgoing_message,
               sizeof(outgoing_message->mtext),
               0);

        append_to_history("", outgoing_message->mtext);

        if (strncmp(outgoing_message->mtext,
                    COMMAND_BURN,
                    strlen(COMMAND_BURN)) == 0) {
            schedule_burn_removal("", outgoing_message->mtext);
        }
    }
}

void run_sender_loop(void) {
    struct ChatMessage outgoing_message;
    outgoing_message.mtype = outgoing_type;

    while (1) {
        int bytes_read = read(STDIN_FILENO,
                              outgoing_message.mtext,
                              MAX_TEXT - 1);
        process_outgoing_message(&outgoing_message, bytes_read);
    }
}
