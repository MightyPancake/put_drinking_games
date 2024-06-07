#include "macros.h"
#include "packet.h"
extern_globals

#include "comms.h"

packet_t rcvd;

int took_counter = 0;
int back_counter = 0;

void handle_queued_requests(){
  for (int i=0; i<queue_count; i++){
    int dest = request_queue[i];
    send_packet((packet_t){
      .num1 = 1
    }, dest, ACK_TAG);
  }
  queue_count = 0;
}

void queue_request(int sender){
  request_queue[queue_count++] = sender;
}

#define send_ack(DEST, ANS) ({ \
  send_packet((packet_t){ \
      .num1=ANS, \
  }, DEST, ACK_TAG); \
})

void* comms_main(void* data){
  while(1){
    MPI_Recv(&rcvd, 1, packet_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    int sender = status.MPI_SOURCE;
    set_lamport(rcvd.clk);
    dbg_print("Got %s(%d, %d) from %d", tag_to_str(status.MPI_TAG), rcvd.num1, rcvd.num2, sender);
    switch (status.MPI_TAG){
      case JOIN_TAG:
        pthread_mutex_lock(&state_mutex);
        pthread_mutex_lock(&groups_mutex);
        groups[sender] = rcvd.num1;
        if (my_group != 0 && rcvd.num2 == my_group){
          state = REST_STATE;
        }
        pthread_mutex_unlock(&groups_mutex);
        pthread_mutex_unlock(&state_mutex);
        break;
      case CRIT_SEC_TAG:
        pthread_mutex_lock(&state_mutex);
        pthread_mutex_lock(&groups_mutex);
        groups[sender] = rcvd.num1;
        if (state != AWAIT_ACK_STATE){
          send_ack(sender, 1);
          pthread_mutex_unlock(&groups_mutex);
          pthread_mutex_unlock(&state_mutex);
          break;
        }
        bool prio_mine = (clk<rcvd.num2 || (clk==rcvd.num2 && rank<sender));
        if (my_group==rcvd.num1){
          if (prio_mine){
            send_ack(sender, 0);
          }else{
            send_ack(sender, 1);
            handle_queued_requests();
            state = WAIT_STATE;
          }
        }else{
          if (prio_mine){
            queue_request(sender);
          }else{
            send_ack(sender, 1);
          }
        }
        pthread_mutex_unlock(&groups_mutex);
        pthread_mutex_unlock(&state_mutex);
        break;
      case ACK_TAG:
        pthread_mutex_lock(&state_mutex);
        if (rcvd.num1){
          ack_count++;
          if (ack_count == size-1-A){
            state = START_STATE;
          }
        }else{
          handle_queued_requests();
          state = WAIT_STATE;
        }
        pthread_mutex_unlock(&state_mutex);
        break;
      default:
        dbg_print("GOT UNKNOWN TAG (%d)!", status.MPI_TAG);
        break;
    }
  }
  return NULL;
}

