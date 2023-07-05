/**
* Constants declaration.
*/

// Socket related constants
#define SOCKET_NAME "/tmp/kvdb.sock"
#define BUFF_SIZE 4096
#define BACKLOG 10

// Thread related constants
#define OK_CODE 0
#define ERR_CODE 1
#define QUIT_CODE 2

// Hash table related constants
#define FNV_OFFSET_BASIS 2166136261
#define FNV_PRIME 16777619
#define DB_TABLE_SIZE 499
