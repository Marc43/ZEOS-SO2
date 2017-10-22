/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}

extern struct list_head blocked;

struct list_head freequeue; 	//We put there the structs just for code style
								//As they are only used in this .c we don't put the extern
struct list_head readyqueue;	//Before the type, i.e extern struct list_head blocked;

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void init_idle (void) {
	//Here we initialize the idle process
	
	if (!list_empty(&freequeue)) { //Free processes available
		struct list_head * lh = list_first (&freequeue);
		list_del(lh);	
		
		struct task_struct* idle = list_head_to_task_struct(lh);
		//Now the 'idle' points to our free PCB
		idle->PID = 0;
		idle->dir_pages_baseAddr = allocate_DIR(idle);
		//Now the process has the PID 0, and a number of Page Directory assigned
		//idle->kernel_esp = ; //Which initial value?
		struct task_struct*	idle_task = idle; //struct task_struct* idle_task = idle;
	}
	//else (Doesn't make sense, there will be free PCB's always...)
}

void init_task1(void) {
	struct list_head* lh = list_first (&freequeue);
	list_del(lh); 	

	struct task_struct* task1 = list_head_to_task_struct(lh);
	task1->PID = 1;
	task1->dir_pages_baseAddr = allocate_DIR(task1);
	set_user_pages(task1); //Initialize pages for task1
	set_cr3(task1->dir_pages_baseAddr);
	tss.esp0 = task[task1->PID].stack[1023];	
	task1->kernel_esp = tss.esp0; //At the start, they point to the same memory position
}


void init_sched(){

}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

/* Free && Ready queues functions */

void init_free_queue () {
	INIT_LIST_HEAD ( &freequeue );
	
	int i;
	for (i=0; i < NR_TASKS; ++i) list_add_tail (&(task[i].task.list), &freequeue);
		
}

void init_ready_queue () {
	//Empty at the beggining
	INIT_LIST_HEAD ( &readyqueue );
}

void task_switch (union task_union* t) {
	__asm__ __volatile__ ( "pushl %%esi;"
						   "pushl %%edi;"
						   "pushl %%ebx;"
						   "movl %0, %%ebx;"
						   :: "m" (t)
						   : "%eax", "%ebx");
	inner_task_switch(t);
	__asm__ __volatile__ (	"popl %ebx;"
						    "popl %edi;"
							"popl %esi;");

}

void inner_task_switch (union task_union* t) {
	tss.esp0 = task[t->task.PID].stack[1023]; //Update the TSS...
	set_cr3 (t->task.dir_pages_baseAddr);
	unsigned int new_esp = t->task.kernel_esp;
	__asm__ __volatile__ (  "pushl %%ebp;"
						    "movl %0, %%esp;"
							"popl %%ebp;"
							:: "m" (new_esp)
							:);
	//RET call missing... But... something is weird

}
