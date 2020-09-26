#include "cachelab.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

static int usage(const char *argv0);
static Line* pop(Line *node);
static void add(Line *head, Line *node);
static void update(Line *head, Line *node);
static int cacheLookup(Cache *cache, address_t addr);
static Cache* initCache(int m, int s, int E, int b, int verbose);
static void freeCache(Cache *cache);
static void readTrace(const char *filename, Cache *cache);

typedef struct counterStruct {
    int hit;
    int miss;
    int evict;
} Counter;

Counter count = {0, 0, 0};


int main(int argc, char **argv)
{
    int opt;
    int verbose = 0, s = 0, E = 0, b = 0, trace_flag = 0;
    char trace[1024];
    while ((opt = getopt(argc, argv, "s:E:b:t:hv")) != -1 )
    {
        switch (opt)
        {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                snprintf(trace,sizeof(trace),"%s",optarg);
                trace_flag = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                usage(argv[0]);
                break;
        }
    }
    if (!s || !E || !b || !trace_flag)
    {
        printf("%s: Missing required command line argument\n",argv[0]);
        usage(argv[0]);
    }

    Cache *cache = initCache(64, s, E, b, verbose);
    readTrace(trace, cache);
    printSummary(count.hit, count.miss, count.evict);
    freeCache(cache);
    return 0;
}

static int usage(const char *argv0)
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n"
           "Options:\n"
           " -h\t\tPrint this help message.\n"
           " -v\t\tOptional verbose flag.\n"
           " -s <num>\tNumber of set index bits.\n"
           " -E <num>\tNumber of lines per set.\n"
           " -b <num>\tNumber of block offset bits.\n"
           " -t <file>\tTrace file.\n\n"
           "Examples:\n"
           " linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n"
           " linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv0, argv0, argv0);
    exit(EXIT_FAILURE);
}


static Line* pop(Line *node)
{
    Line *prev = node->prev, *next = node->next;
    prev->next = next;
    next->prev = prev;
    node->prev = NULL; node->next = NULL;
    return node;
}

static void add(Line *head, Line *node)
{
    Line *ptr = head->next;
    head->next = node;
    node->prev = head;
    node->next = ptr;
    ptr->prev = node;
}

static void update(Line *head, Line *node)
{
    node = pop(node);
    add(head, node);
}

static int cacheLookup(Cache *cache, address_t addr)
{
    int s = cache->s, b = cache->b, m = cache->m;
    int t = m - (s+b);
    unsigned long long int tag_index = addr >> (s+b);
    unsigned long long int set_index = (addr << t) >> (t+b);;
    Set set = cache->cacheSets[set_index];
    Line *head = set.head, *node = NULL;
    for (Line *line = head; line!=NULL; line=line->next) // iterate through the given set
    {
        if (line->valid && line->tag==tag_index) // look if the given line has valid bit and tag_index
        {
            count.hit++; if (cache->verbose) printf(" hit");
            // Update LRU
            update(head, line);
            return 1;
        }
    }
    count.miss++; if (cache->verbose) printf(" miss");
    if ((cache->cacheSets[set_index].count)==cache->E) // check if the line is full. If so, evict the tail line
    {
        count.evict++; if (cache->verbose) printf(" eviction");
        node = pop(set.tail->prev); // get the evicted line
    } else cache->cacheSets[set_index].count++;
    if (node==NULL) node = (Line *) malloc(sizeof(Line));
    node->valid = 1; node->tag = tag_index;
    node->next = NULL; node->prev = NULL;
    add(head, node); // add as the latest used line
    
    return 0;
}

static Cache* initCache(int m, int s, int E, int b, int verbose)
{
    Cache *cache = (Cache *) malloc (sizeof(Cache));
    cache->m = m; cache->s = s; cache->E = E; cache->b = b; cache->verbose = verbose;
    cache->cacheSets = (Set *) malloc ( (1<<s)*sizeof(Set));
    int nSets = (1<<s);
    for (int idx=0;idx<nSets;idx++)
    {
        cache->cacheSets[idx].count = 0;
        cache->cacheSets[idx].head = (Line *) malloc (sizeof(Line));
        cache->cacheSets[idx].tail = (Line *) malloc (sizeof(Line));
        cache->cacheSets[idx].head->prev = NULL; cache->cacheSets[idx].head->valid = 0; cache->cacheSets[idx].head->tag = 0;
        cache->cacheSets[idx].tail->next = NULL; cache->cacheSets[idx].tail->valid = 0; cache->cacheSets[idx].tail->tag = 0;
        cache->cacheSets[idx].head->next = cache->cacheSets[idx].tail;
        cache->cacheSets[idx].tail->prev = cache->cacheSets[idx].head;
    }
    return cache;
}

static void freeCache(Cache *cache)
{
    if (cache==NULL)    return;
    int nSets = 1<<(cache->s);
    Set set;
    for (int i=0;i<nSets;i++)
    {
        set = cache->cacheSets[i];
        for (Line *node=set.head->next; node!=NULL; node=node->next)
            free(node->prev);
        free(set.tail);
    }
    free(cache->cacheSets);
    free(cache);
}

static void readTrace(const char *filename, Cache *cache)
{
    FILE *fp;
    char identifier;
    address_t address;
    int size;	
    if ((fp=fopen(filename,"r")) == NULL)
    {
        printf("%s: No such file or directory\n", filename);
        freeCache(cache);
        exit(EXIT_FAILURE);
    }
    while (fscanf(fp," %c %llx,%d", &identifier, &address, &size)>0)
    {
        if (cache->verbose) printf("%c %llx,%d", identifier, address, size);
        switch (identifier)
        {
            case 'L':
                cacheLookup(cache, address);
                break;
            case 'S':
                cacheLookup(cache, address);
                break;
            case 'M':
                cacheLookup(cache, address);
                cacheLookup(cache, address);
                break;
            default:
                continue;
        }
        if (cache->verbose) printf("\n");
    }
    fclose(fp);
}
