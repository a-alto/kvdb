#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>

#include "const.h"
#include "hashtable.h"

// DB functions
char *get_input_key(char *, int);
char *get_input_val(char *);
char *getkey(char *);
char *setkeyval(char *, char *);
char *delkey(char *);
char *tskey(char *);

// thread function
void *connection_handler(void *);

/* GLOBAL STATE */
entry_t *db_hashtable[DB_TABLE_SIZE] = { NULL }; // hash table declaration and initialization
pthread_rwlock_t locks[DB_TABLE_SIZE]; // locks for synchronization


int main()
{
    int server_sock; // server socket
    int client_sock; // client socket
    int rc; // control value
    struct sockaddr_un server;
    struct sockaddr_un client;
    int sockaddr_len;
    pthread_t thread_id;
    int i; // index for loops

    /* INITIALIZE LOCKS */
    for(i=0; i<DB_TABLE_SIZE; i++)
        pthread_rwlock_init(&locks[i], NULL);

    /* SOCKET INITIALIZATION */
    // creating a new socket
    memset(&server, 0, sizeof(struct sockaddr_un));
    memset(&client, 0, sizeof(struct sockaddr_un));
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Error creating socket.");
        exit(1);
    }
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCKET_NAME);
    sockaddr_len = sizeof(struct sockaddr_un);

    // binding the socket to the file
    unlink(SOCKET_NAME);
    rc = bind(server_sock, (struct sockaddr *) &server, sockaddr_len);
    if (rc == -1) {
        perror("Error binding the socket.");
        close(server_sock);
        exit(1);
    }
    
    // start listening for connections
    rc = listen(server_sock, BACKLOG);
    if (rc == -1) {
        perror("Error listening for connections.");
        close(server_sock);
        exit(1);
    }

    fputs("Server listening...\n", stdout);


    /* SERVER LISTENING AND SERVING REQUESTS */
    // accept connection from the client and serve it with a separate thread
    while((client_sock = accept(server_sock, (struct sockaddr *) &client, (socklen_t *) &sockaddr_len))) {
        if(pthread_create(&thread_id, NULL, connection_handler, (void*) &client_sock) < 0) {
            perror("Error: could not create a new thread.");
            return 1;
        }

        // now accepting other connections
    }
     
    if (client_sock < 0)
        perror("Error while accepting new connections.\nTerminating...");


    /* CLOSING SERVER SOCKET */
    fputs("kvdbd is terminating...\n", stdout);
    unlink(SOCKET_NAME);
    close(server_sock);
    fputs("kvdbd terminated successfully\n", stdout);

    /* DESTROYING LOCKS */
    for(i=0; i<DB_TABLE_SIZE; i++)
        pthread_rwlock_destroy(&locks[i]);

    return 0;
}


