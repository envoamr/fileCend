#ifndef UTILS_H
#define UTILS_H

void filecend_error_exit(char *msg, int *err_code);
int filecend_get_port();
int filecend_write_data(int *sockfd_new);
int filecend_read_data(int *socketfd_new);

#endif
