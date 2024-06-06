#ifndef MACROS_H
#define MACROS_H

#include <mpi.h>
#include <stdbool.h>
#include <pthread.h>

#define extern_globals \
  extern int rank; \
  extern int size; \
  extern MPI_Datatype packet_type; \
  extern MPI_Status status; \
  extern packet_t packet; \
  extern pthread_mutex_t groups_mutex; \
  extern pthread_mutex_t A_mutex; \
  extern pthread_mutex_t state_mutex; \
  extern int* groups; \
  extern int state; \
  extern int A; \
  extern int ack_count; \
  extern int clk; \
  extern int* request_queue; \
  extern int queue_count; \


#define my_group (groups[rank])

#define DEBUG (1)
#define rand_prec 1000
#define randomly(P) (((float)((rand()%rand_prec))/rand_prec)<=(P))


#define RANK_COLOR(R) ({ \
    int RANK = R + 5; \
    int bump = 112; \
    int cutoff = 220; \
    int r = ((RANK * 50) % cutoff) + bump; \
    int g = ((RANK * 80) % cutoff) + bump; \
    int b = ((RANK * 110) % cutoff) + bump; \
    r = r > 255 ? 255 : r; \
    g = g > 255 ? 255 : g; \
    b = b > 255 ? 255 : b; \
    static char color_code[32]; /* Sufficient for "\033[38;2;255;255;255m" */ \
    snprintf(color_code, sizeof(color_code), "\033[38;2;%d;%d;%dm", r, g, b); \
    color_code; \
})

#define dbg_print(format, ...) do { \
    if (!DEBUG) break; \
    const char* color = RANK_COLOR(rank); \
    printf("%s#%d:(A=%d)[%d] " format "\033[0m\n", color, rank, A, clk, ##__VA_ARGS__); \
} while (0)

#define display_groups() { \
  printf("-----groups----\n"); \
  pthread_mutex_lock(&groups_mutex); \
  for (int i=0; i<size; i++){ \
    printf("%d: %d\n", i, groups[i]); \
  } \
  pthread_mutex_unlock(&groups_mutex); \
  printf("---------\n"); \
}

//States
#define PICK_STATE 0
#define AWAIT_ACK_STATE 1
#define MONITOR_STATE 2
#define WAIT_STATE 3
#define REST_STATE 4
#define START_STATE 5

#define set_state(S) ({ \
  pthread_mutex_lock(&state_mutex); \
  state = S; \
  pthread_mutex_unlock(&state_mutex); \
  state; \
})

#endif
