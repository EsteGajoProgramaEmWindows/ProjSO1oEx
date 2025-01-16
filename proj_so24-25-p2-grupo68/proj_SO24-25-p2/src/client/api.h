#ifndef CLIENT_API_H
#define CLIENT_API_H

#include <stddef.h>
#include "src/common/constants.h"

extern int fd_response;
extern int fd_request;
extern int fd_notification;

int kvs_connect(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path,
                char const* notif_pipe_path, int* notif_pipe);
int kvs_disconnect(void);
int kvs_subscribe(const char* key);
int kvs_unsubscribe(const char* key);

#endif  // CLIENT_API_H