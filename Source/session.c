#include "session.h"
#include "chat.h"

void configure_as_first_user(int session_fd) {
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

int open_existing_session_and_connect(void) {
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
    if (bytes_read < 0) {
        close(session_fd);
        write_stdout(SYSTEM_PREFIX);
        write_stdout(MSG_SESSION_OPEN_ERROR);
        return -1;
    }

    queue_id_text[bytes_read] = '\0';
    close(session_fd);

    message_queue_id = atoi(queue_id_text);

    write_stdout(SYSTEM_PREFIX);
    write_stdout(MSG_CONNECTED_TO_USER1);

    return 0;
}

int initialize_session(void) {
    int session_fd = open(SESSION_FILE, O_CREAT | O_EXCL | O_RDWR, 0666);

    if (session_fd != -1) {
        configure_as_first_user(session_fd);
        return 0;
    }

    return open_existing_session_and_connect();
}
