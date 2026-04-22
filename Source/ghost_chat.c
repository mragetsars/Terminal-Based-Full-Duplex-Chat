#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/types.h>


#define HISTORY_FILE_SUFFIX ".txt"
#define SESSION_FILE "ghost_session"
#define HISTORY_FILE_PREFIX "chat_history"
#define TEMP_HISTORY_FILE_SUFFIX "_tmp.txt"

#define MAX_TEXT 1024
#define STDOUT_FD STDOUT_FILENO

#define TERMINAL_CLEAR_SEQUENCE "\033[H\033[J"

#define GHOST_PREFIX "[Ghost] "
#define SYSTEM_PREFIX "[SYSTEM] "
#define CHAT_HEADER "=== Welcome to Ghost Chat ===\n"

#define MSG_OTHER_USER_LEFT "The chat was closed\n"
#define MSG_CONNECTED_TO_USER1 "Connected to User 1's chat!\n"
#define MSG_SESSION_OPEN_ERROR "Error opening session file.\n"
#define MSG_WAITING_FOR_USER2 "Waiting for User 2 to join...\n"
#define MSG_BURN_REMOVED "A BURN message just self-destructed!\n"

#define COMMAND_BURN "BURN"
#define COMMAND_EXIT "EXIT"
#define COMMAND_SYS_EXIT "SYS_EXIT"


struct ChatMessage {
    long mtype;
    char mtext[MAX_TEXT];
};


static int message_queue_id = -1;
static int am_first_user = 0;
static long outgoing_type = 0;
static long incoming_type = 0;
static pid_t receiver_process_id = -1;

static void write_stdout(const char *text) {
    write(STDOUT_FD, text, strlen(text));
}

static void clear_terminal(void) {
    write(STDOUT_FD, TERMINAL_CLEAR_SEQUENCE, strlen(TERMINAL_CLEAR_SEQUENCE));
}

static void build_history_filename(char *buffer) {
    sprintf(buffer, "%s_%d%s", HISTORY_FILE_PREFIX, am_first_user ? 1 : 2, HISTORY_FILE_SUFFIX);
}

static void build_temp_history_filename(char *buffer) {
    sprintf(buffer, "%s_%d%s", HISTORY_FILE_PREFIX, am_first_user ? 1 : 2, TEMP_HISTORY_FILE_SUFFIX);
}

static void show_chat_closed_screen(void) {
    clear_terminal();
    write_stdout(CHAT_HEADER);
    write_stdout(SYSTEM_PREFIX);
    write_stdout(MSG_OTHER_USER_LEFT);
}

static void terminate_program(void) {
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

static void handle_peer_exit(int sig) {
    (void)sig;
    show_chat_closed_screen();
    terminate_program();
}

static void append_to_history(const char *prefix, const char *message_text) {
    char history_filename[64];
    build_history_filename(history_filename);

    int fd = open(history_filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd != -1) {
        write(fd, prefix, strlen(prefix));
        write(fd, message_text, strlen(message_text));
        write(fd, "\n", 1);
        close(fd);
    }
}

static void redraw_history_after_removal(const char *line_to_remove) {
    char history_filename[64];
    char temporary_filename[64];

    build_history_filename(history_filename);
    build_temp_history_filename(temporary_filename);

    int input_fd = open(history_filename, O_RDONLY);
    if (input_fd == -1) {
        return;
    }

    int output_fd = open(temporary_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output_fd == -1) {
        close(input_fd);
        return;
    }

    char line_buffer[MAX_TEXT * 2];
    int line_length = 0;
    char current_char;
    int removed = 0;

    while (read(input_fd, &current_char, 1) > 0) {
        if (current_char == '\n' || line_length == (int)sizeof(line_buffer) - 1) {
            line_buffer[line_length] = '\0';

            if (!removed && strcmp(line_buffer, line_to_remove) == 0) {
                removed = 1;
            } else {
                write(output_fd, line_buffer, strlen(line_buffer));
                write(output_fd, "\n", 1);
            }

            line_length = 0;
        } else {
            line_buffer[line_length++] = current_char;
        }
    }

    close(input_fd);
    close(output_fd);

    unlink(history_filename);
    link(temporary_filename, history_filename);
    unlink(temporary_filename);

    clear_terminal();
    write_stdout(CHAT_HEADER);
    write_stdout(SYSTEM_PREFIX);
    write_stdout(MSG_BURN_REMOVED);

    input_fd = open(history_filename, O_RDONLY);
    if (input_fd != -1) {
        char buffer[1024];
        int bytes_read;
        while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0) {
            write(STDOUT_FD, buffer, bytes_read);
        }
        close(input_fd);
    }
}

