#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

#include "keys_linked_list.h"
#include "queue.h"
#include "constants.h"
#include "parser.h"
#include "operations.h"
#include "subscriber_linked_list.h"
#include "kvs.h"
#include "src/common/constants.h"
#include "src/common/io.h"
#include "src/common/protocol.h"
#include "io.h"



struct SharedData {
  DIR* dir;
  char* dir_name;
  pthread_mutex_t directory_mutex;
};

typedef struct fifoData{
  int fd_response;
  int fd_notification;
}t_fifoData;

typedef struct HostData{
  char *register_fifo_path;
  t_queue *queue;
  t_fifoData *fifo_data;
}t_hostData;

typedef struct managerThreadArgs{
  t_queue *queue;
  t_fifoData *myfifos;
}t_managerThreadArgs;

t_fifoData* fifos;
sem_t empty; // semaphore for empty queue
sem_t full;  // semaphore for full queue
pthread_mutex_t lock_queue = PTHREAD_MUTEX_INITIALIZER; // mutex to protect the queue
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t n_current_backups_lock = PTHREAD_MUTEX_INITIALIZER;
int result_connect = 0;     // variable used to indicate sucessful connection
int result_disconnect = 0;  // variable used to indicate sucessful disconnect

size_t active_backups = 0;     // Number of active backups
size_t max_backups;            // Maximum allowed simultaneous backups
size_t max_threads;            // Maximum allowed simultaneous threads    
char* jobs_directory = NULL;
char* register_fifo_path = NULL;
int fd_register;



void sigusr1(int signum){
  for(int i=0; i < MAX_SESSION_COUNT; i++){
    if(fifos[i].fd_notification == -1){
      continue;
    }
    else{
      close(fifos[i].fd_notification);
      close(fifos[i].fd_response);
    }
  }
  // deletes every subscription associated with the hastable
  for(int i = 0; i < 26; i++){
    KeyNode *node = kvs_table->table[i];
    while(node != NULL){
      t_SubscriberListNode *prev = node->head;
      t_SubscriberListNode *subs = node->head;
      while(subs != NULL){ 
        subs = node->head->next;
        free(prev);
        prev = subs;
      }
      node->head = NULL;
      node = node->next;
    }
  }
}

int filter_job_files(const struct dirent* entry) {
    const char* dot = strrchr(entry->d_name, '.');
    if (dot != NULL && strcmp(dot, ".job") == 0) {
        return 1;  // Keep this file (it has the .job extension)
    }
    return 0;
}

static int entry_files(const char* dir, struct dirent* entry, char* in_path, char* out_path) {
  const char* dot = strrchr(entry->d_name, '.');
  if (dot == NULL || dot == entry->d_name || strlen(dot) != 4 || strcmp(dot, ".job")) {
    return 1;
  }

  if (strlen(entry->d_name) + strlen(dir) + 2 > MAX_JOB_FILE_NAME_SIZE) {
    fprintf(stderr, "%s/%s\n", dir, entry->d_name);
    return 1;
  }

  strcpy(in_path, dir);
  strcat(in_path, "/");
  strcat(in_path, entry->d_name);

  strcpy(out_path, in_path);
  strcpy(strrchr(out_path, '.'), ".out");

  return 0;
}

