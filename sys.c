/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#include <entry.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern struct list_head freequeue;
extern unsigned long last_PID;

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{

  	int PID=-1;
  	// creates the child process
  	union task_union* child_union;
	union task_union* parent_union;

	if (!list_empty(&freequeue)) {
		struct list_head* lh = list_first (&freequeue);
		list_del(lh);

		struct task_struct* child = list_head_to_task_struct(lh);
		child_union = (union task_union*)child;

		struct task_struct* parent = current();
		parent_union = (union task_union*)parent;

		//The size of the union is the max(task_struct, stack)	
		copy_data(parent_union, child_union, sizeof(union task_union));

		allocate_DIR(&(child_union->task)); //Does not contemplate any error...
	} 
  	else {
		printk("Insert an error code, that means there are no more pcb's");
	
		return -ECHILD;	
	}

	//Search for physical pages
	int i = 0;
	int frame = alloc_frame();	
	unsigned int ph_pages [NUM_PAG_DATA];
	
	while (frame >= 0 && i < NUM_PAG_DATA) {
		ph_pages [i] = frame;
		frame = alloc_frame();
		i++;		
	}
	
	if (frame < 0) {
		printk("Insert an error code, no more physical pages available");
	
		int i = 0;
        while (i < NUM_PAG_DATA && ph_pages [i] > 0)                  	free_frame(ph_pages [i]);

		list_add_tail (&(child_union->task.list), &freequeue); //Free frames and restore pcb            
        //Is the allocated DIR someway attached to the PCB????
		
		return -ENOMEM;
	}

	i = 0;
	page_table_entry* PT_child = get_PT(&(child_union->task));
	page_table_entry* PT_parent = get_PT(&(parent_union->task));

	while (i < NUM_PAG_KERNEL) {
		unsigned int kernel_frame_number = get_frame(PT_parent, i);
		set_ss_pag(PT_child, i, kernel_frame_number);
		//from 0 to 255 because we know that addresses (kernel ph_pages)
		i++;
	}

	i = NUM_PAG_KERNEL;	
	while (i < NUM_PAG_KERNEL + NUM_PAG_CODE) {
		unsigned int code_frame_number = get_frame(PT_parent, i);
		set_ss_pag(PT_child, i, code_frame_number);
		i++;
	}

	int j = 0;
	
	for (i = NUM_PAG_KERNEL+NUM_PAG_CODE; i < (NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA); ++i) {
		int data_frame_number = ph_pages [j];
		set_ss_pag(PT_child, i, data_frame_number);
		set_ss_pag(PT_parent, i+NUM_PAG_DATA, data_frame_number);
		++j;
	} 
		
	i = L_USER_START + (NUM_PAG_CODE*PAGE_SIZE);	
		copy_data((void *)i, (void *)((L_USER_START) + (NUM_PAG_CODE+NUM_PAG_DATA)*PAGE_SIZE), NUM_PAG_DATA*PAGE_SIZE);	
		
	for (i = NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA; i < NUM_PAG_KERNEL + NUM_PAG_CODE + (2*NUM_PAG_DATA); i++) {
			del_ss_pag(PT_parent, i);
		}
	
	child_union->task.PID = last_PID++; 

	set_cr3(parent_union->task.dir_pages_baseAddr);	
	
	//We could also take the ebp of the parent
	//And just: ebp >> 4 (number of elements in the stack)
	//stack [1023 - (ebp >> 4)]
	child_union->task.kernel_esp = &(child_union->stack [1023-18]);	
	child_union->stack[1023-17] = (unsigned long *)&ret_from_fork;

	list_add_tail(&(child_union->task.list), &readyqueue);

	return PID;
}

