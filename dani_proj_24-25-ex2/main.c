#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <pthread.h>
#include "constants.h"
#include "parser.h"
#include "operations.h"
#include "queue.h"


  typedef struct process_job_arguments{
    int max_backups; 
    queue_t* job_queue;
  }p_job_args_t;

  pthread_mutex_t lock_queue;
  pthread_mutex_t lock_table;
 
  void *process_job_file(void* args) {
    printf("Ola - thread inicializada\n"); // Debugging para ver se a thread está sendo iniciada
    sleep(10);
    printf("Ola após sleep\n");  // Ver se a thread acorda do sleep
    p_job_args_t* args_aux = (p_job_args_t*)args;
    char job_file_path[MAX_JOB_FILE_NAME_SIZE];

    while(!isEmpty(args_aux->job_queue)){

      printf("ID da Thread %ld", pthread_self()); // debugging

      pthread_mutex_lock(&lock_queue); // locks the mutex
      pop(job_file_path, args_aux->job_queue); 
      pthread_mutex_unlock(&lock_queue); // unlocks the mutex

      int input_fd = open(job_file_path, O_RDONLY);

      if (input_fd < 0) {
        perror("Failed to open job file");
      }
      char output_file_path[MAX_JOB_FILE_NAME_SIZE]; 
      strncpy(output_file_path, job_file_path, strlen(job_file_path)-4);
      strcat(output_file_path, ".out");
      int output_fd = open(output_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd < 0) {
        perror("Failed to create output file");
        close(input_fd);
      
      }
      char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
      char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
      unsigned int delay;
      size_t num_pairs;
      int num_backups = 1;
      int backup_atual = 1;
      int continua = 1;

      while (continua) {
        switch (get_next(input_fd)) {
        case CMD_WRITE:
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
          // locks the mutex
          pthread_mutex_lock(&lock_table);
        // unlocks the mutex
        pthread_mutex_unlock(&lock_table);
          break;


        case CMD_WAIT:
          if (parse_wait(input_fd, &delay, NULL) == -1) {
          fprintf(stderr, "Invalid command Wt. See HELP for usage\n");
          break;
        }

        if (delay > 0) {
          write_to_file(output_fd,"Waiting...\n");
          kvs_wait(delay);
        }
        break;

      case CMD_BACKUP:

        while(num_backups > args_aux->max_backups){
          int status;
          pid_t result = waitpid(-1, &status, WNOHANG);
          if (result > 0){
              num_backups--;
          }
        }

        pid_t pid = fork();
        if(pid == 0){
        if (kvs_backup(job_file_path, backup_atual) == 1) {
           fprintf(stderr, "Failed to perform backup.\n");
          }
          exit(0);
        }
        num_backups++;
        backup_atual++;
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
        break;

      case EOC:
        close(input_fd);
        close(output_fd);
        continua = 0;
      }
    }
  }
  pthread_exit(NULL);
}
  


 
int parse_arguments(int argc, char *argv[], char *directory, int *max_backups, int *max_threads) {
  if (argc != 4) {
    return -1;
  }

  strcpy(directory, argv[1]);
  *max_backups = atoi(argv[2]);
  *max_threads = atoi(argv[3]);

  return 0;
}

int main(int argc, char *argv[]) {
  char directory[MAX_JOB_FILE_NAME_SIZE];
  int max_backups;
  int max_threads;
  //int num_threads;
  p_job_args_t *args;
  pthread_t *tid;

  if (kvs_init()) {
    return 1;
  }

  if (parse_arguments(argc, argv, directory, &max_backups, &max_threads) != 0) {
        return 1;
  }
  tid = (pthread_t*)malloc(sizeof(pthread_t) * (unsigned long)max_threads);
  if (tid == NULL) {
    perror("Failed to allocate memory for threads");
    return 1;
  }
 
  args = (p_job_args_t*)malloc(sizeof(p_job_args_t));

  queue_t* jobs_queue;
  jobs_queue = create_queue();

  DIR *dir = opendir(directory);
    if (!dir) {
        perror("Failed to open directory"); 
        return 1;
    }
  
  struct dirent *entry;
  char job_file_path[MAX_JOB_FILE_NAME_SIZE]; 
  // Iterate over each entry in the directory using readdir()
  while ((entry = readdir(dir)) != NULL) {
        // Check if the entry is a regular file and ends with ".job"
    if (strstr(entry->d_name, ".job") != NULL) {
        job_file_path[0] = '\0';
        strncpy(job_file_path, directory, sizeof(job_file_path) - 4);
        job_file_path[sizeof(job_file_path) - 1] = '\0'; // Ensure null-termination
        strncat(job_file_path, "/", sizeof(job_file_path) - strlen(job_file_path) - 1);
        strncat(job_file_path, entry->d_name, sizeof(job_file_path) - strlen(job_file_path) - 1);
        
        //enqueues the job_file_path to the jobs_queue
        enqueue(jobs_queue, job_file_path); 
    }
  } 
  args->max_backups = max_backups;
  args->job_queue = jobs_queue;

  //initialize the mutex 
  pthread_mutex_init(&lock_queue, NULL); 
  //initialize the mutex 
  pthread_mutex_init(&lock_table, NULL); 
  

  for (int i = 0; i < max_threads; i++){
    if(pthread_create(&tid[i], NULL, process_job_file,(void*)args) == 0){
      printf("Criada a thread com ID %lu\n", (unsigned long)tid[i]); //debugging 
      }
    else{
      fprintf(stderr, "Failed to create thread.\n");
      exit(1);
      }
    }
  
  for(int i = 0; i < max_threads; i++){
    pthread_join(tid[i], NULL);
  }

  free(tid);
  free(args);
  destroy_queue(jobs_queue);

  // destroy the mutex 
  pthread_mutex_destroy(&lock_queue);
   // destroy the mutex 
  pthread_mutex_destroy(&lock_table);
  // process_job_file(job_file_path, max_backups);

  // Close the directory after reading it
  closedir(dir);



  kvs_terminate();
  return 0;
}
