#include <mem/palloc.h>
#include <bitmap.h>
#include <type.h>
#include <round.h>
#include <mem/mm.h>
#include <synch.h>
#include <device/console.h>
#include <mem/paging.h>
#include<mem/swap.h>

/* Page allocator.  Hands out memory in page-size (or
   page-multiple) chunks.
   */
/* pool for memory */
struct memory_pool
{
	struct lock lock;
	struct bitmap *bitmap;
	uint32_t *addr;
};
/* kernel heap page struct */
struct khpage{
	uint16_t page_type;
	uint16_t nalloc;
	uint32_t used_bit[4];
	struct khpage *next;
};

/* free list */
struct freelist{
	struct khpage *list;
	int nfree;
};

static struct khpage *khpage_list;
static struct freelist freelist;
struct memory_pool memory_pool[2];
static uint32_t page_alloc_index;
size_t user_page_idx;
size_t kernel_page_idx;

/* Initializes the page allocator. */
//
	void
init_palloc (void)
{
	/* Calculate the space needed for the khpage list */
	size_t bm_size = sizeof(struct khpage) * 1024;

	size_t kernel_bytes, user_bytes;

	kernel_bytes = (USER_POOL_START-KERNEL_ADDR)/PAGE_SIZE;
	user_bytes = (RKERNEL_HEAP_START - USER_POOL_START)/PAGE_SIZE;

	memory_pool[kernel_area].bitmap = create_bitmap(kernel_bytes/8,(void*)KERNEL_ADDR, 0);//비트맵을 커널 풀에 설정
	memory_pool[user_area].bitmap = create_bitmap(user_bytes/8,(void*)USER_POOL_START, 0);//비트맵을 유저 풀에 설정


	/* initialize */
	for(int i = 0; i < 2; i++) set_bitmap(memory_pool[i].bitmap, 0, true); // 첫 페이지를 할당
		memory_pool[kernel_area].addr = (uint32_t *)KERNEL_ADDR;
		memory_pool[user_area].addr = (uint32_t * )USER_POOL_START;



}



/* Obtains and returns a group of PAGE_CNT contiguous free pages.
   */
	uint32_t *
palloc_get_multiple_page (enum palloc_flags flags ,size_t page_cnt)
{
	void *pages = NULL;
	size_t page_idx;
		uint32_t *start_address;
	struct bitmap* mode_bitmap;

	start_address = memory_pool[flags].addr; // 시작 주소
	if(flags == user_area){ // flag에 따라 맞는 비트맵을 포인팅
		mode_bitmap = memory_pool[user_area].bitmap;
	}
	else{
		mode_bitmap = memory_pool[kernel_area].bitmap;
	}


	if (page_cnt == 0)
		return NULL;

	if((page_idx = find_set_bitmap(mode_bitmap,0,page_cnt,false)) == BITMAP_ERROR) // 페이지 idx를 찾는다
	return NULL;//못찾을 시 종료
	pages = (void*)start_address + page_idx * PAGE_SIZE; // 해당 idx에맞는 페이지할당

//printk("get %x\n",(uint32_t*)pages);

		memset (pages, 0, PAGE_SIZE * page_cnt); // 페이지  초기화

	return (uint32_t*)pages;
}

/* Obtains a single free page and returns its address.
   */
	uint32_t *
palloc_get_one_page (enum palloc_flags flags)
{
	return palloc_get_multiple_page (flags,1);
}

/* Frees the PAGE_CNT pages starting at PAGES. */
	void
palloc_free_multiple_page (void *pages, size_t page_cnt)
{
	size_t page_idx = 0;
	struct bitmap* mode_bitmap;
	if(pages > USER_POOL_START){ //pages 주소를 통해 유저인지 커널영역인지 판별
		page_idx = (((uint32_t)pages - USER_POOL_START) / PAGE_SIZE);
		mode_bitmap = memory_pool[user_area].bitmap;
	}
	else if(pages > KERNEL_ADDR){
	page_idx = (((uint32_t)pages - KERNEL_ADDR) / PAGE_SIZE);
	mode_bitmap = memory_pool[kernel_area].bitmap;
}
	if (pages == NULL || page_cnt == 0)
		return;

	
	set_multi_bitmap(mode_bitmap, page_idx, page_cnt,false); // 해당 idx에 맞는 비트맵을 cnt만큼  다시 false로 설정


}

/* Frees the page at PAGE. */

	void
palloc_free_one_page (void *page)
{
	palloc_free_multiple_page (page, 1);
}


void palloc_pf_test(void)
{
	 uint32_t *one_page1 = palloc_get_one_page(user_area);
	 uint32_t *one_page2 = palloc_get_one_page(user_area);
	 uint32_t *two_page1 = palloc_get_multiple_page(user_area,2);
	 uint32_t *three_page;
	 printk("one_page1 = %x\n", one_page1);
	 printk("one_page2 = %x\n", one_page2);
	 printk("two_page1 = %x\n", two_page1);
	 printk("=----------------------------------=\n");
	 palloc_free_one_page(one_page1);
	 palloc_free_one_page(one_page2);
	 palloc_free_multiple_page(two_page1,2);

	 one_page1 = palloc_get_one_page(user_area);
	 one_page2 = palloc_get_one_page(user_area);
	 two_page1 = palloc_get_multiple_page(user_area,2);

	 printk("one_page1 = %x\n", one_page1);
	 printk("one_page2 = %x\n", one_page2);
	 printk("two_page1 = %x\n", two_page1);

	 printk("=----------------------------------=\n");
	 palloc_free_multiple_page(one_page2, 3);
	 one_page2 = palloc_get_one_page(user_area);
	 three_page = palloc_get_multiple_page(user_area,3);

	 printk("one_page1 = %x\n", one_page1);
	 printk("one_page2 = %x\n", one_page2);
	 printk("three_page = %x\n", three_page);

	 palloc_free_one_page(one_page1);
	 palloc_free_one_page(three_page);
	 three_page = (uint32_t*)((uint32_t)three_page + 0x1000);
	 palloc_free_one_page(three_page);
	 three_page = (uint32_t*)((uint32_t)three_page + 0x1000);
	 palloc_free_one_page(three_page);
	 palloc_free_one_page(one_page2);
}