static void schedule_burn_removal(const char *prefix, char *message_text) {
    char message_copy[MAX_TEXT];
    strcpy(message_copy, message_text);

    strtok(message_copy, " ");
    char *seconds_text = strtok(NULL, " ");
    if (seconds_text == NULL) {
        return;
    }

    int burn_delay = atoi(seconds_text);

    char full_history_line[MAX_TEXT * 2];
    strcpy(full_history_line, prefix);
    strcat(full_history_line, message_text);

    pid_t burn_process_id = fork();
    if (burn_process_id == 0) {
        sleep(burn_delay);
        redraw_history_after_removal(full_history_line);
        exit(0);
    }
}

static void configure_as_first_user(int session_fd) {
    am_first_user = 1;
    outgoing_type = 1;
    incoming_type = 2;

    message_queue_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);

    char queue_id_text[32];
    int length = sprintf(queue_id_text, "%d", message_queue_id);
    write(session_fd, queue_id_text, length);
    close(session_fd);

    write_stdout(SYSTEM_PREFIX);
    write_stdout(MSG_WAITING_FOR_USER2);
}

static int open_existing_session_and_connect(void) {
    am_first_user = 0;
    outgoing_type = 2;
    incoming_type = 1;

    int session_fd = open(SESSION_FILE, O_RDONLY);
    if (session_fd == -1) {
        write_stdout(SYSTEM_PREFIX);
        write_stdout(MSG_SESSION_OPEN_ERROR);
        return -1;
    }

    char queue_id_text[32];
    int bytes_read = read(session_fd, queue_id_text, sizeof(queue_id_text) - 1);
    queue_id_text[bytes_read] = '\0';
    close(session_fd);

    message_queue_id = atoi(queue_id_text);
    write_stdout(SYSTEM_PREFIX);
    write_stdout(MSG_CONNECTED_TO_USER1);

    return 0;
}

static int initialize_session(void) {
    int session_fd = open(SESSION_FILE, O_CREAT | O_EXCL | O_RDWR, 0666);

    if (session_fd != -1) {
        configure_as_first_user(session_fd);
        return 0;
    }

    return open_existing_session_and_connect();
}

static void process_received_message(struct ChatMessage *received_message) {
    if (strncmp(received_message->mtext, COMMAND_SYS_EXIT, strlen(COMMAND_SYS_EXIT)) == 0) {
        kill(getppid(), SIGUSR1);
        exit(0);
    }

    write_stdout(GHOST_PREFIX);
    write_stdout(received_message->mtext);
    write_stdout("\n");

    append_to_history(GHOST_PREFIX, received_message->mtext);

    if (strncmp(received_message->mtext, COMMAND_BURN, strlen(COMMAND_BURN)) == 0) {
        schedule_burn_removal(GHOST_PREFIX, received_message->mtext);
    }
}

static void run_receiver_loop(void) {
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

static void process_outgoing_message(struct ChatMessage *outgoing_message, int bytes_read) {
    if (bytes_read > 0) {
        if (outgoing_message->mtext[bytes_read - 1] == '\n') {
            outgoing_message->mtext[bytes_read - 1] = '\0';
        } else {
            outgoing_message->mtext[bytes_read] = '\0';
        }

        if (strncmp(outgoing_message->mtext, COMMAND_EXIT, strlen(COMMAND_EXIT)) == 0) {
            strcpy(outgoing_message->mtext, COMMAND_SYS_EXIT);
            msgsnd(message_queue_id, outgoing_message, sizeof(outgoing_message->mtext), 0);
            show_chat_closed_screen();
            terminate_program();
        }

        msgsnd(message_queue_id, outgoing_message, sizeof(outgoing_message->mtext), 0);

        append_to_history("", outgoing_message->mtext);

        if (strncmp(outgoing_message->mtext, COMMAND_BURN, strlen(COMMAND_BURN)) == 0) {
            schedule_burn_removal("", outgoing_message->mtext);
        }
    }
}

static void run_sender_loop(void) {
    struct ChatMessage outgoing_message;
    outgoing_message.mtype = outgoing_type;

    while (1) {
        int bytes_read = read(STDIN_FILENO, outgoing_message.mtext, MAX_TEXT - 1);
        process_outgoing_message(&outgoing_message, bytes_read);
    }
}

int main(void) {
    signal(SIGINT, terminate_program);
    signal(SIGUSR1, handle_peer_exit);

    clear_terminal();
    write_stdout(CHAT_HEADER);

    if (initialize_session() != 0) {
        return 1;
    }

    receiver_process_id = fork();

    if (receiver_process_id == 0) {
        run_receiver_loop();
    } else {
        run_sender_loop();
    }

    return 0;
}
