#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "subscriber_linked_list.h"

// create linked list node
t_SubscriberListNode* create_list_node(char *path_notification_fifo){
    t_SubscriberListNode *new_node = (t_SubscriberListNode*)malloc(sizeof(t_SubscriberListNode));
    strcpy(path_notification_fifo, new_node->path_notification_fifo);
    new_node->next = NULL;
    return new_node;
}

// search and return the value of the specified node
int search_list_node(t_SubscriberListNode *node, char *path_notification_fifo){
    t_SubscriberListNode *current = node;
    while(current!=NULL){
        if(!strcmp(current->path_notification_fifo, path_notification_fifo)){
            return current->path_notification_fifo;
        }
        current = current->next;
    }
    return -1; // not found
}

// append to the end of the list
void append_list_node(t_SubscriberListNode *node, char *path_notification_fifo){
    t_SubscriberListNode *current = node;
    while(current->next!=NULL){
        current = current->next;
    }
    current->next = create_list_node(path_notification_fifo);
}



// search and delete one node from the list
void delete_list_node(t_SubscriberListNode *node, char *path_notification_fifo){
    t_SubscriberListNode *current = node;
    t_SubscriberListNode *previous = NULL;

    while(current!=NULL && strcmp(current->path_notification_fifo, path_notification_fifo)){
        previous = current;
        current = current->next;
    }

    if(current==NULL) return;

    if(previous==NULL){
        node = current->next;
    } else{
        previous->next = current->next;
    }

    free(current);
}

// delete linked list
void delete_list(t_SubscriberListNode *head) {
    t_SubscriberListNode *current = head;
    t_SubscriberListNode *next;

    while(current!=NULL){
        next = current->next;
        free(current);
        current = next;
    }
}


