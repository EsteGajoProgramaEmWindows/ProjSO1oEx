#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include "constants.h"
#include "parser.h"
#include "operations.h"

  void process_job_file(const char *job_file_path) {
    int input_fd = open(job_file_path, O_RDONLY);
    if (input_fd < 0) {
        perror("Failed to open job file");
        return;
    }

    char output_file_path[MAX_JOB_FILE_NAME_SIZE]; 
    strncpy(output_file_path, job_file_path, strlen(job_file_path)-4);
    strcat(output_file_path, ".out");
    int output_fd = open(output_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd < 0) {
        perror("Failed to create output file");
        close(input_fd);
        return;
    }

  char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
  char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
  unsigned int delay;
  size_t num_pairs;


  while (1) {
    switch (get_next(input_fd)) {
      case CMD_WRITE:
        printf("escreve\n");
        num_pairs = parse_write(input_fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);
        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_write(num_pairs, keys, values)) {
           fprintf(stderr, "Failed to write pair\n");
        }
        break;

      case CMD_READ:
        printf("le\n");
        num_pairs = parse_read_delete(input_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);
        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_read(output_fd, num_pairs, keys)) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");            
        }
        break;

      case CMD_DELETE:
        num_pairs = parse_read_delete(input_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_delete(output_fd, num_pairs, keys) == 1) {
            fprintf(stderr, "Failed to delete pair\n");
        }
        break;

      case CMD_SHOW:
        kvs_show(output_fd);
        break;

      case CMD_WAIT:
        if (parse_wait(input_fd, &delay, NULL) == -1) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          break;
        }

        if (delay > 0) {
          write_to_file(output_fd,"Waiting...\n");
          kvs_wait(delay);
        }
        break;

      case CMD_BACKUP:

        printf("baka\n");
        pid_t pid = fork();
        if(pid==0){
        if (kvs_backup(job_file_path) == 1) {
           fprintf(stderr, "Failed to perform backup.\n");
          }
          exit(0);
        }
        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command I. See HELP for usage\n");
        break;

      case CMD_HELP:
        const char *help_msg =
            "Available commands:\n"
            "  WRITE [(key,value)(key2,value2),...]\n"
            "  READ [key,key2,...]\n"
            "  DELETE [key,key2,...]\n"
            "  SHOW\n"
            "  WAIT <delay_ms>\n"
            "  BACKUP\n"
            "  HELP\n";
        write_to_file(output_fd, help_msg);

        break;
        
      case CMD_EMPTY:
        printf("comenta\n");
        break;

      case EOC:
        close(input_fd);
        close(output_fd);
        return;
    }
  }
}

int parse_arguments(int argc, char *argv[], char *directory, int *max_backups) {
  if (argc != 3) {
    return -1;
  }

  strcpy(directory, argv[1]);
  *max_backups = atoi(argv[2]);

  return 0;
}

int main(int argc, char *argv[]) {
  char directory[MAX_JOB_FILE_NAME_SIZE];
  int max_backups;

  if (parse_arguments(argc, argv, directory, &max_backups) != 0) {
        return 1;
    }

  if (kvs_init()) {
    return 1;
  }

  DIR *dir = opendir(directory);
    if (!dir) {
        perror("Failed to open directory");
        return 1;
    }
  
  struct dirent *entry;
  // Iterate over each entry in the directory using readdir()
  while ((entry = readdir(dir)) != NULL) {
    // Check if the entry is a regular file and ends with ".job"
    if (strstr(entry->d_name, ".job") != NULL) {
        char job_file_path[2 * MAX_JOB_FILE_NAME_SIZE];  // Increased buffer size for safety
        job_file_path[0] = '\0';

        strncpy(job_file_path, directory, sizeof(job_file_path) - 4);
        job_file_path[sizeof(job_file_path) - 1] = '\0'; // Ensure null-termination
        strncat(job_file_path, "/", sizeof(job_file_path) - strlen(job_file_path) - 1);
        strncat(job_file_path, entry->d_name, sizeof(job_file_path) - strlen(job_file_path) - 1);
        process_job_file(job_file_path);
    }
}


  // Close the directory after reading it
  closedir(dir);

  kvs_terminate();
  return 0;
}
