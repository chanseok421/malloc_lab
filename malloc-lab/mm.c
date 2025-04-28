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

 //바꿈


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
 
 #define WSIZE 4 //기본 단위 워드 크기 (8바이트)
 #define DSIZE 8 // 더블 워드 크기 (16바이트)
 #define CHUNKSIZE (1<<12) //기본적으로 힙을 확장할 때 요청하는 크기 (4096바이트 = 4kb)
 
 #define MAX(x,y) ((x) > (y) ? (x) : (y)) //두 값 중 더 큰 값을 반환하는 매크로 
 
 #define PACK(size, alloc) ((size) | (alloc)) //주어진 size와 할당 여부 (alloc)를 하나의 값으로 패킹
 
 
 #define GET(p) (*(unsigned int *)(p))
 #define PUT(p, val) (*(unsigned int *)(p) = (val))
 
 
 #define GET_SIZE(p)  (GET(p) & ~0x7) //포인터 p가 가리키는 곳에서 읽은 값에서 사이즈만 추출 (하위 3비트 제거)
 #define GET_ALLOC(p) (GET(p) & 0x1) //포틴터 p가 가리키는 곳에서 읽은 값에서 할당 여부만 추출 (맨 마지막 비트)
 
 #define HDRP(bp) ((char *)(bp) - WSIZE) //블록 포인터(bp)를 이용해 헤더 포인터를 반환 (헤더는 블록 시작보다 4바이트 앞에 위치)
 #define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //블록 포인터(bp)를 이용해 풋터 포인터를 반환 (블록 끝 부분의 위치 계산)
 
 
 #define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //다음 블록의 블록 포인터를 반환
 #define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //이전 블록의 블록 포인터를 반환
 
 
 
 
 
 static char *heap_listp = NULL; //전역변수 선언
 
 static void *extend_heap(size_t );
 static void *coalesce(void *);
 static void *find_fit(size_t );
 static void place(void *, size_t);
 
 
 /*
  * mm_init - malloc 패키지를 초기화하는 함수.
  * 힙 영역을 초기화하고, 필요한 초기 블록을 설정한다.
  */
 int mm_init(void)
 {
     /* 초기 빈 힙 생성 */
     if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
         return -1;
     
     PUT(heap_listp, 0); /* 정렬을 위한 패딩 */
     PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* 프로로그 헤더 */
     PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* 프로로그 풋터 */
     PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* 에필로그 헤더 */
     
     heap_listp += (2*WSIZE);
 
     /* CHUNKSIZE만큼 빈 블록을 추가하여 힙 확장 */
     if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
         return -1;
     
     return 0;
 }
 
 static void *extend_heap(size_t words) //새 가용 블록으로 힙 확장하기
 {
     char *bp;
     size_t size;
 
     /* 짝수 개수의 워드를 할당하여 정렬을 유지 */
     size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
     if ((long)(bp = mem_sbrk(size)) == -1)
         return NULL; // 힙 확장 실패 시 NULL 반환
 
     /* 새로 확장된 영역에 free block 헤더/풋터와 새로운 에필로그 헤더를 초기화 */
     PUT(HDRP(bp), PACK(size, 0)); /* 새로 생성된 free block의 헤더 */
     PUT(FTRP(bp), PACK(size, 0)); /* 새로 생성된 free block의 풋터 */
     PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* 새 에필로그 헤더 */
 
     /* 이전 블록이 free였다면 병합(coalesce) */
     return coalesce(bp);
 }
 
 void mm_free(void *bp)
 {
     size_t size = GET_SIZE(HDRP(bp));
 
     PUT(HDRP(bp), PACK(size, 0));
     PUT(FTRP(bp), PACK(size, 0));
     coalesce(bp);
 
 }
 
 /*
  * coalesce - 주변 블록과 병합해서 큰 free 블록을 만든다.
  */
 static void *coalesce(void *bp)
 {
     size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 이전 블록의 할당 여부
     size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 다음 블록의 할당 여부
     size_t size = GET_SIZE(HDRP(bp)); // 현재 블록의 크기
 
     if (prev_alloc && next_alloc) { /* Case 1: 앞/뒤 모두 할당됨 */
         return bp;
     }
 
     else if (prev_alloc && !next_alloc) { /* Case 2: 앞은 할당, 뒤는 free */
         size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 다음 블록 크기 합치기
         PUT(HDRP(bp), PACK(size, 0)); // 새로운 헤더
         PUT(FTRP(bp), PACK(size, 0)); // 새로운 풋터
     }
 
     else if (!prev_alloc && next_alloc) { /* Case 3: 앞은 free, 뒤는 할당 */
         size += GET_SIZE(HDRP(PREV_BLKP(bp))); // 이전 블록 크기 합치기
         PUT(FTRP(bp), PACK(size, 0)); // 새로운 풋터
         PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 이전 블록의 헤더 갱신
         bp = PREV_BLKP(bp); // bp를 이전 블록으로 이동
     }
 
     else { /* Case 4: 앞/뒤 모두 free */
         size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                 GET_SIZE(FTRP(NEXT_BLKP(bp))); // 앞, 현재, 뒤 블록 크기 모두 합침
         PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 새로운 헤더
         PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); // 새로운 풋터
         bp = PREV_BLKP(bp); // bp를 병합된 블록의 시작으로 이동
     }
     return bp; // 병합된 블록 반환
 }
 
 
 /*
  * mm_malloc - brk 포인터를 증가시켜 메모리 블록을 할당합니다.
  *             항상 정렬을 보장하기 위해 블록 크기를 정렬 단위(ALIGNMENT)의 배수로 맞춥니다.
  */
 
 void *mm_malloc(size_t size)
 {
     // int newsize = ALIGN(size + SIZE_T_SIZE);   // 정렬 맞추기
     // void *p = mem_sbrk(newsize);               // 힙 확장 요청
     // if (p == (void *)-1)                       // 실패 시 NULL 반환
     //     return NULL;
     // else
     // {
     //     *(size_t *)p = size;                   // 블록 크기 저장
     //     return (void *)((char *)p + SIZE_T_SIZE); // 실제 데이터 영역 리턴
     // }
     size_t asize;
     size_t extendsize;
     char *bp;
 
     if (size == 0)
         return NULL;
 
     if (size <= DSIZE)
         asize = 2 * DSIZE;
     else
         asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
 
     if ((bp = find_fit(asize)) != NULL) {
         place(bp, asize);
         return bp;
     }
 
     extendsize = MAX(asize, CHUNKSIZE);
     if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
         return NULL;
     place(bp, asize);
     return bp;
 
 }
 
 
 // /*
 //  * mm_free - Freeing a block does nothing.
 //  */
 // void mm_free(void *ptr)
 // {
 // }
 
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
 
 
 /*
  * find_fit - 첫 번째로 맞는(free) 블록을 찾는다 (First-fit 전략)
  */
 static void *find_fit(size_t asize)
 {
     void *bp;
 
     // 1) 힙 시작부터 블록 크기 > 0 (에필로그 전)까지 순회
     for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
         // 2) 자유 블록인지 & 충분한 크기인지 검사 
         if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
             return bp;  // 첫 번째 적합 블록 반환
         }
     }
 
     // 3) 적합 블록 없음 
     return NULL;
 }
 
 
 // static void *find_fit(size_t asize) {
 //     void *bp = heap_listp;
 //     void *best_bp = NULL;
 //     size_t best_size = (size_t)-1;        // SIZE_MAX 
 
 //     for (; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
 //         if (!GET_ALLOC(HDRP(bp))) {
 //             size_t blk_size = GET_SIZE(HDRP(bp));
 //             if (blk_size >= asize && blk_size < best_size) {
 //                 best_size = blk_size;
 //                 best_bp   = bp;
 //                 if (best_size == asize)
 //                     break;                //Perfect fit
 //             }
 //         }
 //     }
 
 //     return best_bp;                       // NULL 가능
 // }
 
 
 
 /*
  * place - 요청한 크기의 블록을 free 블록에 배치
  */
 static void place(void *bp, size_t asize)
 {
     size_t csize = GET_SIZE(HDRP(bp)); // 현재 블록 크기Q
 
     if ((csize - asize) >= (2 * DSIZE)) { // 남은 공간이 충분히 크면
         PUT(HDRP(bp), PACK(asize, 1));    // 앞부분만 할당하고
         PUT(FTRP(bp), PACK(asize, 1));
         bp = NEXT_BLKP(bp);               // 나머지를 새로운 free 블록으로 설정
         PUT(HDRP(bp), PACK(csize-asize, 0));
         PUT(FTRP(bp), PACK(csize-asize, 0));
     } else { // 남은 공간이 적으면 그냥 통째로 할당
         PUT(HDRP(bp), PACK(csize, 1));
         PUT(FTRP(bp), PACK(csize, 1));
     }
 }
 
 
 
 
 
 
 