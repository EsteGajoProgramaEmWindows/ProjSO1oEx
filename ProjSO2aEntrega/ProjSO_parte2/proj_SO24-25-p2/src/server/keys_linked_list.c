#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "keys_linked_list.h"


// create linked list node
t_KeyListNode* create_list_node_key(char key[41]){
    t_KeyListNode *new_node = (t_KeyListNode*)malloc(sizeof(t_KeyListNode));
    strcpy(new_node->key, key);
    new_node->next = NULL;
    return new_node;
}

/*
// search and return the value of the specified node
char* search_list_node_key(t_KeyListNode *node, char *key){
    t_KeyListNode *current = node;
    while(current!=NULL){
        if(!strcmp(current->key,key)){
            return current->key;
        }
        current = current->next;
    }
    return NULL; // not found
}
*/

// append to the end of the list
void append_list_node_key(t_KeyListNode *node, char key[41]){
    t_KeyListNode *current = node;
    while(current->next!=NULL){
        current = current->next;
    }
    current->next = create_list_node_key(key);
}

// search and delete one node from the list
void delete_list_node_key(t_KeyListNode *node, char key[41]){
    t_KeyListNode *current = node;
    t_KeyListNode *previous = NULL;

    while(current!=NULL && strcmp(current->key, key)){
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
void delete_list_key(t_KeyListNode *head) {
    t_KeyListNode *current = head;
    t_KeyListNode *next;

    while(current!=NULL){
        next = current->next;
        free(current);
        current = next;
    }
}


