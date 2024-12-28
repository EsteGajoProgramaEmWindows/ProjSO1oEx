#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_list.h"

// create linked list node
t_ListNode* create_list_node(int value){
    t_ListNode *new_node = (t_ListNode*)malloc(sizeof(t_ListNode));
    new_node->value = value;
    new_node->next = NULL;
    return new_node;
}

// search and return the value of the specified node
int search_list_node(t_ListNode *node, int value){
    t_ListNode *current = node;
    while(current!=NULL){
        if(current->value==value){
            return current->value;
        }
        current = current->next;
    }
    return -1; // not found
}

// append to the end of the list
void append_list_node(t_ListNode *node, int value){
    t_ListNode *current = node;
    while(current->next!=NULL){
        current = current->next;
    }
    current->next = create_list_node(value);
}



// search and delete one node from the list
void delete_list_node(t_ListNode *node, int value){
    t_ListNode *current = node;
    t_ListNode *previous = NULL;

    while(current!=NULL && current->value!=value){
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
void delete_list(t_ListNode *head) {
    t_ListNode *current = head;
    t_ListNode *next;

    while(current!=NULL){
        next = current->next;
        free(current);
        current = next;
    }
}


