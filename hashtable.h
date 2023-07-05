/**
* Hash table headers and struct for DB.
*/

// List node struct for DB hash table (chained-hasing for handling collisions)
// it represents a entry into the DB
typedef struct _entry {
    struct _entry *next;
    struct _entry *previous;
    char *key;
    char *value;
    struct timeval tscreated; // timestamp first set
    struct timeval tsupdate; // timestamp last set
}entry_t;

// HASH TABLE FUNCTIONS
unsigned int hf(char *key); // hash function
entry_t* db_hash_table_search(entry_t *T[], char *key);
void db_hash_table_insert(entry_t *T[], entry_t **node);
void db_hash_table_delete(entry_t *T[], entry_t *node);

// LIST FUNCTIONS (needed because of chained-hashing)
entry_t* entry_list_search(entry_t *L, char *key); // search into the chained list, then return a pointer (undefined type for flexibility)
void entry_list_insert(entry_t **L, entry_t **node); // insert a node into the list
void entry_list_delete(entry_t **L, entry_t *node); // delete a node from the list
