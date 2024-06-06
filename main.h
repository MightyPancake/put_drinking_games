#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include "macros.h"
#include <mpi.h>
#include "comms.h"

void check_thread_support(int provided);
void main_loop();
void finalize();
void monitor();
void start();
void init_globals();
void pick_group();
void nap();

void handle_queued_requests();

#endif

