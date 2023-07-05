#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "clientconst.h"

void print_usage();


int main(int argc, char* argv[])
{
    int sock;
    int bytes_read = 0;
    int rc;
    struct sockaddr_un server;

    // buffers to read/write data from/to the daemon
    char commandbuff[BUFF_SIZE]; // user command buffer
    char resbuff[BUFF_SIZE]; // response from the server buffer

    memset(commandbuff, 0, BUFF_SIZE * sizeof(char)); // clean the command buffer
    memset(resbuff, 0, BUFF_SIZE * sizeof(char)); // clean the response buffer

    if(argc < 2) {
        print_usage();
        exit(1);
    }

    // compose the command
    strncpy(commandbuff, argv[1], BUFF_SIZE);
    strcat(commandbuff, " ");  // "get "
    strncat(commandbuff, argv[2], BUFF_SIZE); // "get key"
    if(strncmp(argv[1], "set", BUFF_SIZE) == 0) {
        if(argc < 4) {
            fputs("Incorrect usage of 'set' command.\n\n", stdout);
            print_usage();
            exit(1);
        }
        
        strcat(commandbuff, " ");  // "set key "
        strncat(commandbuff, argv[3], BUFF_SIZE); // "set key value"
    }
    commandbuff[BUFF_SIZE - 1] = '\0';


    
    /* SOCKET INITIALIZATION */
    // creating a new socket
    memset(&server, 0, sizeof(struct sockaddr_un));
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        fputs("Error opening stream socket.\n", stdout);
        exit(1);
    }
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCKET_NAME);

    // connecting to the socket created in the deamon
    // the file name is used to specify the correct socket
    rc = connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un));
    if (rc < 0) {
        fputs("Error while connecting socket.\nMaybe kvdbd is not running...\n", stdout);
        close(sock);
        exit(1);
    }

    /* COMMAND SEND */
    // send command to the daemon
    rc = write(sock, commandbuff, BUFF_SIZE);
    if(rc == -1) {
        perror("Error while sending command to the daemon.");
        close(sock);
        exit(1);
    }

    // wait for the response
    bytes_read = read(sock, resbuff, BUFF_SIZE);
    if(bytes_read == -1)
        perror("Error while reading response from the daemon.");
    else
        puts(resbuff);

    close(sock);
    
    return 0;
}


void print_usage() {
    fputs("Usage: kvdb <command>\n", stdout);
    fputs("Possible commands:\n", stdout);
    fputs("            set {key} {value}\n", stdout);
    fputs("            get {key}\n", stdout);
    fputs("            del {key}\n", stdout);
    fputs("            ts {key}\n", stdout);
    fputs("\nExample:\n", stdout);
    fputs("       kvdb get pen\n", stdout);
}
