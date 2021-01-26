/*
 * mm-explicit.c - an empty malloc package
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 * @id : 201701977 
 * @name : 권유나
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define SIZE_PTR(p) ((size_t*)(((char*)(p)) - SIZE_T_SIZE))

#define HDRSIZE 4
#define FTRSIZE 4
#define WSIZE	4
#define DSIZE	8
#define CHUNKSIZE	(1<<12)
#define OVERHEAD	8

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define PACK(size, alloc) ((unsigned) ((size) | (alloc)))

#define GET(p)		(*(unsigned int*) (p))
#define PUT(p,val)	(*(unsigned int*) (p) = (unsigned)(val))
#define GET8(p)		(*(unsigned int*)(p))
#define PUT8(p,val)	(*(unsigned int*)(p) = (unsigned int)(val))

#define GET_SIZE(p)		(GET(p)&~0x7)
#define GET_ALLOC(p)	(GET(p)&0x1)

#define HDRP(bp)		((char *)(bp) - WSIZE)
#define FTRP(bp)		((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

#define NEXT_FREEP(bp) ((char *)(bp))
#define PREV_FREEP(bp) ((char *)(bp) + WSIZE)

#define NEXT_FREE_BLKP(bp) ((char *)GET8((char *)(bp)))
#define PREV_FREE_BLKP(bp) ((char *)GET8((char *)(bp) + WSIZE))

static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);

static char* heap_start;
static char* h_ptr;
static char* epilogue;
/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
	if((h_ptr = mem_sbrk(DSIZE + 4 * HDRSIZE)) == NULL)
		return -1;
	heap_start = h_ptr;

	PUT(h_ptr, NULL);
	PUT(h_ptr + WSIZE, NULL);
	PUT(h_ptr + DSIZE, 0);
	PUT(h_ptr + DSIZE + HDRSIZE, PACK(OVERHEAD,1));
	PUT(h_ptr + DSIZE + HDRSIZE + FTRSIZE,PACK(OVERHEAD,1));
	PUT(h_ptr + DSIZE + 2 * HDRSIZE + FTRSIZE,PACK(0,1));

	h_ptr = h_ptr + DSIZE + DSIZE;
	epilogue = h_ptr + HDRSIZE;

	if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;

    return 0;
}

static void *coalesce(void *bp){
	
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	void *prevpo = NULL;
	void *nextpo = NULL;

	if(prev_alloc && next_alloc){}
	else if(!prev_alloc && next_alloc){
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		
		prevpo = PREV_BLKP(bp);
		
		PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
		PUT(FTRP(PREV_BLKP(bp)),PACK(size,0));
		bp = PREV_BLKP(bp);
	}
	else if(prev_alloc && !next_alloc){
		size += GET_SIZE(FTRP(NEXT_BLKP(bp)));

		nextpo = NEXT_BLKP(bp);

		PUT(HDRP(bp),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));
	}
	else{
		size+=GET_SIZE(FTRP(NEXT_BLKP(bp))) +GET_SIZE(HDRP(PREV_BLKP(bp)));

		prevpo = PREV_BLKP(bp);
		nextpo = NEXT_BLKP(bp);

		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		PUT(FTRP(PREV_BLKP(bp)),PACK(size,0));
		bp = PREV_BLKP(bp);
	}

	if(prevpo != NULL){
		if(PREV_FREE_BLKP(prevpo) !=NULL)
			PUT8(NEXT_FREEP(PREV_FREE_BLKP(prevpo)),NEXT_FREE_BLKP(prevpo));
		else PUT8(NEXT_FREEP(heap_start), NEXT_FREE_BLKP(prevpo));
		if(NEXT_FREE_BLKP(prevpo) !=NULL)
			PUT8(PREV_FREEP(NEXT_FREE_BLKP(prevpo)),PREV_FREE_BLKP(prevpo));
	}
	if(nextpo != NULL){
		if(PREV_FREE_BLKP(nextpo) !=NULL)
			PUT8(NEXT_FREEP(PREV_FREE_BLKP(nextpo)),NEXT_FREE_BLKP(nextpo));
		else PUT8(NEXT_FREEP(heap_start), NEXT_FREE_BLKP(nextpo));
		if(NEXT_FREE_BLKP(nextpo) != NULL)
			PUT8(PREV_FREEP(NEXT_FREE_BLKP(nextpo)),PREV_FREE_BLKP(nextpo));
	}

	PUT8(NEXT_FREEP(bp),NEXT_FREE_BLKP(heap_start));

	if(GET(heap_start)){
		PUT8(PREV_FREEP(NEXT_FREE_BLKP(heap_start)),bp);
	}
	PUT(PREV_FREEP(bp),NULL);
	PUT(heap_start,bp);

	return bp;

}

static void place(void *bp, size_t asize){

	size_t fsize = GET_SIZE(HDRP(bp));
	void *po;

	if((fsize - asize) >= 2*DSIZE){
		PUT(HDRP(bp), PACK(asize,1));
		PUT(FTRP(bp), PACK(asize,1));

		po = bp;

		bp=NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(fsize-asize,0));
		PUT(FTRP(bp), PACK(fsize-asize,0));
	
		coalesce(bp);
	}
	else{
		PUT(HDRP(bp),PACK(fsize,1));
		PUT(FTRP(bp), PACK(fsize,1));

		po = bp;
	}

	if(PREV_FREE_BLKP(po) !=NULL)
		PUT8(NEXT_FREEP(PREV_FREE_BLKP(po)), NEXT_FREE_BLKP(po));
	else PUT8(NEXT_FREEP(heap_start),NEXT_FREE_BLKP(po));
	if(NEXT_FREE_BLKP(po) !=NULL)
		PUT8(PREV_FREEP(NEXT_FREE_BLKP(po)),PREV_FREE_BLKP(po));

}

static void *extend_heap(size_t words){

//	char *old_epilogue;
	char *bp;
	unsigned size;

	size = (words % 2) ? (words + 1)*WSIZE : words*WSIZE;
	
	if((long)(bp = mem_sbrk(size)) < 0)
		return NULL;

//	old_epilogue = epilogue;
	epilogue = bp + size - HDRSIZE;
	
	PUT(HDRP(bp), PACK(size,0));
	PUT(FTRP(bp), PACK(size,0));
	PUT(epilogue, PACK(0,1));

	return coalesce(bp);
}

static void *find_fit(size_t asize){
	void *bp;

	bp = NEXT_FREE_BLKP(heap_start);

	while( bp != NULL ){
		if(asize <=GET_SIZE(HDRP(bp))) return bp;

		bp = NEXT_FREE_BLKP(bp);
	}
	return NULL;
}

/*
 * malloc
 */
