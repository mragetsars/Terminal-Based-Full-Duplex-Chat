#ifndef SIGNALS_H
#define SIGNALS_H

#include "common.h"

void handle_sigint(int sig);
void handle_peer_exit(int sig);

#endif
