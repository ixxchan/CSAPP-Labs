/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Tianxiao Shen",
    /* First member's email address */
    "xxchan@sjtu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define GET(p)      (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define PACK(size, alloc) ((size) | (alloc))
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

#define PREDP(bp) ((char *)(bp))
#define SUCCP(bp) ((char *)(bp)+WSIZE)

#define GET_PRED(bp) (*(char **)PREDP(bp))
#define GET_SUCC(bp) (*(char **)SUCCP(bp))

#define PUTP(p, pval) (*(char **)(p) = (char *)(pval))

#define CHECK(bp) (printf("bp:%u, SIZE:%d, ALLOC:%d\n",(unsigned int)bp, GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp))))
#define DEBUG 0
static char *heap_listp = 0;

int mm_check(void)
{
    void *bp = heap_listp;
    int cnt = 0;
    while(GET(HDRP(bp)) != PACK(0,1))
    {
        printf("check node %u, SIZE %u, ALLOC %u, PRED %u, SUCC %u\n", 
            (unsigned int)bp, (unsigned int)GET_SIZE(HDRP(bp)),(unsigned int)GET_ALLOC(HDRP(bp)),(unsigned int)GET_PRED(bp),(unsigned int)GET_SUCC(bp));
        bp = GET_SUCC(bp);
        cnt++;
        if(cnt > 10)
        {
            printf("maybe dead loop"); return -1;
        }
    }
    printf("check epilogue %u, SIZE %u, ALLOC %u, PRED %u\n", (unsigned int)bp,(unsigned int)GET_SIZE(HDRP(bp)),(unsigned int)GET_ALLOC(HDRP(bp)),(unsigned int)GET_PRED(bp)); 
    return 0;
}
void delete_node(void *bp);
inline void delete_node(void *bp)
{
    PUTP(SUCCP(GET_PRED(bp)), GET_SUCC(bp));
    PUTP(PREDP(GET_SUCC(bp)), GET_PRED(bp));
}

void insert_node(void *bp);
/* insert after prologue */
inline void insert_node(void *bp)
{
    PUTP(PREDP(bp), heap_listp);
    PUTP(SUCCP(bp), GET_SUCC(heap_listp));
    PUTP(PREDP(GET_SUCC(bp)),bp);
    PUTP(SUCCP(heap_listp), bp);
}

static void *coalesce(void *bp)
{
    if(DEBUG)
        {printf("before coalesce %u\n",(unsigned int)bp); mm_check(); }
    size_t size = GET_SIZE(HDRP(bp));
    unsigned int prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    unsigned int next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    if(DEBUG)printf("prev %u %u next %u %u\n",PREV_BLKP(bp),prev_alloc,NEXT_BLKP(bp),next_alloc);
    if (prev_alloc && next_alloc)
    {
        return bp;
    }

    else if (prev_alloc && !next_alloc)
    {
        delete_node(bp);
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    }

    else if (!prev_alloc && next_alloc)
    {
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
    }

    else
    {
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); 
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
    }           
   
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_node(bp);
     if(DEBUG)
        {printf("after coalesce %u\n",(unsigned int)bp); mm_check(); }
    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size = (words % 2)? (words + 1) * WSIZE : words * WSIZE;

    /* Allocate even number of words to maintain alignment */    
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    bp -= WSIZE;
    PUT(HDRP(bp), PACK(size, 0)); /* free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* free block footper */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */
    PUTP(PREDP(NEXT_BLKP(bp)), GET_PRED(bp)); /* new epilogue's pred -> old epilogue's pred */
    PUTP(SUCCP(GET_PRED(bp)), NEXT_BLKP(bp)); /* old epilogue's pred's succ -> new epilogue */
    if(DEBUG){printf("before insert \n");mm_check();}
    insert_node(bp);
   
    /* Coalesce if the previous block was free */
    return coalesce(bp); 
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(7*WSIZE)) == (void *) -1)
        return -1;
    PUT(heap_listp + 0*WSIZE, 0); /* padding */
    PUT(heap_listp + 1*WSIZE, PACK(4*WSIZE,1)); /* prologue header */
    PUTP(heap_listp + 2*WSIZE, NULL); /* prologue pred */
    PUTP(heap_listp + 3*WSIZE, heap_listp + 6*WSIZE); /* prologue succ */
    PUT(heap_listp + 4*WSIZE, PACK(4*WSIZE,1));    /* prologue footer */
    PUT(heap_listp + 5*WSIZE, PACK(0, 1)); /* epilogue header */
    PUTP(heap_listp + 6*WSIZE, heap_listp + 2*WSIZE); /* epilogue pred */
    heap_listp += 2 * WSIZE;
    if (DEBUG)
    {    
        printf("heap_listp init %u\n",(unsigned int)heap_listp);
        mm_check();
    }
    /* Extend the heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* first fit search */
static void *find_fit(size_t aligned_size)
{
   // //printf("\nfind fit heaplistp %u, SUCC %u GETSUCC %u\n",heap_listp,SUCCP(heap_listp),GET_SUCC(heap_listp));
    char *bp = GET_SUCC(heap_listp);
   // //printf("find fit %u bp %u HDR%u\n",aligned_size,(unsigned int)bp, (HDRP(bp)));
    while(GET_ALLOC(HDRP(bp)) == 0)
    {
        if(GET_SIZE(HDRP(bp)) >= aligned_size)
            return bp;
        else
            bp = GET_SUCC(bp);
    }
    return NULL;
}

static void place(void *bp, size_t aligned_size)
{
    //printf("place %u, %u, HDR %u SIZE %u ",bp,aligned_size,HDRP(bp),GET_SIZE(HDRP(bp)));
    //printf("brk %u\n",mem_sbrk(0));
    size_t prev_size = GET_SIZE(HDRP(bp));
    size_t remainder_size = prev_size - aligned_size;
    char *succp = GET_SUCC(bp);
    char *predp = GET_PRED(bp);

    if(remainder_size < 4 * WSIZE)
    {
        PUTP(SUCCP(predp), succp);
        PUTP(PREDP(succp), predp);
        PUT(HDRP(bp), PACK(prev_size, 1));
        PUT(FTRP(bp), PACK(prev_size, 1));
    }
    else /* need splitting */
    {
        if(DEBUG)printf("split %u, succp %u, predp %u\n",(unsigned int)bp,(unsigned int)succp,(unsigned int)predp);
        PUT(HDRP(bp), PACK(aligned_size, 1));
        PUT(FTRP(bp), PACK(aligned_size, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(remainder_size, 0));
        PUT(FTRP(bp), PACK(remainder_size, 0));
        PUTP(SUCCP(bp), succp);
        PUTP(PREDP(bp), predp);
        PUTP(SUCCP(predp), bp);
        PUTP(PREDP(succp), bp);
    }
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    char *bp;
    size_t extend_size;
    if (DEBUG) {printf("malloc %u\n",size);mm_check();}
    if(size == 0)
        return NULL;

    if(size <= DSIZE)
        size = 2 * DSIZE;
    else
        size = ALIGN(size + DSIZE);
    
    if ((bp = find_fit(size)) != NULL)
    {
        place(bp, size);
    }
    else
    {
        extend_size = size > CHUNKSIZE? size : CHUNKSIZE;
        if ((bp = extend_heap(extend_size/WSIZE)) == NULL)
            return NULL;
        place(bp, size);
    }
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if(DEBUG){printf("free %u\n",ptr);mm_check();}
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    insert_node(ptr);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(ptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

