# fileCend

Transfers files between two computers. One computer must run the server file and the other the client file.

To transfer files across two computers in the same network (e.g. home) or across two folders in the same computer, run each file in a different computer/folder. To transfer files across the internet (around the world), the router must be configured appropriately, but comes with a security risk.

## Compilation

`gcc` will be used to compile the program. Make sure you are in the repo's working directory.

Create object files for `server.c`, `client.c`, and `utils.c`:

```bash
gcc -c src/server.c -o obj/server.o
gcc -c src/client.c -o obj/client.o
gcc -c src/utils/utils.c -o obj/utils.o
```

Create static library using archive:

```bash
ar -rcsv libfcendutils.a obj/utils.o
```

- `r`: replace old files in library with the new ones
- `c`: create the library
- `s`: add index for faster lookup times
- `v`: verbose output

Link library with program and create executable:

```bash
gcc obj/server.o -o bin/server -L. -lfcendutils -pthread
gcc obj/client.o -o bin/client -L. -lfcendutils -pthread
```

- `L.`: search for libraries in the `.` (current) directory
- `-lfcendutils`: link library libfcendutils.a
- `-pthread`: link the pthread library (use `-lpthread` if it doesn't work)

## Usage

Run the server and client files on two different computers/directories using a terminal. To transfer files to a different directory, run the server/client file from that directory.

`server`

- enter a port number when it asks you to
- enter a path to a file you wish to be sent to the other computer
- ctrl-c to exit out of the program

`client`

- enter a host (IP address), or leave empty for localhost
- enter a port number when it asks you to
- enter a path to a file you wish to be sent to the other computer
- ctrl-c to exit out of the program
