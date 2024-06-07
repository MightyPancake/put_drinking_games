#include <stddef.h>
#include <mpi.h>
#include <stdio.h>

typedef struct packet_t {
    int        num1;
    int        num2;
    int        clk;
} packet_t;

//Number of fields in the packet type
#define NITEMS 3

//Join a group. No ack back
#define JOIN_TAG 1
//num1 - group
//num2 - free group (0 if none)

//I want to update A. Ack required
#define CRIT_SEC_TAG 2
//num1 - group to start
//num2 - clk

//You can go into critical section
#define ACK_TAG 3
//num1 - decision (0|1)

void init_packet_type();
void free_packet_type();
void send_packet(packet_t p, int dest, int tag);
void send_packet_dontwait(packet_t p, int dest, int tag);
void broadcast_packet(packet_t p, int tag);
void ask_for_A();
void join_group(int grp_no);
void start_group();


const char* tag_to_str(int tag);

