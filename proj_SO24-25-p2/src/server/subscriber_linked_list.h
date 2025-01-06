#ifndef SUBSCRIBER_LINKED_LIST_H
#define SUBSCRIBER_LINKED_LIST_H

typedef struct SubscriberListNode {
    char *path_notification_fifo;
    struct SubscriberListNode *next;
} t_SubscriberListNode;

t_SubscriberListNode* create_list_node(char *path_notification_fifo);
void append_list_node(t_SubscriberListNode *node, char *path_notification_fifo);

void delete_list_node(t_SubscriberListNode *node, char *path_notification_fifo);

void delete_list(t_SubscriberListNode *head);

int search_list_node(t_SubscriberListNode *node, char *path_notification_fifo);

#endif