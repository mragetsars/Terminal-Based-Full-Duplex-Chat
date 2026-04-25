#include "history.h"
#include "chat.h"

void build_history_filename(char *buffer) {
    sprintf(buffer, "%s_%d%s",
            HISTORY_FILE_PREFIX,
            am_first_user ? 1 : 2,
            HISTORY_FILE_SUFFIX);
}

void build_temp_history_filename(char *buffer) {
    sprintf(buffer, "%s_%d%s",
            HISTORY_FILE_PREFIX,
            am_first_user ? 1 : 2,
            TEMP_HISTORY_FILE_SUFFIX);
}

void append_to_history(const char *prefix, const char *message_text) {
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

void redraw_history_after_removal(const char *line_to_remove) {
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

void schedule_burn_removal(const char *prefix, char *message_text) {
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
