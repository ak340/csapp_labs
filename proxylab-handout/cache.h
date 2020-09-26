/* begin cache.h */

#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

typedef struct CacheNode node_t;
typedef struct CacheStruct cache_t;


struct CacheStruct{
    node_t *head;
    node_t *tail;
    size_t size;
    size_t max_size;
};

struct CacheNode{
    char key[MAXLINE];
    char *val;
    node_t *prev;
    node_t *next;
    size_t fsize;
};

void cache_init(size_t cache_size);

void cache_free();

node_t* cache_get(char *key);

void cache_set(char *key, char *val, size_t fsize);

#endif
