#include "main.h"
#include "packet.h"
#include <unistd.h>

int rank, size;

int lamport_clk = 0;
pthread_mutex_t lamport_mutex;

MPI_Datatype packet_type;
MPI_Status status;
packet_t packet;

//Mutexes
pthread_mutex_t groups_mutex;
pthread_mutex_t state_mutex;

int* groups = NULL;
int state = PICK_STATE;
int A = 3;
int ack_count = 0;
int clk = 0;

int* request_queue = NULL;
int queue_count = 0;

//Communication thread
pthread_t comms_thread;

int main(int argc, char** argv){
  // MPI_Status status;
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

  init_packet_type(&packet_type);
  //Get MPI info
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  srand(rank);

  init_globals();
  //Init mutexes
  pthread_mutex_init(&groups_mutex, NULL);
  pthread_mutex_init(&state_mutex, NULL);
  pthread_mutex_init(&lamport_mutex, NULL);
  //Start comms and grouping
  pthread_create(&comms_thread, NULL, comms_main, 0);
  main_loop();

  finalize();
  return 0;
}

bool* group_seen = NULL;
int group_count = 0;

void main_loop(){
    group_seen = (bool*)malloc(size*sizeof(bool));
    while(1){
        pthread_mutex_lock(&state_mutex);
        switch(state){
            case PICK_STATE:
                pick_group();
                break;
            case MONITOR_STATE:
                monitor();
                break;
            case START_STATE:
                start();
                break;
            case REST_STATE:
                nap();
                break;
            case WAIT_STATE:
            case AWAIT_ACK_STATE:
                break;
        }
        pthread_mutex_unlock(&state_mutex);
        // sched_yield();
    }
}

void nap(){
    int nap_time = 1 + rand()%4;
    dbg_print("Napping for %ds...", nap_time);
    join_group(0);
    sleep(nap_time);
}

void start(){
    pthread_mutex_lock(&groups_mutex);
    dbg_print("Starting my group (%d)", my_group);
    int nap_time = 1 + rand()%4;
    dbg_print("Napping for %ds...", nap_time);
    broadcast_packet((packet_t){
        .num1=0,
        .num2=my_group
    }, JOIN_TAG);
    sleep(nap_time);
    handle_queued_requests();
    dbg_print("Joining 0");
    state = PICK_STATE;
    pthread_mutex_unlock(&groups_mutex);
}

void monitor(){
    pthread_mutex_lock(&groups_mutex);
    bool all_picked = true;
    for(int i=0; i<size; i++){
        if (groups[i] == 0){
            all_picked = false;
            break;
        }
    }
    if (all_picked){
        state = AWAIT_ACK_STATE;
        ack_count = 0;
        my_group = groups[0];
        broadcast_packet((packet_t){
            .num1=my_group,
            .num2=++clk
        }, CRIT_SEC_TAG);
    }
    pthread_mutex_unlock(&groups_mutex);
}

#define new_group_probability 0.3
#define starting_group_probability 0.4

void pick_group(){
    pthread_mutex_lock(&groups_mutex);
    int group_count = 0;
    for (int i=0; i<size; i++)
        group_seen[i] = false;
    //Gather data about groups
    for (int i=0; i<size; i++){
        int grp = groups[i];
        if (grp != 0 && group_seen[grp] == false){
            group_count++;
            group_seen[grp] = true;
        }
    }
    pthread_mutex_unlock(&groups_mutex);
    if (randomly(new_group_probability) || group_count==0){
        //Create a new group. Get the first avaible group no
        int new_group = 1;
        for (int i=1; i<size; i++){
            if (!group_seen[i]){
                new_group = i;
                break;
            }
        }
        join_group(new_group);
    }else{
        pthread_mutex_lock(&groups_mutex);
        //Join a group
        int users_checked = 0;
        int picked_group = 0;
        for (int i=rand()%size; users_checked<size; users_checked++){
            picked_group = groups[i];
            i = (i+1)%size;
            if (picked_group == 0) continue;
            else break;
        }
        pthread_mutex_unlock(&groups_mutex);
        if (picked_group != 0){
            if (randomly(starting_group_probability)){
                start_group(picked_group);
            }else{
                join_group(picked_group);
            }
        }
    }
}

void check_thread_support(int provided){
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}

void finalize(){
    //Destroy mutexes
    pthread_mutex_destroy(&groups_mutex);
    pthread_mutex_destroy(&lamport_mutex);
    pthread_mutex_destroy(&state_mutex);
    free(groups);
    free(request_queue);

    pthread_join(comms_thread, NULL);
    free_packet_type(packet_type);
    MPI_Finalize();
}

void init_globals(){
    request_queue = (int*)malloc((10*size)*sizeof(int));
    groups = (int*)malloc(size*sizeof(int));
    for (int i=0; i<size; i++){
        groups[i] = 0;
    }
}

