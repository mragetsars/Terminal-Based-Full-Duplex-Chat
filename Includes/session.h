#ifndef SESSION_H
#define SESSION_H

#include "common.h"

void configure_as_first_user(int session_fd);
int open_existing_session_and_connect(void);
int initialize_session(void);

#endif
