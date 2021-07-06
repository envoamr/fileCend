#include "utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>     // memset()
#include <netinet/in.h> // struct sockaddr_in

int main()
{
  int port;
  port = filecend_get_port();

  // initialize internet address structure of server and client
  struct sockaddr_in server_addr, client_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  // create socket, bind this system's address to socket, prepare for connections
  int sockfd, bind_status, listen_status;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  bind_status = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (bind_status == -1)
    filecend_error_exit("error: bind", NULL);

  listen_status = listen(sockfd, 1);
  if (listen_status == -1)
    filecend_error_exit("error: listen", NULL);

  // wait for incoming connection
  printf("Waiting for a connection on port %d...\n", port);
  int sockfd_new;
  socklen_t client_addr_len = sizeof(client_addr);
  sockfd_new = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
  if (sockfd_new == -1)
    filecend_error_exit("error: accept", NULL);
  printf("Connection established.\n");

  // create threads for listening and transmiting data via socket
  // threads are less efficient to async I/O but only two are needed
  pthread_t read_data_thread, write_data_thread;
  pthread_create(&read_data_thread, NULL, (void *)filecend_write_data, (void *)&sockfd_new);
  pthread_create(&write_data_thread, NULL, (void *)filecend_read_data, (void *)&sockfd_new);
  pthread_exit(0); // wait for all threads to terminate before ending program

  return 0;
}
