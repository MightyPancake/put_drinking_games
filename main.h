#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include "macros.h"
#include <mpi.h>
#include "comms.h"

void check_thread_support(int provided);
void finalize();
void init_globals();

#endif

