#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>       // strcat(), stelen()
#include <libgen.h>       // basename()
#include <linux/limits.h> // PATH_MAX
#include <unistd.h>       // access()
#include <errno.h>        // int errno
#include <sys/stat.h>     // struct stat, stat()

void filecend_error_exit(char *msg, int *err_code)
{
  if (err_code != NULL)
    errno = *err_code;
  perror(msg);
  exit(errno);
}

int filecend_get_port()
{
  int port;
  char port_input[7]; // max length is 65535 + null terminator + one digit for out of bound check
  printf("Port number: ");
  fgets(port_input, sizeof(port_input), stdin);

  // convert input str to int
  char *endptr;
  port = strtol(port_input, &endptr, 10); // instead of atoi(), for error handling
  if (port_input == endptr)
  {
    printf("error: Invalid port number\n");
    exit(14);
  }
  if (port < 1025 || port > 65535)
  {
    printf("error: Invalid port number. Valid range is 1025 to 65535\n");
    exit(14);
  }

  return port;
}

int filecend_write_data(int *sockfd_new)
{
  while (1)
  {
    // send data via socket
    int sockfd;
    sockfd = *(int *)sockfd_new;

    // get file path from user
    char file_to_read_path[PATH_MAX]; // PATH_MAX includes null terminator
    printf("Filepath: ");
    fgets(file_to_read_path, sizeof(file_to_read_path), stdin);
    sscanf(file_to_read_path, "%s", file_to_read_path); // removes \n

    // check file mode (file/dir), get length
    struct stat file_to_read_stat;
    int stat_status;
    stat_status = stat(file_to_read_path, &file_to_read_stat);
    if (stat_status == -1)
    {
      printf("error: Could not read file. Does file exist?\n");
      continue;
    }
    if (S_ISDIR(file_to_read_stat.st_mode))
    {
      printf("error: Is a directory\n");
      continue;
    }

    // Understand reason behind 'file_to_read_stat.st_mode & S_IRUSR' from below
    // eg st_mode gives 40644 in octal. only keep 644 (permission) when we
    // bitwise AND with 777 like:
    //   40644 & 00777
    //   100 000 110 100 100
    // ^ 000 000 111 111 111
    // = 000 000 110 100 100 = 644
    // Bitwise 644 with 400 ('read' permission bit)
    //   644 & 400
    //   110 100 100
    // ^ 100 000 000
    // = 100 000 000 = 400
    // anything not 0 is true. if permission is 344:
    //   344 & 400
    //   011 100 100
    // ^ 100 000 000
    // = 000 000 000 = 0
    // passes 0 to if statement so false
    // see https://stackoverflow.com/a/15059931

    if (!(file_to_read_stat.st_mode & S_IRUSR))
    {
      printf("error: No read access\n");
      continue;
    }
    if (file_to_read_stat.st_size > 5242880) // 5mb
    {
      printf("error: File larger than 5MB\n");
      continue;
    }

    // set name of new file as prefix + old file
    char file_to_write_path[sizeof(file_to_read_path) + 1] = "_";
    strcat(file_to_write_path, basename(file_to_read_path));
    int file_to_write_path_length;
    file_to_write_path_length = strlen(file_to_write_path) + 1; // + 1 for null terminator

    // get pointer of file to find length of content and copy to buffer
    FILE *file_to_read_ptr = fopen(file_to_read_path, "rb");
    if (file_to_read_ptr == NULL)
      filecend_error_exit("error: fopen", NULL);

    // move pointer to end of file, get index (length), move pointer to start
    fseek(file_to_read_ptr, 0, SEEK_END);
    int file_to_read_buffer_length = ftell(file_to_read_ptr) + 1; // + 1 for null terminator
    fseek(file_to_read_ptr, 0, SEEK_SET);

    // copy file stream to buffer
    char file_to_read_buffer[file_to_read_buffer_length];
    fread(file_to_read_buffer, 1, sizeof(file_to_read_buffer), file_to_read_ptr);
    fclose(file_to_read_ptr);

    // filename length, filename, buffer size, buffer
    int write_status;

    write_status = write(sockfd, &file_to_write_path_length, sizeof(file_to_write_path_length));
    if (write_status < 0)
      filecend_error_exit("error: Failed to write the file path length to socket", NULL);

    write_status = write(sockfd, file_to_write_path, file_to_write_path_length);
    if (write_status < 0)
      filecend_error_exit("error: Failed to write the file path to socket", NULL);

    write_status = write(sockfd, &file_to_read_buffer_length, sizeof(file_to_read_buffer_length));
    if (write_status < 0)
      filecend_error_exit("error: Failed to write the buffer length to socket", NULL);

    write_status = write(sockfd, file_to_read_buffer, sizeof(file_to_read_buffer));
    if (write_status < 0)
      filecend_error_exit("error: Failed to write the buffer to socket", NULL);

    printf("%s successfully sent.\n", basename(file_to_read_path));
  }
  return 0;
}

int filecend_read_data(int *socketfd_new)
{
  while (1)
  {
    // cast socketfd
    int sockfd;
    sockfd = *(int *)socketfd_new;

    // read filename and buffer from socket
    int read_len;
    int read_err_code;
    read_err_code = 61;

    int filename_len;
    read_len = read(sockfd, &filename_len, sizeof(filename_len));
    if (read_len < sizeof(filename_len))
      filecend_error_exit("\nerror: Peer likely disconnected", &read_err_code);

    char filename[filename_len];
    read_len = read(sockfd, filename, sizeof(filename));
    if (read_len < sizeof(filename))
      filecend_error_exit("\nerror: Peer likely disconnected", &read_err_code);

    int buffer_len;
    read_len = read(sockfd, &buffer_len, sizeof(buffer_len));
    if (read_len < sizeof(buffer_len))
      filecend_error_exit("\nerror: Peer likely disconnected", &read_err_code);

    // write buffer to file
    FILE *new_file = fopen(filename, "ab");
    char buffer[4096];
    int buffer_read = 1;
    int to_write;

    while (buffer_len > 0)
    {
      buffer_read = read(sockfd, buffer, sizeof(buffer));
      buffer_len -= buffer_read;
      if (buffer_len < 1) // end of file, remove null terminator instead of writing gibberish
        buffer_read -= 1;
      to_write = fwrite(buffer, 1, buffer_read, new_file);
    }

    fclose(new_file);

    printf("\n%s received.\nFilepath: ", filename); // no basename() as it gets basename from socket
    fflush(stdout);                                 // either this or add \n for last printf
  }
  return 0;
}
