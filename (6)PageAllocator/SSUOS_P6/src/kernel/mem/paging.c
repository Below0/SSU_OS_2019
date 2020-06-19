#include <device/io.h>
#include <mem/mm.h>
#include <mem/paging.h>
#include <device/console.h>
#include <proc/proc.h>
#include <interrupt.h>
#include <mem/palloc.h>
#include <ssulib.h>
#include<device/ide.h>

uint32_t *PID0_PAGE_DIR;//첫 생성되는 PID

intr_handler_func pf_handler;

//모든 함수는 수정이 가능가능

uint32_t scale_up(uint32_t base, uint32_t size)
{
	uint32_t mask = ~(size-1);
	//printk("%x\n",(uint32_t *)base);
	if(base & mask != 0)
		base = base & mask + size;
	//	printk("up : %x\n",(uint32_t *)base);
	return base;
}
//해당 코드를 사용하지 않고 구현해도 무관함
 void pagememcpy(void* from, void* to, uint32_t len)
 {
 	uint32_t *p1 = (uint32_t*)from;
 	uint32_t *p2 = (uint32_t*)to;
 	int i, e;

 	e = len/sizeof(uint32_t);//len을 통해 반복 횟수를 정함
 	for(i = 0; i<e; i++)
 		p2[i] = p1[i];//값을 복사

 	e = len%sizeof(uint32_t); //나머지 값들
 	if( e != 0)
 	{
 		uint8_t *p3 = (uint8_t*)p1;
 		uint8_t *p4 = (uint8_t*)p2;

 		for(i = 0; i<e; i++)
 			p4[i] = p3[i]; // 1바이트 단위 복사
 	}
 }

uint32_t scale_down(uint32_t base, uint32_t size)
{
	uint32_t mask = ~(size-1);
	//printk("mask : %x\n",base & mask);
	if(base & mask != 0)
		base = base & mask - size;
	//	printk("down %x\n",base);
	return base;
}

void init_paging()
{
	uint32_t *page_dir = palloc_get_one_page(kernel_area);
	uint32_t *page_tbl = palloc_get_one_page(kernel_area);// dir,tbl에 페이지 할당
	PID0_PAGE_DIR = page_dir; // PID0을 위한 페이지 디렉토리 설정

	int NUM_PT, NUM_PE;
	uint32_t cr0_paging_set;
	int i;


	NUM_PT = NUM_PE / 1024; // 페이지 테이블의 크기

	//페이지 디렉토리 구성
	page_dir[0] = (uint32_t)page_tbl | PAGE_FLAG_RW | PAGE_FLAG_PRESENT;
//dir0번 항목이 tbl을 가리키게 설정
	NUM_PE = RKERNEL_HEAP_START / PAGE_SIZE;//페이지 수를 구함
	//페이지 테이블 구성
	for ( i = 0; i < NUM_PE; i++ ) {
		page_tbl[i] = (PAGE_SIZE * i)
			| PAGE_FLAG_RW
			| PAGE_FLAG_PRESENT;
		//writable & present
	} // 페이지 테이블 항목이 페이지 프레임을 가리키게 설정

	//CR0레지스터 설정
	cr0_paging_set = read_cr0() | CR0_FLAG_PG;		// PG bit set

	//컨트롤 레지스터 저장
	write_cr3( (unsigned)page_dir );		// cr3 레지스터에 PDE 시작주소 저장
	write_cr0( cr0_paging_set );          // PG bit를 설정한 값을 cr0 레지스터에 저장
//Paging 기능 enable
	reg_handler(14, pf_handler); // 핸들러 등록
}

void memcpy_hard(void* from, void* to, uint32_t len)
{
	write_cr0( read_cr0() & ~CR0_FLAG_PG);
	memcpy(from, to, len);
	write_cr0( read_cr0() | CR0_FLAG_PG);
}

uint32_t* get_cur_pd()
{
	return (uint32_t*)read_cr3();
}

uint32_t pde_idx_addr(uint32_t* addr)
{
	uint32_t ret = ((uint32_t)addr & PAGE_MASK_PDE) >> PAGE_OFFSET_PDE;
	return ret;
}

uint32_t pte_idx_addr(uint32_t* addr)
{
	uint32_t ret = ((uint32_t)addr & PAGE_MASK_PTE) >> PAGE_OFFSET_PTE;
	return ret;
}
//page directory에서 index 위치의 page table 얻기
uint32_t* pt_pde(uint32_t pde)
{
	uint32_t * ret = (uint32_t*)(pde & PAGE_MASK_BASE);
	return ret;
}
//address에서 page table 얻기
uint32_t* pt_addr(uint32_t* addr)
{
	uint32_t idx = pde_idx_addr(addr);
	uint32_t* pd = get_cur_pd();
	return pt_pde(pd[idx]);
}
//address에서 page 주소 얻기
uint32_t* pg_addr(uint32_t* addr)
{
	uint32_t *pt = pt_addr(addr);
	uint32_t idx = pte_idx_addr(addr);
	uint32_t *ret = (uint32_t*)(pt[idx] & PAGE_MASK_BASE);
	return ret;
}

