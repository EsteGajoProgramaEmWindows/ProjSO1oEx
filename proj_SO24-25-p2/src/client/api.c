#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "api.h"
#include "src/common/constants.h"
#include "src/common/protocol.h"


int kvs_connect(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path,
                char const* notif_pipe_path, int* notif_pipe) {


  // create pipes and connect

  pthread_t* notif_thread;

  int fd_register;
  int fd_request;
  int fd_response;
  int fd_notification;

  // creates and opens request pipe for writing
  if(mkfifo(req_pipe_path, 0640)!=0){
    perror("Error creating request pipe");
    return 1;
  }
  
  fd_request = open(req_pipe_path, O_WRONLY);
  if(fd_request == -1){
    perror("Error opening request pipe");
    return 1;
  }

  // creates and opens response pipe for reading
  if(mkfifo(resp_pipe_path, 0640)!=0){
    perror("Error creating response pipe");
    return 1;
  }

  fd_response = open(resp_pipe_path, O_RDONLY);

  if(fd_response == -1){
    perror("Error opening response pipe");
    return 1;
  }

  // creates and opens notification pipe for writing
  if(mkfifo(notif_pipe_path, 0640)!=0){
    perror("Error creating notification pipe");
    return 1;
  }

  fd_notification = open(notif_pipe_path, O_WRONLY);
  if(fd_notification == -1){
    perror("Error opening notification pipe");
    return 1;
  }

  // creates notifications thread
  pthread_create(&notif_thread, NULL, notifications_handler, (void*)&fd_notification);


  printf("Opening fifo\n");
  fd_register = open(server_pipe_path, O_WRONLY);
  if (fd_register == -1) {
    perror("Error opening the server pipe");
    return 1;
  }

  char buffer[MAX_STRING_SIZE];
  strcpy(buffer, "1 ");
  strcat(buffer, req_pipe_path);
  strcat(buffer, " ");
  strcat(buffer, resp_pipe_path);
  strcat(buffer, " ");
  strcat(buffer, notif_pipe_path);

  printf("Writing to server pipe\n");
  printf("%ld\n", write(fd_register, buffer, strlen(buffer)));

  close(fd_register);
  printf("Closing fifo\n"); 
  pthread_join(notif_thread, NULL);
  return 0;
}
 
int kvs_disconnect(void) {
  // close pipes and unlink pipe files
  return 0;
}

int kvs_subscribe(const char* key) {
  // send subscribe message to request pipe and wait for response in response pipe
  return 0;
}

int kvs_unsubscribe(const char* key) {
    // send unsubscribe message to request pipe and wait for response in response pipe
  return 0;
}

static void* notifications_handler(void *fd_notification){
  int* fd_notif = (int*) fd_notification;
  char buffer[MAX_STRING_SIZE];
  while (1){
    read(fd_notif, buffer, MAX_STRING_SIZE);
    write(STDOUT_FILENO, buffer, MAX_STRING_SIZE);
  }
  pthread_exit(NULL);
}


