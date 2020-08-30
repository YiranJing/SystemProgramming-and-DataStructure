#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "channel.h"


/*
  Retrieve data that exist within the channel
*/
void channel_init(struct receiver* recv, struct sender* sender,
  size_t msg_sz) {

    if (recv == NULL || sender == NULL || msg_sz == NULL) {
      return;
    }

    // recvier and sender must agree on a size of messages during initialisation
    int channel[NUM_FD];
    pipe(channel);

    recv->read = channel[0];
    recv->msg_sz = msg_sz;

    sender->write = channel[1];
    sender->msg_sz = msg_sz;
}

/*
  Retrieve the next data available within the channel
  Must block if no data exists within the cnannel
*/
void channel_get(struct receiver* channel, void* data) {

  if (data == NULL || channel == NULL){
    return;
  }

  // just move data from receiver to the given parameter "data"
  read(channel->read, data, channel->msg_sz);

}

// send function no need to be block (concurrent)
void channel_send(struct sender* channel, void* data) {

  if (data == NULL || channel == NULL){
    return;
  }

  write(channel->write, data, channel->msg_sz);

}

/*
  Copy src to dest
*/
void sender_dup(struct sender* dest, struct sender* src) {

  if (src== NULL || dest == NULL){
    return;
  }

  dest->write = src->write;
  dest->msg_sz = src->msg_sz;

}

void channel_destroy(struct receiver* recv, struct sender* sender) {
  /*
   if (recv != NULL) {
     close(recv->read);
   }
   if (sender != NULL) {
     close(sender->write);
   }
   */
   return;
}
