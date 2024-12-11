#ifndef _QUEUE_H
#define _QUEUE_H

#include "constants.h"
#include <stdlib.h>
#include <string.h>

typedef struct node_queue{
    char file_name[MAX_JOB_FILE_NAME_SIZE];
    struct node_queue * next;
}node_queue_t;

typedef struct queue{
    struct node_queue *head;
    struct node_queue *tail;
}queue_t;

queue_t* create_queue();

int isEmpty(queue_t* queue);

void enqueue(queue_t *queue, char file_name[]);

void pop(char file_name[] , queue_t * queue);

#endif 