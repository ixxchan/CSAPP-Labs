#include "cache.h"

void cache_init() {
    head = NULL;
    total_size = 0;
    cache_time = 0;
    Pthread_rwlock_init(&rwlock, NULL);
}

/* if success (hit) return content length */
int cache_find(char *uri, char *buf) {
    cache_block_t *p;
    int len = 0;

    Pthread_rwlock_rdlock(&rwlock);
    for(p = head; p; p = p->next) {
        if (strcmp(p->uri, uri) == 0) {
            p->timeid = cache_time++;
            len= p->content_length;
            memcpy(buf, p->content, len);
            break;
        }
    }
    Pthread_rwlock_unlock(&rwlock);
    return len;
}

void cache_add(char *uri, char *objectbuf, int objectlen) {
    cache_block_t *p;

    Pthread_rwlock_wrlock(&rwlock);
	while (total_size + objectlen >= MAX_CACHE_SIZE) { /* space not enough */
		struct cache_block *evict = head;
		for (p = head; p; p = p->next) {
			if (p->timeid < evict->timeid)
				evict = p;
		}
		total_size -= evict->content_length;
		if (evict->prev)
			evict->prev->next = evict->next;
		if (evict->next)
			evict->next->prev = evict->prev;
		if (evict == head)
			head = evict->next;
		free(evict->content);
		free(evict->uri);
		free(evict);
	}
	total_size += objectlen;
	p = (cache_block_t *)Malloc(sizeof(cache_block_t));
	p->timeid = cache_time++;
	p->content = (char *)Malloc(objectlen);
	p->content_length = objectlen;
	memcpy(p->content, objectbuf, objectlen);
	p->uri = (char *)Malloc(strlen(uri) + 1);
	strcpy(p->uri, uri);
	if (head)
		head->prev = p;
	p->next = head;
	p->prev = NULL;
	head = p;
    Pthread_rwlock_unlock(&rwlock);
}