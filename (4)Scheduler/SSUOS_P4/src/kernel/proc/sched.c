#include <list.h>
#include <proc/sched.h>
#include <mem/malloc.h>
#include <proc/proc.h>
#include <proc/switch.h>
#include <interrupt.h>

extern struct list plist;
extern struct list rlist;
extern struct list runq[RQ_NQS];

extern struct process procs[PROC_NUM_MAX];
extern struct process *idle_process;
struct process *latest;

bool more_prio(const struct list_elem *a, const struct list_elem *b,void *aux);
int scheduling; 					// interrupt.c

struct process* get_next_proc(void) // 다음 프로세스 탐색
{
		bool found = false;
		struct process *next = NULL;
		struct list_elem *elem;
		int i;
		/* 
		   You shoud modify this function...
		   Browse the 'runq' array 
		 */

		for(i = 0; i < RQ_NQS; i++){ // runq index만큼 탐색
				if(!list_empty(&runq[i])){// 해당 runq가 안비었을 경우
						for(elem = list_begin(&runq[i]); elem != list_end(&runq[i]); elem = list_next(elem))
						{
								next = list_entry(elem, struct process, elem_stat);
								if(next->state == PROC_RUN){//프로세스가 실행가능할 경우만 
								return next;
												
								}
						}

				}
		}
		return next;
}

void schedule(void)
{	
//		intr_disable();//switching + 로그 출력 시 tick증가를 막기 위해
		struct process *cur;
		struct process *next;
		/* You shoud modify this function.... */
		if(cur_process->pid == 0){// idle일 경우 만 다음 프로세스를 찾음
				int check = 0;
				struct list_elem *temp;
				struct process *p;
				proc_wake();
				for(temp = list_begin(&plist)->next; temp != list_end(&plist); temp = list_next(temp)){

						p = list_entry(temp, struct process, elem_all);
						if(p->state == PROC_RUN){
								if(check > 0) printk(", ");
								printk("#=%2d p= %2d c=%2d u= %3d",p->pid,p->priority,p->time_slice,p->time_used);
								check++;
						}

				}
				if(check > 0) printk("\n");
				next = get_next_proc();
				if(next->pid != 0)
				printk("Selected # = %d\n",next->pid);
		}
		else next = idle_process;
		
		cur = cur_process;
		cur_process = next;
		cur_process->time_slice = 0;
		intr_disable();
		switch_process(cur, next);
		intr_enable();
		scheduling = 0;
}
