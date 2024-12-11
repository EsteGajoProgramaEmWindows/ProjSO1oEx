#include "queue.h"

queue_t* create_queue(){
    queue_t* new_queue = (queue_t*)malloc(sizeof(queue_t));
    new_queue -> head = NULL;
    new_queue -> tail = NULL;
    return new_queue;
}

int isEmpty(queue_t* queue){
    if(queue->head == NULL){
        return 1;
    }
    return 0;
}

void enqueue(queue_t *queue, char file_name[]){
    node_queue_t* new_node = (node_queue_t*)malloc(sizeof(node_queue_t));
    strcpy(new_node->file_name, file_name);
    new_node->next = NULL;
    if(queue->tail == NULL){
        queue->head = new_node;
    }
    else{
        queue->tail->next = new_node;
    }

    queue->tail = new_node;
   
}

void pop(char file_name[] , queue_t * queue){
    node_queue_t* temp = queue->head;
    strcpy(file_name, queue->head->file_name);
    if(queue->head->next == NULL){
        queue->tail = NULL;
    }
    
    queue->head = queue->head->next;
    free(temp);

}

void destroy_queue(queue_t *queue) {
    node_queue_t *current = queue->head;
    node_queue_t *next_node;

    while (current != NULL) {
        next_node = current->next;
        free(current);  // Libera o nó atual
        current = next_node;
    }

    free(queue);
}



