#ifndef SUBSCRIBER_LINKED_LIST_H
#define SUBSCRIBER_LINKED_LIST_H

typedef struct SubscriberListNode {
    int fd_notification;
    struct SubscriberListNode *next;
} t_SubscriberListNode;

t_SubscriberListNode* create_list_node(int fd_notification);
void append_list_node(t_SubscriberListNode *node, int fd_notification);

void delete_list_node(t_SubscriberListNode *node, int fd_notification);

void delete_list(t_SubscriberListNode *head);

int search_list_node(t_SubscriberListNode *node, int fd_notification);

#endif