static int run_job(int in_fd, int out_fd, char* filename) {
  size_t file_backups = 0;
  while (1) {
    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    unsigned int delay;
    size_t num_pairs;

    switch (get_next(in_fd)) {
      case CMD_WRITE:
        num_pairs = parse_write(in_fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);
        if (num_pairs == 0) {
          write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_write(num_pairs, keys, values) != (int)(num_pairs)) {
          write_str(STDERR_FILENO, "Failed to write pair\n");
        }
        break;

      case CMD_READ:
        num_pairs = parse_read_delete(in_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0) {
          write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
          continue; 
        }

        if (kvs_read(num_pairs, keys, out_fd)) {
          write_str(STDERR_FILENO, "Failed to read pair\n");
        }
        break;

      case CMD_DELETE:
        num_pairs = parse_read_delete(in_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0) {
          write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_delete(num_pairs, keys, out_fd)) {
          write_str(STDERR_FILENO, "Failed to delete pair\n");
        }
        break;

      case CMD_SHOW:
        kvs_show(out_fd);
        break;

      case CMD_WAIT:
        if (parse_wait(in_fd, &delay, NULL) == -1) {
          write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting %d seconds\n", delay / 1000);
          kvs_wait(delay);
        }
        break;

      case CMD_BACKUP:
        pthread_mutex_lock(&n_current_backups_lock);
        if (active_backups >= max_backups) {
          wait(NULL);
        } else {
          active_backups++;
        }
        pthread_mutex_unlock(&n_current_backups_lock);
        int aux = kvs_backup(++file_backups, filename, jobs_directory);

        if (aux < 0) {
            write_str(STDERR_FILENO, "Failed to do backup\n");
        } else if (aux == 1) {
          return 1;
        }
        break;

      case CMD_INVALID:
        write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        write_str(STDOUT_FILENO,
            "Available commands:\n"
            "  WRITE [(key,value)(key2,value2),...]\n"
            "  READ [key,key2,...]\n"
            "  DELETE [key,key2,...]\n"
            "  SHOW\n"
            "  WAIT <delay_ms>\n"
            "  BACKUP\n" // Not implemented
            "  HELP\n");

        break;

      case CMD_EMPTY:
        break;

      case EOC:
        printf("EOF\n");
        return 0;
    }
  }
}

//frees arguments
static void* get_file(void* arguments) {

  // creates signal set
  sigset_t blockSet;
  // Initialize the signal set
    if (sigemptyset(&blockSet) == -1) {
        perror("sigemptyset");
        exit(EXIT_FAILURE);
    }
    // Add SIGUSR1 to the signal set
    if (sigaddset(&blockSet, SIGUSR1) == -1) {
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
  //mask sigurs1
  sigprocmask(SIG_BLOCK, &blockSet, NULL);

  struct SharedData* thread_data = (struct SharedData*) arguments;
  DIR* dir = thread_data->dir;
  char* dir_name = thread_data->dir_name;

  if (pthread_mutex_lock(&thread_data->directory_mutex) != 0) {
    fprintf(stderr, "Thread failed to lock directory_mutex\n");
    return NULL;
  }

  struct dirent* entry;
  char in_path[MAX_JOB_FILE_NAME_SIZE], out_path[MAX_JOB_FILE_NAME_SIZE];
  while ((entry = readdir(dir)) != NULL) {
    if (entry_files(dir_name, entry, in_path, out_path)) {
      continue;
    }

    if (pthread_mutex_unlock(&thread_data->directory_mutex) != 0) {
      fprintf(stderr, "Thread failed to unlock directory_mutex\n");
      return NULL;
    }

    int in_fd = open(in_path, O_RDONLY);
    if (in_fd == -1) {
      write_str(STDERR_FILENO, "Failed to open input file: ");
      write_str(STDERR_FILENO, in_path);
      write_str(STDERR_FILENO, "\n");
      pthread_exit(NULL);
    }

    int out_fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd == -1) {
      write_str(STDERR_FILENO, "Failed to open output file: ");
      write_str(STDERR_FILENO, out_path);
      write_str(STDERR_FILENO, "\n");
      pthread_exit(NULL);
    }

    int out = run_job(in_fd, out_fd, entry->d_name);

    close(in_fd);
    close(out_fd);

    if (out) {
      if (closedir(dir) == -1) {
        fprintf(stderr, "Failed to close directory\n");
        return 0;
      }

      exit(0);
    }

    if (pthread_mutex_lock(&thread_data->directory_mutex) != 0) {
      fprintf(stderr, "Thread failed to lock directory_mutex\n");
      return NULL;
    }
  }

  if (pthread_mutex_unlock(&thread_data->directory_mutex) != 0) {
    fprintf(stderr, "Thread failed to unlock directory_mutex\n");
    return NULL;
  }

  pthread_exit(NULL);
}


