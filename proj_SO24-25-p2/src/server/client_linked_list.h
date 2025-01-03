#ifndef CLIENT_LINKED_LIST_H
#define CLIENT_LINKED_LIST_H

typedef struct ClientListNode {
    int client_id;
    struct ClientListNode *next;
} t_ClientListNode;

t_ClientListNode* create_list_node(int client_id);
void append_list_node(t_ClientListNode *node, int client_id);

void delete_list_node(t_ClientListNode *node, int cleint_id);

void delete_list(t_ClientListNode *head);

int search_list_node(t_ClientListNode *node, int client_id);

#endif