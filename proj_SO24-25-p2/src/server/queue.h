#ifndef _QUEUE_H
#define _QUEUE_H

#include "constants.h"
#include <stdlib.h>
#include <string.h>

typedef struct node_queue{
    char response_fifo_name[40];
    char request_fifo_name[40];
    char notification_fifo_name[40];
    struct node_queue * next;
}t_node_queue;

typedef struct queue{
    struct node_queue *head;
    struct node_queue *tail;
}t_queue;

t_queue* create_queue();

int isEmpty(t_queue* queue);

void enqueue(t_queue* queue,  char* response_fifo,  char* request_fifo,  char* notification_fifo);
t_queue* pop(t_queue* queue);

void destroy_queue(t_queue *queue);

#endif 