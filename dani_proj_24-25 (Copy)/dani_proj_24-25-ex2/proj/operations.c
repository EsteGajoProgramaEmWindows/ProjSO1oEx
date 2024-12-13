#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include "kvs.h"
#include "operations.h"   
#include "constants.h"


static struct HashTable* kvs_table = NULL;
pthread_rwlock_t lock_key[TABLE_SIZE];


/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

// Function to sort the indices array
int compare_indixes(const void *a, const void *b) {
    int int_a = *(const int *)a;
    int int_b = *(const int *)b;
    return (int_a > int_b) - (int_a < int_b); 
}
int compare_keys(const void *a, const void *b) {
    const char (*key_a)[MAX_STRING_SIZE] = a;
    const char (*key_b)[MAX_STRING_SIZE] = b;
    return strcmp(*key_a, *key_b);
}

void write_to_file(int fd, const char *data){
      size_t len = strlen(data);
      ssize_t written = 0;

      while (written < (ssize_t) len){
        ssize_t ret = write(fd, data + written, (len - (size_t)(written)));
        if((int)ret == -1){
          perror("Failed to write to file");
          close(fd);
          return;
        }
        written = written + ret;
      }
}

int kvs_init() {
  for(int i = 0; i < TABLE_SIZE; i++) {
    pthread_rwlock_init(&lock_key[i], NULL);
  }
  if (kvs_table != NULL) {
    fprintf(stderr, "KVS state has already been initialized\n");
    return 1;
  }

  kvs_table = create_hash_table();
  return kvs_table == NULL;
}

int kvs_terminate() {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  for(int i = 0; i < TABLE_SIZE; i++) {
    pthread_rwlock_destroy(&lock_key[i]);
  }
  free_table(kvs_table);
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]) {

  for (int i = 0; i < MAX_TABLE_INDEX; i++){
    qsort(keys, num_pairs, MAX_STRING_SIZE, compare_keys);
    pthread_rwlock_wrlock(&kvs_table->lock_key[i]);
  }

  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  for (size_t i = 0; i < num_pairs; i++) {
    if (write_pair(kvs_table, keys[i], values[i]) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", keys[i], values[i]);
    }
  }

  for(int i = 0; i < MAX_TABLE_INDEX; i++) {
    pthread_rwlock_unlock(&kvs_table->lock_key[i]);
  }

  return 0;
}

int kvs_read(int fd, size_t num_pairs, char keys[][MAX_STRING_SIZE]) {

  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  write_to_file(fd, "[");


  for (int i = 0; i < MAX_TABLE_INDEX; i++) {
    qsort(keys, num_pairs, MAX_STRING_SIZE, compare_keys);
    pthread_rwlock_rdlock(&kvs_table->lock_key[i]);
  }
  
  for (size_t i = 0; i < num_pairs; i++) {
    // Compute the index based on the key (assuming hash function is used)
    int index = hash(keys[i]) % TABLE_SIZE;

    // Read the key-value pair
    char* result = read_pair(kvs_table, keys[i]);
    
    // Optionally, sort keys before processing them
    qsort(keys, num_pairs, MAX_STRING_SIZE, compare_keys);
    
    if (result == NULL) {
      write_to_file(fd, "(");
      write_to_file(fd, strcat(keys[i], ",KVSERROR"));
      write_to_file(fd, ")");
    } else {
      char *text = (char*)malloc(strlen(keys[i]) + strlen(result) + 2);
      strcpy(text, "(");
      text = strcat(text, keys[i]);
      text = strcat(text, ",");
      text = strcat(text, result);
      write_to_file(fd, text);
      free(text);
    }

    // Free the result after processing
    free(result);

  }

  for(int i = 0; i < MAX_TABLE_INDEX; i++) {
    pthread_rwlock_unlock(&kvs_table->lock_key[i]);
  }

  write_to_file(fd, "]\n");
  
  return 0;
}

int kvs_delete(int fd, size_t num_pairs, char keys[][MAX_STRING_SIZE]) {
  
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  int aux = 0;

  for (int i = 0; i < MAX_TABLE_INDEX; i++) {
    qsort(keys, num_pairs, MAX_STRING_SIZE, compare_keys); // Sort keys before processing them
    pthread_rwlock_rdlock(&kvs_table->lock_key[i]);
  }

  for (size_t i = 0; i < num_pairs; i++) {
    // Before deleting, acquire the write lock (write lock)
    int index = hash(keys[i]) % TABLE_SIZE; // Assumes 'hash' is the function used to map the key to the table


    // Try to delete the key-value pair
    if (delete_pair(kvs_table, keys[i]) != 0) {
      if (!aux) {
        write_to_file(fd, "[");
        aux = 1;
      }

      // If deletion failed, write the key with "KVSMISSING"
      char *text = (char*)malloc(strlen(keys[i]) + strlen("KVSMISSING") + 2);
      strcpy(text, "(");
      text = strcat(text, keys[i]);
      text = strcat(text, ",KVSMISSING)");
      write_to_file(fd, text);
      free(text);
    }

  
  }
  // Release the write lock (unlock) after processing all keys
  for(int i = 0; i < MAX_TABLE_INDEX; i++) {
    pthread_rwlock_unlock(&kvs_table->lock_key[i]);
  }

  if (aux) {
    write_to_file(fd, "]\n");
  }

  return 0;
}


int kvs_backup(const char *job_file_path, int backup_atual) {
    char base_path[MAX_JOB_FILE_NAME_SIZE];
    char output_file_path[2 * MAX_JOB_FILE_NAME_SIZE]; // Increased buffer size

    // Remove the last 4 characters to strip the extension
    strncpy(base_path, job_file_path, strlen(job_file_path) - 4);
    base_path[strlen(job_file_path) - 4] = '\0';
    
    // Create backup file with the next available number
    snprintf(output_file_path, sizeof(output_file_path), "%s-%d.bck", base_path, backup_atual);
    int output_fd = open(output_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd < 0) {
        perror("Failed to create backup file");
        return 1;
    }
    kvs_show(output_fd);
    close(output_fd);
    return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}
