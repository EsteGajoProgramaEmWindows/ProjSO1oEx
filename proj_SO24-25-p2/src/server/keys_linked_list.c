#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "keys_linked_list.h"


// create linked list node
t_KeyListNode* create_list_node(char *key){
    t_KeyListNode *new_node = (t_KeyListNode*)malloc(sizeof(t_KeyListNode));
    strcpy(key, new_node->key);
    new_node->next = NULL;
    return new_node;
}

// search and return the value of the specified node
int search_list_node(t_KeyListNode *node, char *key){
    t_KeyListNode *current = node;
    while(current!=NULL){
        if(!strcmp(current->key,key)){
            return current->key;
        }
        current = current->next;
    }
    return -1; // not found
}

// append to the end of the list
void append_list_node(t_KeyListNode *node, char *key){
    t_KeyListNode *current = node;
    while(current->next!=NULL){
        current = current->next;
    }
    current->next = create_list_node(key);
}



// search and delete one node from the list
void delete_list_node(t_KeyListNode *node, char *path_notification_fifo){
    t_KeyListNode *current = node;
    t_KeyListNode *previous = NULL;

    while(current!=NULL && strcmp(current->key, path_notification_fifo)){
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
void delete_list(t_KeyListNode *head) {
    t_KeyListNode *current = head;
    t_KeyListNode *next;

    while(current!=NULL){
        next = current->next;
        free(current);
        current = next;
    }
}


