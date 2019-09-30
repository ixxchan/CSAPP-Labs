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

#define CHECK(bp) (printf("bp:%u, SIZE:%d, ALLOC:%d\n",(unsigned int)bp, GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp))))

static char *heap_listp = 0;

int mm_check(void)
{
    void *bp = heap_listp;
    printf("check heap_listp");CHECK(heap_listp);
    while(GET(HDRP(bp)) != PACK(0,1))
    {
        if(GET(HDRP(bp)) == 0)
        {
            printf("FIND SIZE 0 ALLOC 0 BLOCK %u!\n",(unsigned int)bp);
            return -1;
        }
        else
            bp = NEXT_BLKP(bp);
    }
    printf("SAFE!\n");
    return 0;
}

static void *coalesce(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    unsigned int prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    unsigned int next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    if (prev_alloc && next_alloc)
    {
    }

    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc)
    {
        bp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); 
        bp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }           
    
    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size = (words % 2)? (words + 1) * WSIZE : words * WSIZE;

    /* Allocate even number of words to maintain alignment */    
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    PUT(HDRP(bp), PACK(size, 0)); /* free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* free block footper */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue */

    /* Coalesce if the previous block was free */
    return coalesce(bp); 
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1)
        return -1;
    PUT(heap_listp, 0);                       /* padding */
    PUT(heap_listp + 1*WSIZE, PACK(DSIZE,1)); /* prologue header */
    PUT(heap_listp + 2*WSIZE, PACK(DSIZE,1)); /* prologue footer */
    PUT(heap_listp + 3*WSIZE, PACK(0, 1));    /* epilogue header */
    heap_listp += 2 * WSIZE;

    /* Extend the heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* first fit search */
static void *find_fit(size_t aligned_size)
{
    char *bp = heap_listp;
    while(GET(HDRP(bp)) != PACK(0,1))
    {
        if(GET_ALLOC(HDRP(bp)) == 0 && GET_SIZE(HDRP(bp)) >= aligned_size)
            return bp;
        else
            bp = NEXT_BLKP(bp);
    }
    return NULL;
}

static void place(void *bp, size_t aligned_size)
{
    size_t prev_size = GET_SIZE(HDRP(bp));
    size_t remainder_size = prev_size - aligned_size;

    if(remainder_size < 4 * WSIZE)
    {
        PUT(HDRP(bp), PACK(prev_size, 1));
        PUT(FTRP(bp), PACK(prev_size, 1));
    }
    else /* need splitting */
    {
        PUT(HDRP(bp), PACK(aligned_size, 1));
        PUT(FTRP(bp), PACK(aligned_size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remainder_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remainder_size, 0));
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
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
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














