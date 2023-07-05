/**
* Hash table structs and functions for DB.
*/

#include <stdlib.h>
#include <string.h>

#include "const.h"
#include "hashtable.h"


/* HASH TABLE */
/**
* Hash function: FNV-1a (good speed and collision avoidance)
*/
unsigned int hf(char *key) {
    unsigned int hash = FNV_OFFSET_BASIS;
    int i;

    for(i=0; *(key + i)!='\0'; i++) {
        hash ^= *(key + i);
        hash *= FNV_PRIME;
    }

    return hash;
}

/**
* Search into the hash table, then return a pointer
*/
entry_t* db_hash_table_search(entry_t *T[], char *key) {
    unsigned int hash=0;
    entry_t *x=NULL;

    hash=hf(key);
    hash%=DB_TABLE_SIZE;
    x=entry_list_search(T[hash], key);

    return x;
}

/**
* Insert a node into the hash table
*/
void db_hash_table_insert(entry_t *T[], entry_t **node) {
    unsigned int hash=0;

    hash=hf((*node)->key);
    hash%=DB_TABLE_SIZE;
    entry_list_insert(&T[hash], node);
}

/**
* Delete a node from the hash table
*/
void db_hash_table_delete(entry_t *T[], entry_t *node) {
    unsigned int hash=0;

    hash=hf(node->key);
    hash%=DB_TABLE_SIZE;
    entry_list_delete(&T[hash], node);
}


/* CHAINED-LIST */

/**
* Search into the chained list, then return a pointer
*/
entry_t* entry_list_search(entry_t *L, char *key) {

    while(L!=NULL && strcmp(L->key, key)!=0) {
        L=L->next;
    }
    return L;
}

/**
* Insert a node into the list
*/
void entry_list_insert(entry_t **L, entry_t **node) {

    (*node)->next=(*L);
    if((*L)!=NULL) {
        (*L)->previous = (*node);
    }
    (*L)=(*node);
    (*L)->previous=NULL;
}

/**
* Delete a node from the list
*/
void entry_list_delete(entry_t **L, entry_t *node) {

    if(node->previous!=NULL) {
        node->previous->next=node->next;
    }
    else {
        (*L)=node->next;
    }

    if(node->next!=NULL) {
        node->next->previous=node->previous;
    }

    free(node->key);
    free(node->value);
    free(node);
    node=NULL;
}
