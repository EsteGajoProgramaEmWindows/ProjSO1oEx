#include "queue.h"
#include <stdio.h>

t_queue* create_queue() {
    t_queue* new_queue = (t_queue*)malloc(sizeof(t_queue));
    new_queue->head = NULL;
    new_queue->tail = NULL;
    return new_queue;
}

int isEmpty(t_queue* queue) {
    return queue->head == NULL;
}

void enqueue(t_queue* queue,  char* response_fifo,  char* request_fifo,  char* notification_fifo) {
    printf("Valores : %s : %s : %s :\n", response_fifo,request_fifo, notification_fifo);
    //erro aqui
    t_node_queue* new_node = (t_node_queue*)malloc(sizeof(t_node_queue));

    strncpy(new_node->response_fifo_name, response_fifo, 40);

    strncpy(new_node->request_fifo_name, request_fifo, 40);

    strncpy(new_node->notification_fifo_name, notification_fifo, 40);

    new_node->next = NULL;
    if (queue->tail == NULL) {
        queue->head = new_node;
    } else {
        queue->tail->next = new_node;
    }

    queue->tail = new_node;
}

t_node_queue* pop(t_queue *queue) {
    printf("Entrou POP EIEIE\n");
    if(isEmpty(queue)) {
        return NULL;
    }
    t_node_queue *data_queue = (t_node_queue*)malloc(sizeof(t_node_queue));
    //erro aqui

    strncpy(data_queue->response_fifo_name, queue->head->response_fifo_name, 40);
    printf("NO POP CAMINHO: %s\n", queue->head->response_fifo_name);

    strncpy(data_queue->request_fifo_name, queue->head->request_fifo_name, 40);
    printf("NO POP CAMINHO 2: %s\n", queue->head->request_fifo_name);


    strncpy(data_queue->notification_fifo_name, queue->head->notification_fifo_name, 40);
    printf("NO POP CAMINHO 3: %s\n", queue->head->notification_fifo_name);

    t_node_queue* temp = queue->head;
    queue->head = queue->head->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    free(temp);
    return data_queue;
  
}

void destroy_queue(t_queue* queue) {
    t_node_queue* current = queue->head;
    t_node_queue* next_node;

    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }

    free(queue);
}
