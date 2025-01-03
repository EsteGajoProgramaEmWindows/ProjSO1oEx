#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "client_linked_list.h"

// create linked list node
t_ClientListNode* create_list_node(int client_id){
    t_ClientListNode *new_node = (t_ClientListNode*)malloc(sizeof(t_ClientListNode));
    new_node->client_id = client_id;
    new_node->next = NULL;
    return new_node;
}

// search and return the value of the specified node
int search_list_node(t_ClientListNode *node, int client_id){
    t_ClientListNode *current = node;
    while(current!=NULL){
        if(current->client_id==client_id){
            return current->client_id;
        }
        current = current->next;
    }
    return -1; // not found
}

// append to the end of the list
void append_list_node(t_ClientListNode *node, int client_id){
    t_ClientListNode *current = node;
    while(current->next!=NULL){
        current = current->next;
    }
    current->next = create_list_node(client_id);
}



// search and delete one node from the list
void delete_list_node(t_ClientListNode *node, int client_id){
    t_ClientListNode *current = node;
    t_ClientListNode *previous = NULL;

    while(current!=NULL && current->client_id!=client_id){
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
void delete_list(t_ClientListNode *head) {
    t_ClientListNode *current = head;
    t_ClientListNode *next;

    while(current!=NULL){
        next = current->next;
        free(current);
        current = next;
    }
}


