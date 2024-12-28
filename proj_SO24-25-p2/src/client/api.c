#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
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

  int fd_register;
  printf("Opening fifo\n");
  fd_register = open(server_pipe_path, O_WRONLY);
  if (fd_register == -1) {
    perror("Error opening the server pipe");
    return 1;
  }

  char *buffer = (char*)malloc(sizeof(char) * 41);
  strcpy(buffer, "Client 1");
  printf("Writing to server pipe\n");
  printf("%ld\n", write(fd_register, buffer, strlen(buffer)));

  close(fd_register);
  printf("Closing fifo\n");
  free(buffer);
  
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


