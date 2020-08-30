#ifndef SRC_CHANNEL_H
#define SRC_CHANNEL_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define NUM_FD (2)

#include <stdlib.h>

struct sender{

  int write; // write end
  size_t msg_sz;
};

struct receiver{

  int read; // read end
  size_t msg_sz;
};

void channel_init(struct receiver* recv, struct sender* sender,
  size_t msg_sz);

void channel_get(struct receiver* channel, void* data);

void channel_send(struct sender* channel, void* data);

void sender_dup(struct sender* dest, struct sender* src);

void channel_destroy(struct receiver* recv, struct sender* sender);

#endif
