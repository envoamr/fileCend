#include "utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>     // memset()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // inet_pton()

int main()
{
  // get ip address from user
  char host[16]; // IPv4's max 15 chars + null terminator
  printf("Host: ");
  fgets(host, sizeof(host), stdin);
  sscanf(host, "%s", host); // removes \n
  if (strcmp(host, "localhost") == 0 || strcmp(host, "\n") == 0)
    memcpy(host, "127.0.0.1", sizeof(host));

  // create server struct and validate host
  struct sockaddr_in server_addr;
  int host_status;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  host_status = inet_pton(AF_INET, host, &server_addr.sin_addr);
  if (host_status == 0)
  {
    printf("error: Bad address\n");
    exit(14);
  }

  // get port from user
  int port;
  port = filecend_get_port();
  server_addr.sin_port = htons(port);

  int sockfd;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
    filecend_error_exit("error: socket", NULL);

  printf("Trying to connect to %s:%d...\n", host, port);
  int connect_status;
  connect_status = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (connect_status == -1)
    filecend_error_exit("error: connect", NULL);
  printf("Connection established.\n");

  // create threads for listening and transmiting data via socket
  // threads are less efficient to async I/O but only two are needed
  pthread_t read_data_thread, write_data_thread;
  pthread_create(&read_data_thread, NULL, (void *)filecend_write_data, (void *)&sockfd);
  pthread_create(&write_data_thread, NULL, (void *)filecend_read_data, (void *)&sockfd);
  pthread_exit(0); // wait for all threads to terminate before ending program

  return 0;
}
