#ifndef CHAT_H
#define CHAT_H

#include "common.h"

void write_stdout(const char *text);
void clear_terminal(void);
void show_chat_closed_screen(void);
void terminate_program(void);

void process_received_message(struct ChatMessage *received_message);
void run_receiver_loop(void);

void process_outgoing_message(struct ChatMessage *outgoing_message, int bytes_read);
void run_sender_loop(void);

#endif
