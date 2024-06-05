#ifndef MACROS_H
#define MACROS_H

#include <mpi.h>
#include <stdbool.h>
#include <pthread.h>

#define max(A, B) ((A) > (B) ? (A) : (B))

#define extern_globals \
    extern int* groups; \
    extern int rank; \
    extern int size; \
    extern MPI_Datatype packet_type; \
    extern MPI_Status status; \
    extern bool group_picked; \
    extern pthread_mutex_t groups_mutex; \
    extern bool zzz_time; \
    extern int clk; \
    extern int awaiting_count; \
    extern int* awaiting_reqs; \
    extern bool awaiting_ack; \
    extern int ack_count; \
    extern int A; \
    extern pthread_mutex_t A_mutex; \
    extern int a_amp; \
    extern bool i_start; \
    extern bool got_all_acks; \


#define DEBUG (1)
#define ULTRA_DBG (0)
#define DISPLAY_GROUPS (0)
#define rand_prec 1000
#define randomly(P) (((float)((rand()%rand_prec))/rand_prec)<=(P))

#define print_stats printf("#%d: (im_starting: %d, group: %d, ack_waiting: %d)\n", rank, im_starting, my_group, ack_waiting)

#define my_group groups[rank]

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

#endif
