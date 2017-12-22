/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>

#define NR_TASKS      10
#define KERNEL_STACK_SIZE	1024
#define QUANTUM	10
#define NUM_SEMAPHORES 20

struct semaphore {
	int owner_pid;
	int num_blocked;
	int max_blocked;
	struct list_head blocked_processes;
};

struct semaphore sem_vector[NUM_SEMAPHORES];

struct info_dir {
	unsigned int valid;
	unsigned int num_of;
};

extern enum state_t { ST_FREE, ST_READY, ST_BLOCKED, ST_RUN };

struct stats {
	unsigned long user_ticks;
	unsigned long system_ticks;
	unsigned long blocked_ticks;
	unsigned long ready_ticks;
	unsigned long total_trans;
	unsigned long remaining_ticks;
	unsigned long elapsed_total_ticks;
};

struct iorb {
	char* ubuf;
	int	  remaining;
	int	  last_pos;
	//Incomplete iorb bc is just for reading
};

struct heap {
	int	  last_logical;
	void* pointer_byte;
};

struct task_struct {
  int PID;			/* Process ID. This MUST be the first field of the struct. */
  page_table_entry* dir_pages_baseAddr;
  
  unsigned int kernel_esp; //To undo the dynamic link and some black magic
  unsigned int quantum;
  
  enum state_t state;
  
  struct list_head list; 

  struct info_dir* info_dir_;

  struct stats stats;


  struct iorb iorb;

  struct heap heap;

};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE]; //Stack system per process 
};

struct list_head freequeue;
struct list_head readyqueue;

extern union task_union protected_tasks[NR_TASKS+2];
extern union task_union *task; /* Vector de tasques */

#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

#define INITIAL_ESP       	KERNEL_ESP(&task[1])

/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_sched(void);

struct task_struct * current();

void task_switch(union task_union*t);

void inner_task_switch (union task_union* t);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
void sched_next_rr();

void update_process_state_rr(struct task_struct *t, struct list_head *dest);

int needs_sched_rr();

void update_sched_data_rr();

void schedule();

/* Quantum getter and setter */

int get_quantum (struct task_struct *t); //How many CPU ticks a process can waste

void set_quantum (struct task_struct *t, int new_quantum); //Set a new quantum for task t

int thread_of (struct task_struct *fth, struct task_struct *son);

/* Init free && ready queue */

void init_free_queue();

void init_ready_queue();

void init_semaphores();

void init_dir_structure();

#endif  /* __SCHED_H__ */
