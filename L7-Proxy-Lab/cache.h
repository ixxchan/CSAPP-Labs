#include "csapp.h"
#include <sys/time.h>

struct cache_block {
	long timeid;
	char *content;
	int content_length;
	char *uri;
	struct cache_block *next, *prev;
};
typedef struct cache_block cache_block_t;

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static cache_block_t *head;
static int total_size;
static long cache_time;
static pthread_rwlock_t rwlock;

void cache_init();
/* if success (hit) return content length */
int cache_find(char *uri, char *buf);
void cache_add(char *uri, char *objectbuf, int objectlen);