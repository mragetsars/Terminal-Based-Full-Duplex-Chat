#ifndef HISTORY_H
#define HISTORY_H

#include "common.h"

void build_history_filename(char *buffer);
void build_temp_history_filename(char *buffer);
void append_to_history(const char *prefix, const char *message_text);
void redraw_history_after_removal(const char *line_to_remove);
void schedule_burn_removal(const char *prefix, char *message_text);

#endif
