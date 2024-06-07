#include "macros.h"
#include "packet.h"
extern_globals

void init_packet_type(MPI_Datatype* p_type){
    int       blocklengths[NITEMS] = {1,1,1};
    MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint offsets[NITEMS]; 
    offsets[0] = offsetof(packet_t, num1);
    offsets[1] = offsetof(packet_t, num2);
    offsets[2] = offsetof(packet_t, clk);

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, typy, p_type);
    MPI_Type_commit(p_type);
}

void free_packet_type(MPI_Datatype* p_type){
    MPI_Type_free(p_type);
}

void send_packet(packet_t p, int dest, int tag){
    // MPI_Send(&p, 1, packet_type, dest, tag, MPI_COMM_WORLD);
    send_packet_dontwait(p, dest, tag);
}

void send_packet_dontwait(packet_t p, int dest, int tag){
    MPI_Request request;
    pthread_mutex_lock(&lamport_mutex);
    p.clk = ++lamport_clk;   
    MPI_Isend(&p, 1, packet_type, dest, tag, MPI_COMM_WORLD, &request);
    MPI_Request_free(&request);
    pthread_mutex_unlock(&lamport_mutex);
}

void broadcast_packet(packet_t p, int tag){
    pthread_mutex_lock(&lamport_mutex);
    p.clk = ++lamport_clk;   
    for (int i=0; i<size; i++){
        if (i==rank) continue;
        MPI_Request request;
        MPI_Isend(&p, 1, packet_type, i, tag, MPI_COMM_WORLD, &request);
        MPI_Request_free(&request);
    }
  pthread_mutex_unlock(&lamport_mutex);
}

const char* tag_to_str(int tag){
    switch (tag){
        case JOIN_TAG: return "JOIN"; break;
        case CRIT_SEC_TAG: return "CRIT_SEC"; break;
        case ACK_TAG: return "ACK"; break;
        default: return "unknown"; break;
    }
}

void join_group(int grp_no){
    pthread_mutex_lock(&groups_mutex);
    my_group = grp_no;
    if (grp_no == 0){
        state = PICK_STATE;
    }else if (rank == size-1){
        state = MONITOR_STATE;
    }else{
        state = WAIT_STATE;
    }
    broadcast_packet((packet_t){
        .num1=grp_no,
        .num2=0
    }, JOIN_TAG);
    dbg_print("Joining %d", grp_no);
    pthread_mutex_unlock(&groups_mutex);
}

void start_group(int grp){
    pthread_mutex_lock(&groups_mutex);
    state = AWAIT_ACK_STATE;
    ack_count = 0;
    broadcast_packet((packet_t){
        .num1=my_group,
        .num2=++clk
    }, CRIT_SEC_TAG);
    dbg_print("Starting %d", grp);
    pthread_mutex_unlock(&groups_mutex);
}


