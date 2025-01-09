#ifndef KEY_VALUE_STORE_H
#define KEY_VALUE_STORE_H
#define TABLE_SIZE 26

#include <stddef.h>
#include <pthread.h>
#include "subscriber_linked_list.h"

typedef struct KeyNode {
    char *key;
    char *value;
    struct KeyNode *next;
    t_SubscriberListNode *head;
} KeyNode;

typedef struct HashTable {
    KeyNode *table[TABLE_SIZE];
    pthread_rwlock_t tablelock;
} HashTable;

/// Creates a new KVS hash table.
/// @return Newly created hash table, NULL on failure
struct HashTable *create_hash_table();

int hash(const char *key); 

// Writes a key value pair in the hash table.
// @param ht The hash table.
// @param key The key.
// @param value The value.
// @return 0 if successful.
int write_pair(HashTable *ht, const char *key, const char *value);

// Reads the value of a given key.
// @param ht The hash table.
// @param key The key.
// return the value if found, NULL otherwise.
char* read_pair(HashTable *ht, const char *key);

/// Deletes a pair from the table.
/// @param ht Hash table to read from.
/// @param key Key of the pair to be deleted.
/// @return 0 if the node was deleted successfully, 1 otherwise.
int delete_pair(HashTable *ht, const char *key);

/// Writes into the subscription linked list the notification fifo name associated with the client
/// that subscribed the key
/// @param ht Hash table to write
/// @param key Key of the subscription
/// @param subs_notif_fifo Name of the notification fifo associated with the client
/// @return 0 if the key doesn't exist in the subscriber list, 1 otherwise
int write_subscription(HashTable *ht, const char *key, const char *subs_notif_fifo);

/// Deletes the subscription for a given key from the subscriber linked list
/// @param ht Hash table to delete
/// @param key Key of the subscription 
/// @param subs_notif_fifo Name of the notification fifo associated with the client
/// @return 0 if the subscription existed and was deleted successfully, 1 if the subscription didn't existed
int delete_subscription(HashTable *ht, const char *key, const char *subs_notif_fifo);

/// Frees the hashtable. 
/// @param ht Hash table to be deleted.
void free_table(HashTable *ht);


#endif  // KVS_H
