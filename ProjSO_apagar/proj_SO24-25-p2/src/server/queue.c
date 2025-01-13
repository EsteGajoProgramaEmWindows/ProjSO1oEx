#include "queue.h"
#include <stdio.h>
#include <errno.h> // Include errno for error numbers

t_queue* create_queue() {
    t_queue* new_queue = (t_queue*)malloc(sizeof(t_queue));
    if (new_queue == NULL) {
        perror("Failed to allocate memory for new queue");
        exit(EXIT_FAILURE);
    }
    new_queue->head = NULL;
    new_queue->tail = NULL;

    if (pthread_mutex_init(&new_queue->lock, NULL) != 0) {
        perror("Failed to initialize mutex");
        free(new_queue);
        exit(EXIT_FAILURE);
    }

    return new_queue;
}

int isEmpty(t_queue* queue) {
    return queue->head == NULL;
}

void enqueue(t_queue* queue, char* response_fifo, char* request_fifo, char* notification_fifo) {
    printf("Valores : %s : %s : %s :\n", response_fifo, request_fifo, notification_fifo);
    t_node_queue* new_node = (t_node_queue*)malloc(sizeof(t_node_queue));
    if (new_node == NULL) {
        perror("Failed to allocate memory for new node");
        return;
    }

    strncpy(new_node->response_fifo_name, response_fifo, 39);
    new_node->response_fifo_name[39] = '\0';
    strncpy(new_node->request_fifo_name, request_fifo, 39);
    new_node->request_fifo_name[39] = '\0';
    strncpy(new_node->notification_fifo_name, notification_fifo, 39);
    new_node->notification_fifo_name[39] = '\0';

    new_node->next = NULL;

    printf("Attempting to lock mutex for enqueue\n");
    if (pthread_mutex_lock(&queue->lock) != 0) {
        perror("Failed to lock mutex in enqueue");
    }
    
    if (queue->tail == NULL) {
        queue->head = new_node;
    } else {
        queue->tail->next = new_node;
    }
    queue->tail = new_node;
    
    printf("Unlocking mutex after enqueue\n");
    if (pthread_mutex_unlock(&queue->lock) != 0) {
        perror("Failed to unlock mutex in enqueue");
    }
}

t_node_queue* pop(t_queue* queue) {
    printf("Entrou POP EIEIE\n");
    printf("Attempting to lock mutex for pop\n");
    if (pthread_mutex_lock(&queue->lock) != 0) {
        perror("Failed to lock mutex in pop");
    }
    if (isEmpty(queue)) {
        printf("Queue is empty in pop\n");
        if (pthread_mutex_unlock(&queue->lock) != 0) {
            perror("Failed to unlock mutex in pop");
        }
        return NULL;
    }
    t_node_queue* data_queue = (t_node_queue*)malloc(sizeof(t_node_queue));
    if (data_queue == NULL) {
        perror("Failed to allocate memory for data queue");
        if (pthread_mutex_unlock(&queue->lock) != 0) {
            perror("Failed to unlock mutex in pop");
        }
        return NULL;
    }
    strncpy(data_queue->response_fifo_name, queue->head->response_fifo_name, sizeof(data_queue->response_fifo_name));
    strncpy(data_queue->request_fifo_name, queue->head->request_fifo_name, sizeof(data_queue->request_fifo_name));
    strncpy(data_queue->notification_fifo_name, queue->head->notification_fifo_name, sizeof(data_queue->notification_fifo_name));
    t_node_queue* temp = queue->head;
    queue->head = queue->head->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    free(temp);
    printf("Unlocking mutex after pop\n");
    if (pthread_mutex_unlock(&queue->lock) != 0) {
        perror("Failed to unlock mutex in pop");
    }
    return data_queue;
}

void destroy_queue(t_queue* queue) {
    printf("Attempting to lock mutex for destroy_queue\n");
    if (pthread_mutex_lock(&queue->lock) != 0) {
        perror("Failed to lock mutex in destroy_queue");
    }
    
    t_node_queue* current = queue->head;
    t_node_queue* next_node;

    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }

    printf("Unlocking mutex after destroy_queue\n");
    if (pthread_mutex_unlock(&queue->lock) != 0) {
        perror("Failed to unlock mutex in destroy_queue");
    }
    
    printf("Destroying mutex\n");
    if (pthread_mutex_destroy(&queue->lock) != 0) {
        perror("Failed to destroy mutex");
    }
    
    free(queue);
}