void *connection_handler(void *socket_desc) {
    // get the socket descriptor
    int client_sock = *(int *)socket_desc;
    int bytes_read = 0;
    int rc; // control value
    int i; // index for generic lookups

    char inputbuff[BUFF_SIZE]; // input buffer for commands TODO change to __thread char inputbuff[BUFF_SIZE] (maybe with static)
    char outputbuff[BUFF_SIZE]; // output buffer for sending response to the client

    char *key = NULL; // key by which to search into the DB
    char *val = NULL; // retrieved value from the DB or value to set

    memset(inputbuff, 0, BUFF_SIZE); // clean the input buffer
    memset(outputbuff, 0, BUFF_SIZE); // clean the output buffer
    
    // receive the command from client
    bytes_read = read(client_sock, inputbuff, BUFF_SIZE);
    if(bytes_read == -1) {
        perror("Error while reading command from the client.");
        close(client_sock); // close connection with the client
        pthread_exit((void *)ERR_CODE);
    }
    if(bytes_read == 0) {
        close(client_sock); // close connection with the client
        pthread_exit((void *)OK_CODE);
    }
    inputbuff[bytes_read] = '\0'; // end of string
    
    if(strncmp(inputbuff, "get", 3) == 0) {
        key = get_input_key(inputbuff, 4);

        // get the index in HT and use it to find the correspondent lock
        i = hf(key) % DB_TABLE_SIZE;
        pthread_rwlock_rdlock(&locks[i]);
        val = getkey(key);
        pthread_rwlock_unlock(&locks[i]);

        if(val != NULL)
            strncpy(outputbuff, val, BUFF_SIZE); // retrun retrieved value to the client
        else
            strcpy(outputbuff, "[Element not existing]");

        // print log
        printf("GET %s: %s\n", key, outputbuff);
    }
    else if(strncmp(inputbuff, "set", 3) == 0) {
        key = get_input_key(inputbuff, 4);
        val = get_input_val(inputbuff);

        // get the index in HT and use it to find the correspondent lock
        i = hf(key) % DB_TABLE_SIZE;
        pthread_rwlock_wrlock(&locks[i]);
        setkeyval(key, val);
        pthread_rwlock_unlock(&locks[i]);

        // retrun retrieved key: value to the client
        strncpy(outputbuff, key, BUFF_SIZE);
        strcat(outputbuff, ": ");
        strncat(outputbuff, val, BUFF_SIZE);

        // print log
        printf("SET %s\n", outputbuff);
    }
    else if(strncmp(inputbuff, "del", 3) == 0) {
        key = get_input_key(inputbuff, 4);

        // get the index in HT and use it to find the correspondent lock
        i = hf(key) % DB_TABLE_SIZE;
        pthread_rwlock_wrlock(&locks[i]);
        delkey(key);
        pthread_rwlock_unlock(&locks[i]);

        strncpy(outputbuff, key, BUFF_SIZE); // retrun deleted key to the client

        // print log
        printf("DEL %s\n", outputbuff);
    }
    else if(strncmp(inputbuff, "ts", 2) == 0) {
        key = get_input_key(inputbuff, 3);

        // get the index in HT and use it to find the correspondent lock
        i = hf(key) % DB_TABLE_SIZE;
        pthread_rwlock_rdlock(&locks[i]);
        val = tskey(key); // val will be the formatted timestamps
        pthread_rwlock_unlock(&locks[i]);

        strncpy(outputbuff, val, BUFF_SIZE); // retrun retrieved timestamps to the client

        // free the timestamp message
        free(val);
        val = NULL;

        // print log
        printf("TS %s: %s\n", key, outputbuff);
    }
    else {
        strcpy(outputbuff, "Command not supported.");

        // print log
        printf("[NOT SUPPORTED] %s: %s\n", inputbuff, outputbuff);
    }

    // send response to the client
    rc = write(client_sock, outputbuff, strlen(outputbuff));
    if (rc == -1) {
        perror("Error while sending response to the client.");
        close(client_sock);
        pthread_exit((void *)ERR_CODE);
    }
    
    close(client_sock);
    pthread_exit((void *)OK_CODE);
}



/* DB FUNCTIONS */

/**
* Given an input command, and a starting index (based on the command)
* returns the key for that command
*/
char *get_input_key(char *inputstr, int starting_index) {
    char *key;
    int inputlen;
    int i = starting_index; // starting from the first character after "get "
    int ki = 0; // key index

    inputlen = strlen(inputstr);
    key = (char *)malloc(inputlen * sizeof(char));
    while(i < inputlen && *(inputstr + i) != ' ' && *(inputstr + i) != '\0') {
        *(key + ki) = *(inputstr + i);
        i++;
        ki++;
    }

    *(key + ki) = '\0';

    return key;
}

/**
* Given an input command,nreturns the value for that command.
*/
char *get_input_val(char *inputstr) {
    char *val;
    int inputlen;
    int i = 4; // starting from the first character after "get "
    int ki = 0; // val index

    inputlen = strlen(inputstr);
    val = (char *)malloc(inputlen * sizeof(char));
    // go after the key
    while(i < inputlen && *(inputstr + i) != ' ' && *(inputstr + i) != '\0')
        i++;
    i++;

    // get the value for the command
    while(i < inputlen && *(inputstr + i) != ' ' && *(inputstr + i) != '\0') {
        *(val + ki) = *(inputstr + i);
        i++;
        ki++;
    }

    *(val + ki) = '\0';

    return val;
}

