#include "cache.h"


static cache_t *proxy_cache;
static int read_cnt;
static pthread_mutex_t r_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t w_mutex = PTHREAD_MUTEX_INITIALIZER;


static void _cache_append(node_t *data);
static void _cache_pop(node_t *data);


static node_t * _cache_lookup(char *key)
{
    if (proxy_cache->size == 0)
        return NULL;
    for (node_t *node = proxy_cache->tail->prev; node != NULL && node != proxy_cache->head; node= node->prev)
    {
        if (strcmp(node->key, key)==0)
            return node;
    }
    return NULL;
}

void cache_init(size_t cache_size)
{
    node_t *head = (node_t *) Malloc(sizeof(node_t));
    node_t *tail = (node_t *) Malloc(sizeof(node_t));

    head->prev = NULL; head->next = tail;
    tail->next = NULL; tail->prev = head;

    proxy_cache = (cache_t *) Malloc(sizeof(cache_t));
    proxy_cache->head = head;
    proxy_cache->tail = tail;
    proxy_cache->size = 0;
    proxy_cache->max_size = cache_size;

    read_cnt = 0;
}

void cache_free()
{
    for (node_t *node = proxy_cache->head; node != NULL; node = node->next)
    {
        Free(node->val);
        Free(node);
    }
    proxy_cache->size = 0;
    Free(proxy_cache);
}

node_t *cache_get(char *key)
{
    pthread_mutex_lock(&r_mutex);
    if (++read_cnt==1)
        pthread_mutex_lock(&w_mutex);
    pthread_mutex_unlock(&r_mutex);

    node_t *node = _cache_lookup(key);
    if (node != NULL)
    {
        pthread_mutex_lock(&r_mutex);
        _cache_pop(node);
        _cache_append(node);
        pthread_mutex_unlock(&r_mutex);
    }

    pthread_mutex_lock(&r_mutex);
    if (--read_cnt==0)
        pthread_mutex_unlock(&w_mutex);
    pthread_mutex_unlock(&r_mutex);

    return node;
}

void cache_set(char *key, char *val, size_t fsize)
{
    pthread_mutex_lock(&w_mutex);
    node_t *node = _cache_lookup(key);
    node_t *cached = NULL;

    if (node)
    {
        _cache_pop(node);
        Free(node->val);
    }
    else
    {
        node = (node_t *) Malloc(sizeof(node_t));
        snprintf(node->key, MAXLINE, "%s", key);
    }
    node->val = (char *) Malloc(fsize*sizeof(char));
    node->fsize = fsize;
    memcpy(node->val, val, fsize);
    if ( (proxy_cache->size)++ == proxy_cache->max_size )
    {
        cached = proxy_cache->head->next;
        _cache_pop(cached);
        Free(cached->val);
        Free(cached);
        (proxy_cache->size)--;
    }
    _cache_append(node);

    pthread_mutex_unlock(&w_mutex);
}

static void _cache_append(node_t *data)
{
    node_t *ptail = proxy_cache->tail->prev;
    ptail->next = data;
    data->prev = ptail;

    data->next = proxy_cache->tail;
    proxy_cache->tail->prev = data;
}

static void _cache_pop(node_t *data)
{
    node_t *prev = data->prev;
    node_t *next = data->next;

    data->prev = data->next = NULL;

    prev->next = next;
    next->prev = prev;
}

