#include "macros.h"
#include "packet.h"
extern_globals

#include "grouping.h"

#define new_group_probability 0.3
#define starting_group_probability 0.4

bool* group_seen = NULL;

void grouping_main(){
  group_seen = (bool*)malloc(size*sizeof(bool));
  while(1){
    if (zzz_time){
      int nap_time = 0 + rand()%3;
      dbg_print("Sleeping for %ds...", nap_time);
      sleep(nap_time);
      zzz_time = false;
      group_picked = false;
      if (i_start){
        pthread_mutex_lock(&groups_mutex);
        i_start = false;
        my_group = 0;
        // group_picked = true;
        broadcast_packet((packet_t){
          .num1=0, //group 0
          .num2=1 //Increase local A
        }, JOIN_TAG);
        pthread_mutex_unlock(&groups_mutex);
      }
    }
    //Skip if waiting for ack
    if (awaiting_ack) continue;
    //Monitor for edge cases after group was picked
    if (group_picked)
      monitor();
    else
      pick_group();
  }
}

void monitor(){
  //Look if all groups are locked
  if (rank != size-1 || awaiting_ack) return;
  // dbg_print("Monitoring for edge cases...");
  bool all_booked = true;
  pthread_mutex_lock(&groups_mutex);
  for (int i=0; i<size; i++){
    if (groups[i] == 0){
      // dbg_print("P%d hasn't join yet!", i);
      all_booked = false;
      break;
    }
  }
  pthread_mutex_unlock(&groups_mutex);
  //If all groups are assigned (and probably didnt start)
  // Join the first one and start it!
  if (DISPLAY_GROUPS && rank == size-1) display_groups();
  if (all_booked){
    printf("Edge case; No start. Joining P0 and starting!\n");
    start_group(groups[0]);
  }else{
    // dbg_print("No edge case");
  }
}

void pick_group(){
  int group_count = 0;
  for (int i=0; i<size; i++)
    group_seen[i] = false;
  
  //Gather data about groups
  pthread_mutex_lock(&groups_mutex);
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
    //Join a group
    int users_checked = 0;
    for (int i=rand()%size; users_checked<size; users_checked++){
      pthread_mutex_lock(&groups_mutex);
      int picked_group = groups[i];
      pthread_mutex_unlock(&groups_mutex);
      i = (i+1)%size;
      if (picked_group == 0) continue;
      if (randomly(starting_group_probability)){
        start_group(picked_group);
      }else{
        join_group(picked_group);
      }
      break;
    }
  }
}

