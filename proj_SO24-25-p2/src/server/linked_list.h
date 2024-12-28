#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct ListNode {
    int value;
    struct Node *next;
} t_ListNode;

t_ListNode* create_list_node(int value);
void append_list_node(t_ListNode *node, int value);

void delete_list_node(t_ListNode *node, int value);

void delete_list(t_ListNode *head);

int search_list_node(t_ListNode *node, int value);

#endif