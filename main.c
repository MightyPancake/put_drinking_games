#include "main.h"
#include "grouping.h"
#include "packet.h"

int rank, size;

MPI_Datatype packet_type;
MPI_Status status;
packet_t packet;

//Groups
pthread_mutex_t groups_mutex;
int* groups = NULL;

//Grupowanie
bool group_picked = false;
bool zzz_time = false;

//Sekcja krytyczna
bool got_all_acks = false;
pthread_mutex_t A_mutex;
int A = 2;
bool awaiting_ack = false;
bool i_start = false;
int a_amp = 0;
int ack_count = 0;
int clk = 0;

//Requests queue
int* awaiting_reqs;
int awaiting_count = 0;

//Communication thread
pthread_t comms_thread;

int main(int argc, char** argv){
  // MPI_Status status;
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  // if DEBUG
  //   check_thread_support(provided);

  init_packet_type(&packet_type);
  //Get MPI info
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  srand(rank);
  // dbg_print("Hello!");

  init_globals();
  //Init mutexes
  pthread_mutex_init(&groups_mutex, NULL);
  pthread_mutex_init(&A_mutex, NULL);
  //Start comms and grouping
  pthread_create( &comms_thread, NULL, comms_main, 0);
  grouping_main();

  finalize();
  return 0;
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

    pthread_join(comms_thread, NULL);
    free_packet_type(packet_type);
    MPI_Finalize();
}

void init_globals(){
    awaiting_reqs = (int*)malloc(size*sizeof(int));
    groups = (int*)malloc(size*sizeof(int));
    for (int i=0; i<size; i++){
        groups[i] = 0;
    }
}

