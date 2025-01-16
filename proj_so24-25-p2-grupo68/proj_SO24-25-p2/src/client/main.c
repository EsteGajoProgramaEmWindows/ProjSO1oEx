#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "api.h"
#include "parser.h"
#include "src/common/constants.h"
#include "src/common/io.h"


pthread_t main_thread;
char req_pipe_path[256] = "/tmp/req";
char resp_pipe_path[256] = "/tmp/resp";
char notif_pipe_path[256] = "/tmp/notif";
pthread_t* notif_thread;

static void sigint_handler(int signum){
  if(kvs_disconnect() != 0) {
    fprintf(stderr, "Failed to disconnect to the server\n");
  }
  // TODO: end notifications thread
  pthread_join(*notif_thread, NULL);
  free(notif_thread);
        
  if(unlink(req_pipe_path) != 0) {
    perror("Error removing request pipe");
  }
  if(unlink(resp_pipe_path)!= 0) {
    perror("Error removing response pipe");
  }
  if(unlink(notif_pipe_path)!= 0) {
    perror("Error removing notification pipe");
  }

}


static void sig_pipe_handler(int signum){
  printf("Received SIGPIPE : Fifos closed by server\n");
  close(fd_notification);
  close(fd_request);
  close(fd_response);
  exit(1);
}


static void* notifications_handler(void *fd_notif){
  int fd_notify = *(int*) fd_notif;
  int intr = 0;
  char buffer[MAX_STRING_SIZE];
  while (1){
    int result = read_all(fd_notify, buffer, MAX_STRING_SIZE, &intr);
    if(result == -1){
    perror("read failed");
    }
    if(result == 0){
      pthread_kill(main_thread, SIGPIPE);
      break;
    }

    if(write_all(STDOUT_FILENO, buffer, MAX_STRING_SIZE)==-1)
      perror("write failed");
  }
  pthread_exit(NULL);
}


int main(int argc, char* argv[]) {
  struct sigaction sa;
  sa.sa_handler = sigint_handler; // arm the sig_int 
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGINT, &sa, NULL) == -1){
    perror("sigaction failed");
    return 1;
  }
  sa.sa_handler = sig_pipe_handler; // arm the sig_pipe
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if(sigaction(SIGPIPE, &sa, NULL) == -1){
    perror("sigaction failed");
    return 1;
  }

  
  notif_thread = (pthread_t*)malloc(sizeof(pthread_t));

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <client_unique_id> <register_pipe_path>\n", argv[0]);
    return 1;
  }


  char keys[MAX_NUMBER_SUB][MAX_STRING_SIZE] = {0};
  unsigned int delay_ms;
  size_t num;

  strncat(req_pipe_path, argv[1], strlen(argv[1]) * sizeof(char));
  strncat(resp_pipe_path, argv[1], strlen(argv[1]) * sizeof(char));
  strncat(notif_pipe_path, argv[1], strlen(argv[1]) * sizeof(char));

  // TODO open pipes 
  int notif_pipe;
  if(kvs_connect(req_pipe_path, resp_pipe_path, argv[2], notif_pipe_path, &notif_pipe) != 0){
    fprintf(stderr, "Failed to connect to the server\n");
    return 1;
  }

  // creates notifications thread
  pthread_create(notif_thread, NULL, notifications_handler, (void*)&notif_pipe);

  while (1) {
    switch (get_next(STDIN_FILENO)) {
      case CMD_DISCONNECT:
        if (kvs_disconnect() != 0) {
          fprintf(stderr, "Failed to disconnect to the server\n");
          return 1;
        }
        // TODO: end notifications thread
        pthread_join(*notif_thread, NULL);
        free(notif_thread);
        
        if(unlink(req_pipe_path) != 0) {
          perror("Error removing request pipe");
        }
        if(unlink(resp_pipe_path)!= 0) {
          perror("Error removing response pipe");
        }
        if(unlink(notif_pipe_path)!= 0) {
          perror("Error removing notification pipe");
        }
        printf("Disconnected from server\n");
        return 0;

      case CMD_SUBSCRIBE:
        num = parse_list(STDIN_FILENO, keys, 1, MAX_STRING_SIZE);
        if (num == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }
         
        if (kvs_subscribe(keys[0])) {
            fprintf(stderr, "Command subscribe failed\n");
        }

        break;

      case CMD_UNSUBSCRIBE:
        num = parse_list(STDIN_FILENO, keys, 1, MAX_STRING_SIZE);
        if (num == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }
         
        if (kvs_unsubscribe(keys[0])) {
            fprintf(stderr, "Command subscribe failed\n");
        }

        break;

      case CMD_DELAY:
        if (parse_delay(STDIN_FILENO, &delay_ms) == -1) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay_ms > 0) {
            printf("Waiting...\n");
            delay(delay_ms);
        }
        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_EMPTY:
        break;

      case EOC:
        // input should end in a disconnect, or it will loop here forever
        break;
    }
  }
  free(notif_thread);
}
