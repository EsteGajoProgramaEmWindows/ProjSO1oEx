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
#include "src/common/io.h"

int fd_response;
int fd_request;
int fd_notification;

int kvs_connect(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path,
                char const* notif_pipe_path, int* notif_pipe) {

    // create pipes and connect

    int fd_register;
    char buffer[122] = {0};
    int intr = 0;
    char buffer_response[3];
    char message[MAX_STRING_SIZE];

    // creates and opens request pipe for writing
    if (mkfifo(req_pipe_path, 0640) != 0) {
        perror("Error creating request pipe");
        return 1;
    }

    // creates and opens response pipe for reading
    if (mkfifo(resp_pipe_path, 0640) != 0) {
        perror("Error creating response pipe");
        return 1;
    }

    // creates and opens notification pipe for writing
    if (mkfifo(notif_pipe_path, 0640) != 0) {
        perror("Error creating notification pipe");
        return 1;
    }

    fd_register = open(server_pipe_path, O_WRONLY);
    printf("EU ABRI O PIPE\n");
    if (fd_register == -1) {
        perror("Error opening the server pipe");
        return 1;
    }

    sprintf(buffer, "1%-40s%-40s%-40s", req_pipe_path, resp_pipe_path, notif_pipe_path);

    if (write_all(fd_register, &buffer, sizeof(t_data_fifo)) == -1) {
        perror("write failed");
    }

    close(fd_register);

    fd_request = open(req_pipe_path, O_WRONLY);
    if (fd_request == -1) {
        perror("Error opening request pipe");
        return 1;
    }

    fd_response = open(resp_pipe_path, O_RDONLY);
    if (fd_response == -1) {
        perror("Error opening response pipe");
        return 1;
    }

    *notif_pipe = open(notif_pipe_path, O_WRONLY);
    if (*notif_pipe == -1) {
        perror("Error opening notification pipe");
        return 1;
    }

    if (read_all(fd_response, buffer_response, strlen(buffer_response), &intr) == -1) {
        perror("read_failed");
    };

    char result[2] = {buffer_response[1], '\0'};
    strcpy(message, "Server returned");
    strcat(message, result);
    strcat(message, "for operation:connect");
    if (write_all(STDOUT_FILENO, message, strlen(message)) == -1) {
        perror("write failed");
    };

    return 0;
}

int kvs_disconnect(void) {
    // close pipes and unlink pipe files
    char buffer_response[3];
    char message[MAX_STRING_SIZE];
    int intr = 0;
    char buffer[MAX_STRING_SIZE] = "2";
    if (write_all(fd_request, buffer, strlen(buffer)) == -1) {
        perror("write failed");
    };
    if (read_all(fd_response, buffer_response, strlen(buffer_response), &intr) == -1) {
        perror("read_failed");
    };

    char result[2] = {buffer_response[1], '\0'};

    strcpy(message, "Server returned");
    strcat(message, result);
    strcat(message, "for operation:disconnect");

    if (write_all(STDOUT_FILENO, message, strlen(message)) == -1) {
        perror("write failed");
    }

    close(fd_response);
    close(fd_request);
    close(fd_notification);
    return 0;
}

int kvs_subscribe(const char* key) {
    // send subscribe message to request pipe and wait for response in response pipe
    char buffer_response[3];
    int intr = 0;
    char message[MAX_STRING_SIZE];
    char buffer_request[42] = "3";

    strcat(buffer_request, key);
    if (write_all(fd_request, buffer_request, strlen(key)) == -1) {
        perror("write failed");
    }

    if (read_all(fd_response, buffer_response, strlen(buffer_response), &intr) == -1) {
        perror("read_failed");
    }

    char result[2] = {buffer_response[1], '\0'};

    strcpy(message, "Server returned");
    strcat(message, result);
    strcat(message, "for operation:subscribe");
    if (write_all(STDOUT_FILENO, message, strlen(message)) == -1) {
        perror("write failed");
    }
    return 0;
}

int kvs_unsubscribe(const char* key) {
    // send unsubscribe message to request pipe and wait for response in response pipe
    char buffer_response[3];
    int intr = 0;
    char message[MAX_STRING_SIZE];
    char buffer_request[42] = "4";

    strcat(buffer_request, key);
    if (write_all(fd_request, buffer_request, strlen(key)) == -1) {
        perror("write failed");
    }

    if (read_all(fd_response, buffer_response, strlen(buffer_response), &intr) == -1) {
        perror("read_failed");
    }

    char result[2] = {buffer_response[1], '\0'};

    strcpy(message, "Server returned");
    strcat(message, result);
    strcat(message, "for operation:unsubscribe");

    if (write_all(STDOUT_FILENO, message, strlen(message)) == -1) {
        perror("write failed");
    }
    return 0;
}