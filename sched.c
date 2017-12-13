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

unsigned long last_PID = 0;
unsigned int ticks_rr = QUANTUM;

struct info_dir dir_ [NR_TASKS];

union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */
struct task_struct*	idle_task ;

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}

extern struct list_head blocked;

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
	//Search the first free DIR
	int i = 0; int found = 0;
	while (!found  && i < NR_TASKS) { 
		if (dir_ [i].valid == 0) found = 1;	
		++i;
	}
	if (i >= NR_TASKS) return -1; 

	dir_ [i].valid = 1;
	dir_ [i].num_of = 1;

	t->info_dir_ = &(dir_ [i]);	
	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[i]; 

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
	
	if (!list_empty(&freequeue)) { //Free processes available
		struct list_head* lh = list_first (&freequeue);
		list_del(lh);	
		
		struct task_struct* idle = list_head_to_task_struct(lh);
		//Now the 'idle' points to our free PCB
		idle->PID = last_PID++;
		allocate_DIR(idle);
		union task_union* idle_union = (union task_union*)idle;
		//Now the process has the PID 0, and a number of Page Directory assigned
		idle_task = idle; //struct task_struct* idle_task = idle;
		idle_union->stack[KERNEL_STACK_SIZE-1] = (unsigned long)&cpu_idle;
		idle_union->stack[KERNEL_STACK_SIZE-2] = 0;
		idle->kernel_esp = &(idle_union->stack [KERNEL_STACK_SIZE - 2]);
		
		idle->quantum = QUANTUM;
		
		//Estadisticas del proceso Idle
		idle->state = ST_READY; //??? No proper state ???
		idle->stats.user_ticks = 0;
		idle->stats.system_ticks = 0;
	 	idle->stats.blocked_ticks = 0;
		idle->stats.ready_ticks = 0;
		idle->stats.total_trans = 0;
		idle->stats.remaining_ticks = 0;
		idle->stats.elapsed_total_ticks = get_ticks();
	}

}

void init_task1(void) {
	if (!list_empty(&freequeue)) {
		struct list_head* lh = list_first (&freequeue);
		list_del(lh); 	
	
		struct task_struct* task1 = list_head_to_task_struct(lh);
		task1->PID = last_PID++;
		allocate_DIR(task1);
		set_user_pages(task1); //Initialize pages for task1
		set_cr3(task1->dir_pages_baseAddr);
		
		union task_union* tu = (union task_union*)task1;
		tss.esp0 = &(tu->stack[KERNEL_STACK_SIZE]);
	
		task1->kernel_esp = tss.esp0; //At the start, they point to the same memory position 
		
		set_quantum(task1, QUANTUM);
		
		//Estadisticas del proceso task1
		task1->stats.user_ticks = 0;
		task1->stats.system_ticks = 0;
	 	task1->stats.blocked_ticks = 0;
		task1->stats.ready_ticks = 0;
		task1->stats.total_trans = 0;
		task1->stats.remaining_ticks = QUANTUM;
		task1->stats.elapsed_total_ticks = get_ticks();
		
		update_process_state_rr(task1, NULL);
	}
}

void init_sched(){
	init_free_queue();
	init_ready_queue();
	init_idle();
	init_task1();
	init_semaphores();
	init_dir_structure();
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
	__asm__ __volatile__ ( "pushl %esi;"
						   "pushl %edi;"
						   "pushl %ebx;");
	inner_task_switch(t);
	__asm__ __volatile__ (	"popl %ebx;"
						    "popl %edi;"
							"popl %esi;");

}

void inner_task_switch (union task_union* t) {	
	tss.esp0 = &(t->stack[KERNEL_STACK_SIZE]);
	ticks_rr = t->task.quantum;
	if (thread_of(current(), t) == -1) set_cr3 (t->task.dir_pages_baseAddr);
	__asm__ __volatile__ ( 	"movl %%ebp, %0;" 
						    "movl %1, %%esp;"
							"popl %%ebp;"
							"ret;"
							: "=m" (current()->kernel_esp)
							: "m" (t->task.kernel_esp)
							:);

}

void update_sched_data_rr () {
	current()->stats.remaining_ticks--;
	ticks_rr--; //I update the total of ticks executed by the current process
}

int needs_sched_rr () {
	//When we take all the quantum or the current process is the idle task and there is a process ready to be executed	
	if (ticks_rr == 0 || (current()->PID == 0 && !list_empty(&readyqueue))) return 1;
	else return 0;
}

void update_process_state_rr (struct task_struct *t, struct list_head *dst_queue) {
	struct list_head* lh = &(t->list);

	if (dst_queue == NULL) {
		t->state = ST_RUN;
	}
	else if (dst_queue == &(readyqueue)) {
		
		current()->stats.system_ticks += get_ticks()-current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();
		
		if (t->state == ST_BLOCKED) list_del(lh);
		
		t->state = ST_READY;
		list_add_tail(lh, dst_queue); //By the moment only ready	
	}
	else { //From run to blocked
		t->state = ST_BLOCKED;
		list_add_tail(lh, dst_queue);
	}	
}

void sched_next_rr () {
	if (!list_empty(&readyqueue)) {

		struct list_head* lh = list_first(&readyqueue);
		struct task_struct* new = list_head_to_task_struct(lh);
		list_del(lh); //Extract the process from the queue
	
		//Actualización de ESTADISTICAS del nuevo proceso (estadisticas READY -> RUN_system)
		new->stats.ready_ticks += get_ticks()-new->stats.elapsed_total_ticks;
		new->stats.elapsed_total_ticks = get_ticks();
		new->stats.remaining_ticks = get_quantum(new);
		new->stats.total_trans++;
		current()->stats.total_trans++;
		
		//Cambio de estado del nuevo proceso a ST_RUN
		update_process_state_rr(new, NULL);
	
		//Ponemos el nuevo proceso a ejecutar			
		task_switch((union task_union*) new);	
	}
	
	else task_switch((union task_union*) idle_task); //Modificar estadisticas eh!! TODO
}

int get_quantum (struct task_struct *t) {
	return t->quantum;
}

void set_quantum (struct task_struct *t, int new_quantum) {
	t->quantum = new_quantum; 
}

void schedule () {
	update_sched_data_rr();
	if (needs_sched_rr()) {
		update_process_state_rr(current(), &readyqueue);
		sched_next_rr();
	}
}

void init_semaphores() {
	int i;
	for (i = 0; i < NUM_SEMAPHORES; ++i) {
		sem_vector [i].owner_pid = -1;
		sem_vector [i].num_blocked = 0;
		sem_vector [i].max_blocked = 0;
		INIT_LIST_HEAD(&(sem_vector[i].blocked_processes));	
	}

	return ;
}

void init_dir_structure() {
	int i;
	for (i = 0; i < NR_TASKS; ++i) {
		dir_ [i].valid = 0;
		dir_ [i].num_of = 0;
	}

	return ;
}

int thread_of (struct task_struct *fth, struct task_struct *son) {
	if (fth->dir_pages_baseAddr == son->dir_pages_baseAddr) {
		return 1;
	}
	
	return -1;

	//This does not really check if is father -> son, just if they share the directory... :)
}