void *malloc (size_t size) {
	unsigned asize;
	unsigned extendsize;
	char *bp;

	if(size <= 0) return NULL;

	if(size <= DSIZE) asize = DSIZE * 2;
	else asize = DSIZE * ((size + DSIZE+ (DSIZE -1))/ DSIZE);

	if((bp = find_fit(asize)) != NULL){
		place(bp, asize);
		return bp;
	}

	extendsize = MAX(asize,CHUNKSIZE);

	if((bp = extend_heap(extendsize / WSIZE)) == NULL) return NULL;

	place(bp,asize);
	return bp;
//	return NULL;
}

/*
 * free
 */
void free (void *ptr) {
    if(!ptr) return;

	size_t size = GET_SIZE(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size,0));
	PUT(FTRP(ptr), PACK(size,0));

	coalesce(ptr);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
	void *newptr;

	if(size==0){
		free(oldptr);
		return 0;
	}

	if(oldptr == NULL){
		return malloc(size);
	}
	newptr = malloc(size);
	if(!newptr){
		return 0;
	}
	oldsize = *SIZE_PTR(oldptr);
	if(size < oldsize) oldsize = size;
	memcpy(newptr, oldptr, oldsize);
	
	free(oldptr);
	return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
	size_t bytes = nmemb*size;
	void *newptr;

	newptr = malloc(bytes);
	memset(newptr,0,bytes);

	return newptr;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p < mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int verbose) {
}