/**
* Given a key, get the value for that key from the DB.
*/
char *getkey(char *key) {
    entry_t *el = NULL;

    el=db_hash_table_search(db_hashtable, key);

    if(el != NULL)
        return el->value;
    
    return NULL;
}

/**
* Given a key and a value, set the value for that key to the DB.
*/
char *setkeyval(char *key, char *val) {
    entry_t *aux;
    struct timeval nowtime; // aux var for time

    aux=db_hash_table_search(db_hashtable, key);

    if(aux == NULL) {
        // calculate the current time
        gettimeofday(&nowtime, NULL);
        // create a new entry instance
        aux=(entry_t *)malloc(sizeof(entry_t));
        aux->next=NULL;
        aux->previous=NULL;
        aux->key=strdup(key);
        aux->value=strdup(val);
        aux->tscreated=nowtime;
        aux->tsupdate=nowtime;

        // insert new entry in the hash table
        db_hash_table_insert(db_hashtable, &aux);
    }
    else {
        // calculate the current time
        gettimeofday(&nowtime, NULL);
        // update the existing entry instance
        free(aux->value);
        aux->value=NULL;
        aux->value=strdup(val);
        aux->tsupdate=nowtime;
    }

    return key; // retrun the ky for which the 'set' occurred
}

/**
* Given a key, delete that key from the DB.
*/
char *delkey(char *key) {
    entry_t *el=NULL;
    int i;

    el=db_hash_table_search(db_hashtable, key);
    if(el != NULL) {
        db_hash_table_delete(db_hashtable, el); // node is freed by the function
        el=NULL;
    }

    return key; // retrun the ky for which the 'del' occurred
}

/**
* Given a key, get the timestamp for when the key was first set
* and the timestamp for when the key was last updated.
*/
char *tskey(char *key) {
    entry_t *el = NULL;
    time_t unixtime;
    struct tm * timeptr;
    int millisec;
    char millistr[4];
    char datestr_created[24]; // string for ISO-formatted date
    char datestr_update[24]; // string for ISO-formatted date
    char *timestamp_message = (char *)malloc(69 * sizeof(char)); // eventual message to return

    el=db_hash_table_search(db_hashtable, key);

    if(el != NULL) {
        // get and format created date
        unixtime = el->tscreated.tv_sec;
        timeptr = localtime(&unixtime);
        millisec = (int) (el->tscreated.tv_usec / (long)1000);

        strftime(datestr_created, sizeof(datestr_created) - 1, "%Y-%m-%d %H-%M-%S.", timeptr); // convert current ISO date to string
        datestr_created[20] = '\0';

        sprintf(millistr, "%d", millisec);
        millistr[3] = '\0';

        strcat(datestr_created, millistr);
        timeptr = NULL;

        // get and format update date
        unixtime = el->tsupdate.tv_sec;
        timeptr = localtime(&unixtime);
        millisec = (int) (el->tsupdate.tv_usec / (long)1000);

        strftime(datestr_update, sizeof(datestr_update) - 1, "%Y-%m-%d %H-%M-%S.", timeptr); // convert current ISO date to string
        datestr_update[20] = '\0';

        sprintf(millistr, "%d", millisec);
        millistr[3] = '\0';

        strcat(datestr_update, millistr);
        timeptr = NULL;

        // final string construction
        strcpy(timestamp_message, "First set: ");
        strcat(timestamp_message, datestr_created);
        strcat(timestamp_message, "\n");
        strcat(timestamp_message, "Last set: ");
        strcat(timestamp_message, datestr_update);
    }
    else
        strcpy(timestamp_message, "[Element not existing]");
    
    return timestamp_message;
}