void sys_exit() {
	struct task_struct* in_cpu = current();
	//We have to free the frames, and 're'-queue the PCB
	page_table_entry* PT = get_PT(in_cpu);

	//list_del(&(in_cpu->list)); This process is actually running, so is not within any list

	int i;
	for (i = NUM_PAG_KERNEL+NUM_PAG_CODE; i < NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; ++i) 
		free_frame (get_frame(PT, i));	

	//ALliberar PCB (encuar cua FREES);	
	list_del(&(in_cpu->list));
	
	sched_next_rr();
}

int sys_write (int fd, char* buffer, int size) {
	char mkernel_buff[4];
    int bytesWritten = 0;
	int error = check_fd (fd, 1);
	
	if (error != 0) return error;
	if (buffer == NULL) return -EINVAL;
	if (size <= 0) return -EINVAL;
	if (access_ok(VERIFY_READ, buffer, size) == 0) return -EACCES;
	
    // traspaso de bloques de 4 en 4 bytes
    for (;size > 4; buffer +=  4, size -= 4){
		copy_from_user(buffer, mkernel_buff, 4);
		bytesWritten += sys_write_console(mkernel_buff,4);
	}
            
    // traspaso del resto del buffer
    if (size != 0){
	    copy_from_user(buffer, mkernel_buff, size);
	    bytesWritten += sys_write_console(mkernel_buff,size);
    }
 
    return bytesWritten;    // Devuelve el num. de bytes escritos
}

int sys_gettime () {
	
	return zeos_ticks;
}

int sys_get_stats (int pid, struct stats *st){
	struct task_struct *ts;
	
	//Comprobaciones
	if (access_ok(VERIFY_WRITE,st,56) == 0) return -EACCES;
	
	//Busqueda del PCB
	ts = getPCBfromPID (pid, &readyqueue);
	if (ts != NULL ) copy_to_user(&ts->stats, st, 56);

	else return -ESRCH;
	return 0;
}

int sys_sem_init (int n_sem, int value) {
	//ebx: num_sem, ecx: value
	//Creates a new semaphore with num: num_sem && blocked queue size: value
	if (n_sem >= 20) return -EINVAL;

	if (sem_vector [n_sem].owner_pid == -1) {
		sem_vector [n_sem].max_blocked = value;
		sem_vector [n_sem].num_blocked = 0;
		//We do nothing with the blocked queue by the moment...
	}
	else { 
		printk("That semaphore already exists!");
		
		return -EINVAL;
	}

	return 0;	
}

int sys_sem_wait (int n_sem) {
	if (n_sem >= 20) return -EINVAL;

	if (sem_vector [n_sem].owner_pid == -1) return -EINVAL;

	if (sem_vector [n_sem].num_blocked <= 0) {
		//Whops, someone must be blocked
		update_process_state_rr(current(), (&(sem_vector [n_sem].blocked_processes)));
	}
	else
		sem_vector [n_sem].num_blocked--;
		
	return 0;
}

int sys_sem_signal (int n_sem) {
	if (n_sem >= 20) return -EINVAL;

	if (sem_vector [n_sem].owner_pid == -1) return -EINVAL;

	if (sem_vector [n_sem].num_blocked == 0)
		sem_vector [n_sem].num_blocked++;
	else {
		sem_vector [n_sem].num_blocked--;
		struct list_head* lh = list_first(&(sem_vector [n_sem].blocked_processes));
		struct task_struct* first = list_head_to_task_struct(lh);
		update_process_state_rr(first, &readyqueue); //Unblock		
	}

	return 0;
}

int sys_sem_destroy (int n_sem) {
	if (n_sem >= 20) return -EINVAL;

	if (sem_vector [n_sem].owner_pid == -1) return -EINVAL;

	if (sem_vector [n_sem].owner_pid == current()->PID) {
		sem_vector [n_sem].owner_pid   = -1;
		sem_vector [n_sem].max_blocked = 0;
		INIT_LIST_HEAD(&(sem_vector [n_sem].blocked_processes));
		//TODO Check this out..	
	}
	else 
		return -EINVAL;

	return 0;
}
