#include "macros.h"
#include "packet.h"
extern_globals

#include "comms.h"

packet_t rcvd;

void handle_queued_requests(){
  if (awaiting_ack) return;
  for (int i=0; i<awaiting_count; i++){
    int req = awaiting_reqs[i];
    send_packet((packet_t){
      .num1 = 1
    }, req, ACK_TAG);
  }
  awaiting_count = 0;
}

void queue_request(int sender){
  awaiting_reqs[awaiting_count++] = sender;
}

void* comms_main(void* data){
  while(1){
    go_into_crit();
    handle_queued_requests();
    MPI_Recv(&rcvd, 1, packet_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    int sender = status.MPI_SOURCE;
    dbg_print("Got %s(%d, %d) from %d.", tag_to_str(status.MPI_TAG), rcvd.num1, rcvd.num2, sender);
    switch (status.MPI_TAG){
      case JOIN_TAG:
        pthread_mutex_lock(&groups_mutex);
        groups[sender] = rcvd.num1;
        pthread_mutex_unlock(&groups_mutex);
        if (rcvd.num2){ //Increase A
          dbg_print("_____Increasing A by %d", rcvd.num2);
          pthread_mutex_lock(&A_mutex);
          A += rcvd.num2;
          pthread_mutex_unlock(&A_mutex);
        }
        break;
      case CRIT_SEC_TAG:
        if (rcvd.num1) groups[sender] = rcvd.num1;
        if (awaiting_ack && rcvd.num1 == my_group){
          if (rank>sender){
            send_packet((packet_t){
              .num1=0
            }, sender, ACK_TAG);
            dbg_print("You cannot!");
          }else{
            send_packet((packet_t){
              .num1=1
            }, sender, ACK_TAG);
            awaiting_ack = false;
            ack_count = 0;
            dbg_print("Ok, go :/");
          }
        }else if (awaiting_ack && (clk<rcvd.num2 || (clk==rcvd.num2 && rank>sender))){
          queue_request(sender);
          dbg_print("Queued.");
        }else{
          dbg_print("Accepted.");
            send_packet((packet_t){
              .num1=1
            }, sender, ACK_TAG); 
        }
        break;
      case ACK_TAG:
        // if (got_all_acks) break;
        if (!awaiting_ack) break;
        if (rcvd.num1){
          ack_count++;
          dbg_print("Got ack (%d/%d)", ack_count, size-1);
          if (ack_count == size-1){
            got_all_acks = true;
          }
        }else{
          i_start = false;
          awaiting_ack = false;
          ack_count = 0;
        }
        break;
      case UPDATE_A_TAG:
        pthread_mutex_lock(&A_mutex);
        A += rcvd.num1;
        pthread_mutex_unlock(&A_mutex);
        if (rcvd.num2 && rcvd.num2 == my_group){
          join_group(0);
          zzz_time = true;
        }
        break;
      default:
        dbg_print("GOT UNKNOWN TAG (%d)!", status.MPI_TAG);
        break;
    }
  }
  return NULL;
}

void go_into_crit(){
  if (!awaiting_ack) return;
  if (!got_all_acks) return;
  pthread_mutex_lock(&A_mutex);
  if (A<=0) {
    dbg_print("Not enough A yet!");
    pthread_mutex_unlock(&A_mutex);
    return;
  }else{
    dbg_print("Got enough A!");
  }
  A += a_amp;
  pthread_mutex_unlock(&A_mutex);
  got_all_acks = false;
  pthread_mutex_lock(&groups_mutex);
  dbg_print("Startng my group (%d)", my_group);
  broadcast_packet((packet_t){
    .num1=a_amp,
    .num2 = my_group
  }, UPDATE_A_TAG);
  pthread_mutex_unlock(&groups_mutex);
  awaiting_ack = false;
  zzz_time = true;
}

