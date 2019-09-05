#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void csim(int vflag, int s, int E, int b,
          const char *tracefile, int *hits, int *misses, int *evictions)
{
    FILE *fp;
    char ch;
    unsigned long long address;
    int size;
    typedef struct block
    {
        long long tag;
        int valid;
    } block;
    block **cache;

    // initialize counters
    *hits = *misses = *evictions = 0;

    // initialize cache
    cache = (block **)malloc((1 << s) * sizeof(block *));
    for (int i = 0; i < (1 << s); ++i)
    {
        cache[i] = (block *)malloc(E * sizeof(block));
        for (int j = 0; j < E; ++j)
            cache[i][j].valid = 0;
    }

    fp = fopen(tracefile, "r");
    if (fp == NULL)
    {
        perror("No such file or directory.\n");
        exit(EXIT_FAILURE);
    }

    int i, j, hitflag = 0, setid;
    long long tg;
    while ((ch = fgetc(fp)) != EOF)
    {
        if (ch == ' ')
        {
            fscanf(fp, "%c", &ch);
            fscanf(fp, "%llx,%d", &address, &size);
            if (vflag)
                printf("%c %llx,%d ", ch, address, size);
            tg = address >> (s + b);
            setid = (address >> b) & ((1 << s) - 1);
            hitflag = 0;
            // search tg in setid
            for (i = 0; cache[setid][i].valid && i < E; ++i)
                if (cache[setid][i].tag == tg)
                {
                    if (vflag)
                        printf("hit");
                    ++(*hits);
                    hitflag = 1;
                    // put tg to the last of the set
                    // the latter the position, the more recently the tag is used
                    for (j = i; cache[setid][j + 1].valid && j < E - 1; ++j)

                        cache[setid][j].tag = cache[setid][j + 1].tag;
                    cache[setid][j].tag = tg;
                    break;
                }
            if (!hitflag)
            {
                if (vflag)
                    printf("miss");
                ++(*misses);
                if (i == E)
                {
                    if (vflag)
                        printf(" eviction");
                    ++(*evictions);
                    // LRU replacement
                    for (j = 0; j < E - 1; ++j)
                        cache[setid][j].tag = cache[setid][j + 1].tag;
                    cache[setid][E - 1].tag = tg;
                }
                else
                // empty positions available
                {
                    cache[setid][i].valid = 1;
                    cache[setid][i].tag = tg;
                }
            }
            if (ch == 'M')
            {
                if (vflag)
                    printf(" hit");
                ++(*hits);
            }
            if (vflag)
                printf("\n");
        }
        else
        {
            fscanf(fp, "%llx,%d", &address, &size);
            // do nothing
        }
        // clean blanks and "\n"
        while (fgetc(fp) != '\n')
            ;
    }
    fclose(fp);

    for(i = 0; i < (1 << s); ++i)
        free(cache[i]);
    free(cache);
}

int main(int argc, char *argv[])
{
    int svalue = 0;
    int Evalue = 0;
    int bvalue = 0;
    char *tvalue = NULL;
    int vflag = 0;
    int c;
    int hflag = 0;
    while (!hflag && (c = getopt(argc, argv, ":hvs:E:b:t:")) != -1)
        switch (c)
        {
        case 'v':
            vflag = 1;
            break;
        case 's':
            svalue = atoi(optarg);
            break;
        case 'E':
            Evalue = atoi(optarg);
            break;
        case 'b':
            bvalue = atoi(optarg);
            break;
        case 't':
            tvalue = optarg;
            break;
        case ':':
            printf("option requires an argument -- '%c'\n", optopt);
            hflag = 1;
            break;
        case '?':
            printf("invalid option -- `%c'\n", optopt);
            hflag = 1;
            break;
        case 'h':
        default:
            hflag = 1;
        }

    if (!hflag && (!svalue || !Evalue || !bvalue))
    {
        printf("Missing required command line argument\n");
        hflag = 1;
    }
    if (hflag)
    {
        printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
               "Options:\n"
               "-h         Print this help message.\n"
               "-v         Optional verbose flag.\n"
               "-s <num>   Number of set index bits.\n"
               "-E <num>   Number of lines per set.\n"
               "-b <num>   Number of block offset bits.\n"
               "-t <file>  Trace file.\n"
               "\n"
               "Examples:\n"
               "linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
               "linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
        return 1;
    }

    int hits, misses, evictions;
    csim(vflag, svalue, Evalue, bvalue, tvalue, &hits, &misses, &evictions);
    printSummary(hits, misses, evictions);
    return 0;
}
