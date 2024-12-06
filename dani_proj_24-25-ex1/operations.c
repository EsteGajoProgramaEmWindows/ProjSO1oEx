#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include "kvs.h"
#include "operations.h"   
#include "constants.h"
#include <unistd.h>

static struct HashTable* kvs_table = NULL;


/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
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

  free_table(kvs_table);
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]) {

  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  for (size_t i = 0; i < num_pairs; i++) {
    if (write_pair(kvs_table, keys[i], values[i]) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", keys[i], values[i]);
    }
  }

  return 0;
}

int kvs_read(int fd, size_t num_pairs, char keys[][MAX_STRING_SIZE]) {

  
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  write_to_file(fd, "[(");
  for (size_t i = 0; i < num_pairs; i++) {
    char* result = read_pair(kvs_table, keys[i]);
    if (result == NULL) {
      write_to_file(fd, strcat(keys[i], ",KVSERROR"));
    } else {
      char *text = (char*)malloc(strlen(keys[i]) + strlen(result) + 2);
      strcpy(text, "");
      text = strcat(text, keys[i]);
      text = strcat(text, ",");
      text = strcat(text, result);
      write_to_file(fd, text);
      free(text);
    }
    free(result);
  }
  write_to_file(fd, ")]\n");
  return 0;
}

int kvs_delete(int fd, size_t num_pairs, char keys[][MAX_STRING_SIZE]) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }
  int aux = 0;

  for (size_t i = 0; i < num_pairs; i++) {
    if (delete_pair(kvs_table, keys[i]) != 0) {
      if (!aux) {
        write_to_file(fd,"[(");
        aux = 1;
      }
      char *text = (char*)malloc(strlen(keys[i]) + strlen("KVSMISSING") + 2); 
      strcpy(text, "");
      text = strcat(text, keys[i]); 
      text = strcat(text, ",KVSMISSING");
      write_to_file(fd, text);
      free(text);
    }
  }

  if (aux) {
    write_to_file(fd, ")]\n");
  }
  return 0;
}

void kvs_show(int fd) {
  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
      char *text = malloc(strlen(keyNode->key) + strlen(keyNode->value) + 6);
      strcpy(text, "");
      text = strcat(text, "(");
      text = strcat(text, keyNode->key);
      text = strcat(text, ", ");
      text = strcat(text, keyNode->value);
      text = strcat(text, ")\n");
      keyNode = keyNode->next; // Move to the next node
      write_to_file(fd, text);
      free(text);
    }
  }
}

int kvs_backup(const char *job_file_path) {
  char path_suffix[MAX_JOB_FILE_NAME_SIZE];
  char output_file_path[MAX_JOB_FILE_NAME_SIZE]; 
  strncpy(output_file_path, job_file_path, strlen(job_file_path)-4);
  output_file_path[strlen(job_file_path) - 4] = '\0';

      int backup_number = 1;
    struct dirent *entry;
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("Failed to open directory");
        return 1;
    }

    // Iterate over the directory to find the highest backup number
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, output_file_path) == entry->d_name && strstr(entry->d_name, ".bck") != NULL) {
            int current_backup_number;
            if (sscanf(entry->d_name + strlen(output_file_path) + 1, "%d.bck", &current_backup_number) == 1) {
                if (current_backup_number >= backup_number) {
                    backup_number = current_backup_number + 1;
                }
            }
        }
    }
    closedir(dir);
  
  sprintf(path_suffix, "-%d.bck", backup_number);
  strcat(output_file_path, path_suffix);
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