static void dispatch_threads(DIR* dir) {
  pthread_t* threads = malloc(max_threads * sizeof(pthread_t));

  if (threads == NULL) {
    fprintf(stderr, "Failed to allocate memory for threads\n");
    return;
  }

  struct SharedData thread_data = {dir, jobs_directory, PTHREAD_MUTEX_INITIALIZER};


  for (size_t i = 0; i < max_threads; i++) {
    if (pthread_create(&threads[i], NULL, get_file, (void*)&thread_data) != 0) {
      fprintf(stderr, "Failed to create thread %zu\n", i);
      pthread_mutex_destroy(&thread_data.directory_mutex);
      free(threads);
      return;
    }
  }

  // ler do FIFO de registo

  for (unsigned int i = 0; i < max_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Failed to join thread %u\n", i);
      pthread_mutex_destroy(&thread_data.directory_mutex);
      free(threads);
      return;
    }
  }

  if (pthread_mutex_destroy(&thread_data.directory_mutex) != 0) {
    fprintf(stderr, "Failed to destroy directory_mutex\n");
  }

  free(threads);
}


static void *manager_thread_handler(void *arguments){
  // pointer to an linked list that will be used to store the index of the hash table associated 
  // to the keys subscribed by which client
  t_managerThreadArgs *args = (t_managerThreadArgs*) arguments;
  t_KeyListNode *key_list;
  t_queue *queue = args->queue;
  t_fifoData *apt_fifo_data = args->myfifos;


  char buffer[3] = "";
  int intr;

  // creates signal set
  sigset_t blockSet;
  // Initialize the signal set
    if (sigemptyset(&blockSet) == -1) {
        perror("sigemptyset");
        exit(EXIT_FAILURE);
    }
    // Add SIGUSR1 to the signal set
    if (sigaddset(&blockSet, SIGUSR1) == -1) {
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
  //mask sigurs1
  sigprocmask(SIG_BLOCK, &blockSet, NULL);
  while(1){

    sem_wait(&full); // decrement the semaphore of full itens
    pthread_mutex_lock(&lock_queue); // lock the mutex
    t_node_queue *name_fifos = pop(queue);
    pthread_mutex_unlock(&lock_queue); // unlock the mutex
    sem_post(&empty); // increment the semaphore for empty itens

    if(name_fifos == NULL){
      continue;
    }
    // Opens the request fifo
    int fd_request;
    fd_request = open(name_fifos->request_fifo_name, O_RDONLY);

   if(fd_request == -1) {
      char msg[128] = "";
      sprintf(msg, "Failed to open request fifo: %s\n", name_fifos->request_fifo_name);
      write_str(STDERR_FILENO, msg);
      continue;
    }
    // Opens the response fifo
    int fd_response;
    fd_response = open(name_fifos->response_fifo_name, O_WRONLY);
    if(fd_request == -1) {
      char msg[128] = "";
      sprintf(msg, "Failed to open response fifo: %s\n", name_fifos->response_fifo_name);
      write_str(STDERR_FILENO, msg);
      continue;
    }

    int fd_notification;
    fd_notification = open(name_fifos->notification_fifo_name, O_WRONLY);
    if(fd_notification == -1){
      char msg[128] = "";
      sprintf(msg, "Failed to open notification fifo: %s\n", name_fifos->notification_fifo_name);
      write_str(STDERR_FILENO, msg);
      continue;
    }
    if(result_connect == 1){
      strcpy(buffer, "11");
      if(write_all(fd_response, buffer, strlen(buffer)) == -1) {
          write_str(STDERR_FILENO, "write failed");
          continue;
      }
    }
    strcpy(buffer, "10");
    if(write_all(fd_response, buffer, strlen(buffer)) == -1) {
      write_str(STDERR_FILENO, "write failed");
      continue;
    }

    apt_fifo_data->fd_notification = fd_notification;
    apt_fifo_data->fd_response = fd_response;

    while(1){
      // process the commands sent by the client in the request fifo
      int opcode;
      char read_buffer[42];
      char key[41];

      if(read_all(fd_request, read_buffer, sizeof(read_buffer), &intr)==-1){
        write_str(STDERR_FILENO, "read failed");
      }

      strncpy(key, read_buffer +1, 41);
      if(opcode != 2 || opcode != 3 || opcode != 4){
        write_str(STDERR_FILENO, "OPCODE: INVALID");
      }
      opcode = read_buffer[0];
      switch (opcode){
        case OP_CODE_DISCONNECT:
          apt_fifo_data->fd_notification = -1;
          apt_fifo_data->fd_notification = -1;
          //deletes every subscription key in the server
          while(key_list!=NULL) {
            kvs_unsubscribe(key_list->key, fd_notification);
            key_list = key_list->next;
          }
          
          if(result_disconnect == 1) {
            strcpy(buffer, "21");
            if(write_all(fd_response, buffer, strlen(buffer)) == -1) {
              write_str(STDERR_FILENO, "write failed");
            }
          }

          else{
            strcpy(buffer, "20");
            if(write_all(fd_response, buffer, strlen(buffer)) == -1) {
              write_str(STDERR_FILENO, "write failed");
            }
          }
            close(fd_request);
            close(fd_response);
            close(fd_notification);
            break;
        case OP_CODE_SUBSCRIBE:
          // insertes in the keys linked list of the client the key that was subscribed
          append_list_node_key(key_list, key);
          // inserts in the linked list of subscriptions the fifo path of the client in the determinied key
          if(kvs_subscribe(key, fd_notification) == 0){
            strcpy(buffer, "30");
            if(write_all(fd_response, buffer, strlen(buffer)) == -1) {
              write_str(STDERR_FILENO, "write failed");
            }
          }
          strcpy(buffer, "31");
          if(write_all(fd_response, buffer, strlen(buffer)) == -1) {
            write_str(STDERR_FILENO, "write failed");
          }
          break;

        case OP_CODE_UNSUBSCRIBE:
          // removes from the keys linked list of the client the key that was unsubscribed
          delete_list_node_key(key_list, key);
          // removes from the linked list of subscriptions the fifo path of the client in the determinied key
            if(kvs_unsubscribe(key, fd_notification) == 0){
              strcpy(buffer, "40");
              if(write_all(fd_response, buffer, strlen(buffer)) == -1) {
                write_str(STDERR_FILENO, "write failed");
              }
            }
            else{
              strcpy(buffer, "41");
              if(write_all(fd_response, buffer, strlen(buffer)) == -1) {
                write_str(STDERR_FILENO, "write failed");
              }
            }
          break;
        }
      }

    }

  pthread_exit(NULL);

}

static void* manages_register_fifo(void *arguments){

  
  t_hostData *host_data = (t_hostData*)arguments;
  
  char *fifo_path_register = host_data->register_fifo_path;
  t_queue *queue = host_data->queue;
  char buffer[122];
  int opcode;
  int intr = 0;
  char request_fifo_name[41] = {0};
  char response_fifo_name[41] = {0};
  char notification_fifo_name[41] = {0};

  // Opens register fifo for reading
  fd_register = open(fifo_path_register, O_RDONLY);

  if(fd_register == -1) {
    result_connect = 1;
    write_str(STDERR_FILENO, "open failed");
    result_connect = 1;
  }

  while(1){
    if(read_all(fd_register, buffer, 123, &intr) == -1){
      write_str(STDERR_FILENO, "read failed");
      result_connect = 1;
      continue;
    }
    // extracts each part of the mensage sent from the register fifo
    opcode = buffer[0] - 48;
    if(opcode == OP_CODE_CONNECT) {

      memcpy(request_fifo_name, buffer +1 , 40);
      for (int i = 0; i < 40; i++){
        if(request_fifo_name[i] == 32){
          request_fifo_name[i] = '\0';
        }
      }
      memcpy(response_fifo_name, buffer + 41, 40);
      for(int i = 0; i < 40; i++){
        if(response_fifo_name[i] == 32){
          response_fifo_name[i] = '\0';
        }
      }

      memcpy(notification_fifo_name, buffer + 81, 40);
      for(int i = 0; i < 40; i++){
        if(notification_fifo_name[i] == 32){
          notification_fifo_name[i] = '\0';
        }
      }

      sem_wait(&empty); // Decrement the semaphore of empty items
      pthread_mutex_lock(&lock_queue); // locks the mutex 
      enqueue(queue,request_fifo_name, response_fifo_name, notification_fifo_name);
      pthread_mutex_unlock(&lock_queue); // unlocks the mutex
      sem_post(&full); // Increment the semaphore of full items
    }
    else{
      write_str(STDERR_FILENO, "Opcode Invalid\n");
    }
  }
  
  pthread_exit(NULL);
}


int main(int argc, char** argv) {
  if (argc < 5) {
    write_str(STDERR_FILENO, "Usage: ");
    write_str(STDERR_FILENO, argv[0]);
    write_str(STDERR_FILENO, " <jobs_dir>");
		write_str(STDERR_FILENO, " <max_threads>");
		write_str(STDERR_FILENO, " <max_backups>");
    write_str(STDERR_FILENO, " <resgister_fifo_path> \n");
    return 1;
  }

  // Create the variable that holds the id of the host thread
  pthread_t* host_thread = malloc(sizeof(pthread_t));
  pthread_t* manager_thread = (pthread_t*) malloc(sizeof(pthread_t) *(MAX_SESSION_COUNT));
  t_queue* manager_queue;
  manager_queue = create_queue();
  jobs_directory = argv[1];
  fifos = (t_fifoData*)malloc(sizeof(t_fifoData) * MAX_SESSION_COUNT);

  for(int i = 0; i < MAX_SESSION_COUNT; i++){
    fifos[i].fd_response = -1;
    fifos[i].fd_notification = -1;
  }

  sem_init(&empty, 0, MAX_SESSION_COUNT); // initialize the semaphore of empty items with the max of clients in simultaneous in the same session
  sem_init(&full, 0, 0); // initialize the semaphore of full items with 0
  

  char* endptr;
  max_backups = strtoul(argv[3], &endptr, 10);
  register_fifo_path = argv[4];

  if (*endptr != '\0') {
    fprintf(stderr, "Invalid max_proc value\n");
    return 1;
  }

  max_threads = strtoul(argv[2], &endptr, 10);

  if (*endptr != '\0') {
    fprintf(stderr, "Invalid max_threads value\n");
    return 1;
  }

	if (max_backups <= 0) {
		write_str(STDERR_FILENO, "Invalid number of backups\n");
		return 0;
	}

	if (max_threads <= 0) {
		write_str(STDERR_FILENO, "Invalid number of threads\n");
		return 0;
	}

  if (kvs_init()) {
    write_str(STDERR_FILENO, "Failed to initialize KVS\n");
    return 1;
  }

  DIR* dir = opendir(argv[1]);
  if (dir == NULL) {
    fprintf(stderr, "Failed to open directory: %s\n", argv[1]);
    return 0;
  }

  // Remove existing FIFO if any
  if (unlink(register_fifo_path) != 0) { 
    write_str(STDERR_FILENO, "unlink failed\n");
  } 

  // Creates register fifo
  if(mkfifo(register_fifo_path, 0640)!=0) {
    write_str(STDERR_FILENO, "mkfifo failed\n");
    return 1;
  }

  t_hostData *host_data = (t_hostData *)malloc(sizeof(t_hostData));
  host_data->queue = manager_queue;
  host_data->register_fifo_path = register_fifo_path;
  host_data->fifo_data = fifos;

  //Create host thread
  if(pthread_create(host_thread, NULL, manages_register_fifo, (void *)host_data)!=0 ){
    write_str(STDERR_FILENO, "pthread_create failed\n");
    result_connect = 1;
  }

  dispatch_threads(dir);

  if (closedir(dir) == -1) {
    fprintf(stderr, "Failed to close directory\n");
    return 0;
  }

  while (active_backups > 0) {
    wait(NULL);
    active_backups--;
  }

  t_managerThreadArgs args[MAX_SESSION_COUNT];
  for (int i = 0; i < MAX_SESSION_COUNT; i++){
    args[i].queue = manager_queue;
    args[i].myfifos = fifos + i;
  }

  // Create manager threads
  for (int i = 0; i < MAX_SESSION_COUNT; i++){
    if(pthread_create(&manager_thread[i], NULL, manager_thread_handler,(void*)&args[i]) != 0){
      fprintf(stderr, "Failed to create thread.\n");
      exit(1);
    }
  }

  for(int i = 0; i < (int)max_threads; i++){
    pthread_join(manager_thread[i], NULL);
  }


  // creates signal set
  sigset_t blockSet;
  // Initialize the signal set
    if (sigemptyset(&blockSet) == -1) {
        perror("sigemptyset");
        exit(EXIT_FAILURE);
    }
    // Add SIGUSR1 to the signal set
    if (sigaddset(&blockSet, SIGUSR1) == -1) {
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }

  //mask sigurs1
  sigprocmask(SIG_BLOCK, &blockSet, NULL);

  pthread_join(*host_thread, NULL);
  sem_destroy(&empty);
  sem_destroy(&full);
  pthread_mutex_destroy(&lock_queue);
  free(host_thread);
  kvs_terminate();

  return 0;
}
