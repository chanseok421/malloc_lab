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
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4 //기본 단위 워드 크기 (4바이트)
#define DSIZE 8 // 더블 워드 크기 (8바이트)
#define CHUNKSZIE (1<<12) //기본적으로 힙을 확장할 때 요청하는 크기 (4096바이트 = 4kb)

#define MAX (x,y) ((x) > (y) ? (x) : (y)) //두 값 중 더 큰 값을 반환하는 매크로 

#define PACK(size, alloc) ((size) | (alloc)) //주어진 size와 할당 여부 (alloc)를 하나의 값으로 패킹

#define GET_SIZE(p)  (GET(p) & ~0x7) //포인터 p가 가리키는 곳에서 읽은 값에서 사이즈만 추출 (하위 3비트 제거)
#define GET_ALLOC(p) (GET(p) & 0x1) //포틴터 p가 가리키는 곳에서 읽은 값에서 할당 여부만 추출 (맨 마지막 비트)

#define HDRP(bp) ((char *)(bp) -WSIZE) //블록 포인터(bp)를 이용해 헤더 포인터를 반환 (헤더는 블록 시작보다 4바이트 앞에 위치)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) -DSIZE) //블록 포인터(bp)를 이용해 풋터 포인터를 반환 (블록 끝 부분의 위치 계산)


#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) -WSIZE))) //다음 블록의 블록 포인터를 반환
#define PREV_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) -DSIZE))) //이전 블록의 블록 포인터를 반환

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)

{
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
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
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}