/*
    page table 복사
*/
void pt_copy(uint32_t *pd, uint32_t * dest_pd, uint32_t idx, uint32_t *start, uint32_t * end, bool share)
//void  pt_copy(uint32_t *pd, uint32_t *dest_pd, uint32_t idx)
{
	uint32_t *pt = pt_pde(pd[idx]); // pd[idx]가 가리키는 pt 리턴
	uint32_t *new_pt = palloc_get_one_page(kernel_area); // 새 pt위한 페이지 할당
	uint32_t i;
	uint32_t *temp; // pte가 가리킬 새 pf

	dest_pd[idx] = (uint32_t)new_pt| (pd[idx] & ~PAGE_MASK_BASE & ~    PAGE_FLAG_ACCESS); // 새 pd의 idx 항목이 new_pt 가리킴

    for(i = 0; i < 1024; i++) // 모든 pte 탐색
    {
      	if(pt[i] & PAGE_FLAG_PRESENT) // PRESENT 할 경우
    	{
            //new_pt = VH_TO_RH(new_pt);
						if(share == false){
							temp = palloc_get_one_page(kernel_area); //새 pt위한 새 공간 할당
							pagememcpy((void*)(pt[i] & PAGE_MASK_BASE), (void*)temp, PAGE_SIZE); // pf 복사
            new_pt[i] = (uint32_t)temp | (pt[i] & ~PAGE_MASK_BASE & ~    PAGE_FLAG_ACCESS); //새 pt의 항목이 temp를 가리킴

            //new_pt = RH_TO_VH(new_pt);
					}
					else // true일 경우
            new_pt[i] = pt[i]; // 같은 pf 가리킴
        }
    }
	}


/*
    page directory 복사.
    커널 영역 복사나 fork에서 사용
*/

void pd_copy(uint32_t *pd, uint32_t * dest_pd, uint32_t idx, uint32_t *start, uint32_t * end, bool share)
{
	uint32_t i, j ,k;
	j = pde_idx_addr(start); k= pde_idx_addr(end);// start,end에 해당하는 pde의 index 구함
	for(i = j; i < k; i++) // pd의 start idx부터 end idx까지만 탐색
	{
		if(pd[i] & PAGE_FLAG_PRESENT) // 존재시:
			pt_copy(pd, dest_pd, i,start,end,share);
	}
//}
}

uint32_t* pd_create (pid_t pid)
{
	uint32_t *pd = palloc_get_one_page(kernel_area); // 새 디렉토리 위한 페이지 할당
	pd_copy(PID0_PAGE_DIR, pd, 0,0,(uint32_t *)RKERNEL_HEAP_START,true);
	return pd;
}
void pf_handler(struct intr_frame *iframe)
{
	void *fault_addr;
	asm ("movl %%cr2, %0" : "=r" (fault_addr)); // page fault addr is saved in cr2 register

	printk("page fault : %X\n",fault_addr);

#ifdef SCREEN_SCROLL
	refreshScreen();
#endif
	uint32_t pdi, pti;
    uint32_t *pta;
    uint32_t *pda = (uint32_t*)read_cr3();
    pdi = pde_idx_addr(fault_addr);
    pti = pte_idx_addr(fault_addr);

    if(pda == PID0_PAGE_DIR){ // pda = pd of pid0

        write_cr0( read_cr0() & ~CR0_FLAG_PG); // paging disable

        pta = pt_pde(pda[pdi]);
        write_cr0( read_cr0() | CR0_FLAG_PG); // paging enable

    }
    else{

        pta = pt_pde(pda[pdi]);
    }

    if(pta == NULL){
        write_cr0( read_cr0() & ~CR0_FLAG_PG);

        pta = palloc_get_one_page(kernel_area);
        memset(pta,0,PAGE_SIZE);

        pda[pdi] = (uint32_t)pta | PAGE_FLAG_RW | PAGE_FLAG_PRESENT;

        pta[pti] = (uint32_t)fault_addr | PAGE_FLAG_RW  | PAGE_FLAG_PRESENT;

        pdi = pde_idx_addr(pta);
        pti = pte_idx_addr(pta);

        uint32_t *tmp_pta = pt_pde(pda[pdi]);
        tmp_pta[pti] = (uint32_t)pta | PAGE_FLAG_RW | PAGE_FLAG_PRESENT;

        write_cr0( read_cr0() | CR0_FLAG_PG);
    }
    else{
        pta[pti] = (uint32_t)fault_addr | PAGE_FLAG_RW  | PAGE_FLAG_PRESENT;
    }
}
