#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <stdio.h>
#include <errno.h>
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

#define MSG_CONNECTED_TO_USER1 "Connected to User 1's chat!\n"
#define MSG_SESSION_OPEN_ERROR "Error opening session file.\n"
#define MSG_WAITING_FOR_USER2 "Waiting for User 2 to join...\n"
#define MSG_OTHER_USER_LEFT "An EXIT message closed the chat.\n"
#define MSG_BURN_REMOVED "A BURN message just self-destructed.\n"

#define COMMAND_BURN "BURN"
#define COMMAND_EXIT "EXIT"
#define COMMAND_SYS_EXIT "SYS_EXIT"

struct ChatMessage {
    long mtype;
    char mtext[MAX_TEXT];
};

extern int am_first_user;
extern long outgoing_type;
extern long incoming_type;
extern int message_queue_id;
extern pid_t receiver_process_id;

#endif
