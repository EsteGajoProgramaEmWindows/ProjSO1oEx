#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "subscriber_linked_list.h"

// create linked list node
t_SubscriberListNode* create_list_node(int fd_notification){
    t_SubscriberListNode *new_node = (t_SubscriberListNode*)malloc(sizeof(t_SubscriberListNode));
    new_node->fd_notification = fd_notification; 
    new_node->next = NULL;
    return new_node;
}

// search and return the value of the specified node
int search_list_node(t_SubscriberListNode *node, int fd_notification){
    t_SubscriberListNode *current = node;
    while(current != NULL){
        if(current->fd_notification == fd_notification){
            return 1; // Return 1 for found
        }
        current = current->next;
    }
    return -1; // Not found
}

// append to the end of the list
void append_list_node(t_SubscriberListNode *node, int fd_notification){
    t_SubscriberListNode *current = node;
    while(current->next != NULL){
        current = current->next;
    }
    current->next = create_list_node(fd_notification);
}

// search and delete one node from the list
void delete_list_node(t_SubscriberListNode *node, int fd_notification){
    t_SubscriberListNode *current = node;
    t_SubscriberListNode *previous = NULL;

    while(current != NULL && (current->fd_notification = fd_notification)){
        previous = current;
        current = current->next;
    }

    if(current == NULL) return;

    if(previous == NULL){
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

    while(current != NULL){
        next = current->next;
        free(current);
        current = next;
    }
}
