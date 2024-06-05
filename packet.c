#include "macros.h"
#include "packet.h"
extern_globals

void init_packet_type(MPI_Datatype* p_type){
    int       blocklengths[NITEMS] = {1,1};
    MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT};
    MPI_Aint offsets[NITEMS]; 
    offsets[0] = offsetof(packet_t, num1);
    offsets[1] = offsetof(packet_t, num2);
    // offsets[2] = offsetof(packet_t, data);

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, typy, p_type);
    MPI_Type_commit(p_type);
}

void free_packet_type(MPI_Datatype* p_type){
    MPI_Type_free(p_type);
}

void send_packet(packet_t p, int dest, int tag){
    // if DEBUG
        // printf("Sending %s to %d...\n", tag_to_str(tag), dest);
    MPI_Send(&p, 1, packet_type, dest, tag, MPI_COMM_WORLD);
}

void broadcast_packet(packet_t p, int tag){
    // if DEBUG
    //     printf("#%d: Broadcasting %s(group: %d, %d)\n", rank, tag_to_str(tag), p.group, p.starting);
    for (int i=0; i<size; i++){
        if (i==rank) continue;
        MPI_Send(&p, 1, packet_type, i, tag, MPI_COMM_WORLD);
    }
}

const char* tag_to_str(int tag){
    switch (tag){
        case JOIN_TAG: return "JOIN"; break;
        case CRIT_SEC_TAG: return "CRIT_SEC"; break;
        case ACK_TAG: return "ACK"; break;
        case UPDATE_A_TAG: return "UPDATE_A"; break;
        default: return "unknown"; break;
    }
}

void join_group(int grp_no){
    pthread_mutex_lock(&groups_mutex);
    my_group = grp_no;
    // group_picked = true;
    broadcast_packet((packet_t){
    .num1=grp_no,
    .num2=0
    }, JOIN_TAG);
    pthread_mutex_unlock(&groups_mutex);
}

void start_group(int grp){
    i_start = true;
    pthread_mutex_lock(&groups_mutex);
    my_group = grp;
    // group_picked = true;
    a_amp = -1;
    broadcast_packet((packet_t){
        .num1=my_group,
        .num2=++clk
    }, CRIT_SEC_TAG);
    pthread_mutex_unlock(&groups_mutex);
    awaiting_ack = true;
    ack_count = 0;
}


