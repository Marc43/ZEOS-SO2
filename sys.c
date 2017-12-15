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

int sys_ni_syscall(){
	//Estadisticas RUN_user a RUN_system
	current()->stats.user_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
	
	//Estadisticas RUN_system a RUN_user
	current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
	
	return -38; /*ENOSYS*/
}

int sys_getpid(){
	//Estadisticas RUN_user a RUN_system
	current()->stats.user_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
	
	//Estadisticas RUN_system a RUN_user
	current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();

	return current()->PID;
}

int sys_fork(){
	//Estadisticas RUN_user a RUN_system
	current()->stats.user_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
  	
	union task_union* child_union; 	//PCB del proceso hijo
	union task_union* parent_union;	//PCB del proceso padre

	// Comprobacion de si hay task_struct (PCB) libres en la cola de freequeue
	if (list_empty(&freequeue)) {	
		//printk("Insert an error code, that means there are no more pcb's");
	
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

		return -EAGAIN;  // Antes -ECHILD	
	}
	// Hay un PCB libre y podemos usarlo para el proceso hijo 
  	else {	
		struct list_head* lh = list_first (&freequeue);
		list_del(lh);

		struct task_struct* child = list_head_to_task_struct(lh);
		child_union = (union task_union*)child;

		struct task_struct* parent = current();
		parent_union = (union task_union*)parent;
		
	}

	//Busqueda de páginas físicas de memoria (frames)
	int i = 0; int frame=0;
	unsigned int ph_pages [NUM_PAG_DATA];
	
	while (frame >= 0 && i < NUM_PAG_DATA) {
		frame = alloc_frame();	
	
		if (frame < 0) {
			free_user_pages(&(child_union->task)); //<-- ¿ Qué hace esto aquí ? !!!	
	
			int size_phpages = i; 
			for (i = 0; i < size_phpages; i++) free_frame(ph_pages [i]);		

			list_add_tail (&(child_union->task.list), &freequeue); //Free frames and restore pcb            
			
			//Estadisticas RUN_system a RUN_user
			current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
			current()->stats.elapsed_total_ticks = get_ticks();

			return -ENOMEM;
		}
		ph_pages[i] = frame;
		i++;
	}
	
	copy_data(parent_union, child_union, sizeof(union task_union));
	
	if (!allocate_DIR(&(child_union->task))) return -ENOMEM;

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
	child_union->task.quantum = get_quantum(&parent_union->task);
	set_cr3(parent_union->task.dir_pages_baseAddr);	
	
	//We could also take the ebp of the parent
	//And just: ebp >> 4 (number of elements in the stack)
	//stack [1023 - (ebp >> 4)]
	child_union->task.kernel_esp = &(child_union->stack [KERNEL_STACK_SIZE-18]);	
	child_union->stack[KERNEL_STACK_SIZE-17] = (unsigned long *)&ret_from_fork;
	
	// Init estadisticas
	child_union->task.state = ST_READY;
	child_union->task.stats.user_ticks = 0;
	child_union->task.stats.system_ticks = 0;
	child_union->task.stats.blocked_ticks = 0;
	child_union->task.stats.ready_ticks = 0;
	child_union->task.stats.total_trans = 0;
	child_union->task.stats.remaining_ticks = 0;
	child_union->task.stats.elapsed_total_ticks = get_ticks();
	
	list_add_tail(&(child_union->task.list), &readyqueue);
	
	//Estadisticas RUN_system a RUN_user
	current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();

	return child_union->task.PID; //Se devuelve el siquiente PID que se puede usar
}

void sys_exit() {
	//Estadisticas RUN_user a RUN_system
	current()->stats.user_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();

	struct task_struct* in_cpu = current();
	page_table_entry* PT = get_PT(in_cpu);

	int pPID = in_cpu->PID;
	
	in_cpu->info_dir_->num_of--;
	if (in_cpu->info_dir_->num_of <= 0) {
		in_cpu->info_dir_->valid = 0;
		
		int i;
		for (i = NUM_PAG_KERNEL+NUM_PAG_CODE; i < NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; ++i) {	
			free_frame (get_frame(PT, i));	
			del_ss_pag(PT, i);
		} 
	}

	int i;
	for (i = 0; i < NUM_SEMAPHORES; i++) { //NUM_SEMAPHORES
		if (sem_vector [i].owner_pid == pPID) {
			sys_sem_destroy(i);
		}
	}

	list_add_tail(&(current()->list), &freequeue);	

	current()->PID = -1; //To ensure our 'search' algorithm does not match at any cost

	sched_next_rr();

	return ;	
}

int sys_clone (void (*function)(void), void* stack) {

	if (!(access_ok(VERIFY_WRITE, stack, 4) && access_ok(VERIFY_READ, function, 4))) return -EFAULT;

	if (!list_empty(&freequeue)) {	

		struct list_head* lh = list_first (&freequeue);
		list_del(lh);
		
		struct task_struct* task_thread = list_head_to_task_struct (lh);
		union task_union* 	thread = (union task_union*) task_thread;
		union task_union*	current_tasku = (union task_union*) current();

		copy_data(current_tasku, thread, sizeof(union task_union));

		thread->task.kernel_esp = &(thread->stack[KERNEL_STACK_SIZE-18]); 
		thread->task.PID = last_PID++;
		thread->task.info_dir_->num_of++; //Update the number of proccesses on that directory...		
		thread->task.dir_pages_baseAddr = current_tasku->task.dir_pages_baseAddr; //Just in case...
	
		thread->stack[KERNEL_STACK_SIZE-2] = stack; 
		thread->stack[KERNEL_STACK_SIZE-5] = function;
		thread->stack[KERNEL_STACK_SIZE-18] = 0xaaaa;
		thread->stack[KERNEL_STACK_SIZE-17] = &ret_from_fork;
		
	
		thread->task.state = ST_READY;
		thread->task.stats.user_ticks = 0;
		thread->task.stats.system_ticks = 0;
		thread->task.stats.blocked_ticks = 0;
		thread->task.stats.ready_ticks = 0;                   		
		thread->task.stats.total_trans = 0;
		thread->task.stats.remaining_ticks = 0;
		thread->task.stats.elapsed_total_ticks = get_ticks(); 	
			
		list_add_tail(&(thread->task.list), &readyqueue);

		return last_PID-1;
	}
	else {
		printk ("No more PCB's free, output an error (Errno)");
		return -ENOMEM;
	}

	return -1;
}

int sys_write (int fd, char* buffer, int size) {
	//Estadisticas RUN_user a RUN_system
	current()->stats.user_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();

	char mkernel_buff[4];
	int bytesWritten = 0;
	int error = check_fd (fd, ESCRIPTURA);
	
	if (error != 0){
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

		return -EBADF;
	}
	if (buffer == NULL){
       		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

        	return -EFAULT;
	}
	if (size < 0){
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

		return -EINVAL;
	}
	if (access_ok(VERIFY_READ, buffer, size) == 0){
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

		return -EACCES;
	}
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
	
	//Estadisticas RUN_system a RUN_user
	current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();

    	return bytesWritten;    // Devuelve el num. de bytes escritos
}

int sys_gettime () {	
	//Estadisticas RUN_user a RUN_system
	current()->stats.user_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
	
	//Estadisticas RUN_system a RUN_user
	current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();

	return zeos_ticks;
}

int sys_get_stats (int pid, struct stats *st){
	//Estadisticas RUN_user a RUN_system
	current()->stats.user_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();

	int i;
	
	if (pid < 0) {
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

 		return -EINVAL;
	}
	
	if (access_ok(VERIFY_WRITE,st,sizeof(struct stats)) == 0){
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

		return -EFAULT;
	}

	//Busqueda del PCB
	if (current()->PID == pid){
		copy_to_user(&(current()->stats), st, sizeof(struct stats));
		
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

		return 0;
	}

	for (i = 0; i < NR_TASKS; ++i){
		if (task[i].task.PID == pid && task[i].task.state == ST_READY){
			copy_to_user(&(task[i].task.stats), st, sizeof(struct stats));

			//Estadisticas RUN_system a RUN_user
			current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
			current()->stats.elapsed_total_ticks = get_ticks();

			return 0;
		}
	}
	//Estadisticas RUN_system a RUN_user
	current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();

	return -ESRCH;
	
}

int sys_sem_init (int n_sem, int value) {
	//ebx: num_sem, ecx: value
	//Creates a new semaphore with num: num_sem && blocked queue size: value
	if (n_sem >= NUM_SEMAPHORES || n_sem < 0 || value < 0) return -EINVAL;

	if (sem_vector [n_sem].owner_pid == -1) { //Checks if it is initialized and also free!
		sem_vector [n_sem].owner_pid = current()->PID;
		sem_vector [n_sem].max_blocked = value;
		sem_vector [n_sem].num_blocked = value; //TODO check 'value'
		//We do nothing with the blocked queue by the moment...
	}
	else { 
		printk("That semaphore already exists!");
		
		return -EINVAL;
	}

	return 0;	
}

int sys_sem_wait (int n_sem) {
	if (n_sem >= NUM_SEMAPHORES || n_sem < 0) return -EINVAL;

	if (sem_vector [n_sem].owner_pid == -1) return -EINVAL;

	if (sem_vector [n_sem].num_blocked <= 0) {
		//Whops, someone must be blocked
		update_process_state_rr(current(), (&(sem_vector [n_sem].blocked_processes)));
		sched_next_rr(); //The next instruction will be executed when the sem. is destroyed or it is unblocked
		if (sem_vector [n_sem].owner_pid == -1) return -EINVAL; 
	}
	else
		sem_vector [n_sem].num_blocked--;
		
	return 0;
}

int sys_sem_signal (int n_sem) {
	if (n_sem >= NUM_SEMAPHORES || n_sem < 0) return -EINVAL;

	if (sem_vector [n_sem].owner_pid == -1) return -EINVAL;

	if (sem_vector [n_sem].num_blocked > 0 || list_empty(&(sem_vector [n_sem].blocked_processes)))
		sem_vector [n_sem].num_blocked++;
	else if (!list_empty(&(sem_vector [n_sem].blocked_processes))){
		struct list_head* lh = list_first(&(sem_vector [n_sem].blocked_processes));
		struct task_struct* first = list_head_to_task_struct(lh);
		update_process_state_rr(first, &readyqueue); //Unblock			
	}

	return 0;
}

int sys_sem_destroy (int n_sem) {
	if (n_sem >= NUM_SEMAPHORES || n_sem < 0) return -EINVAL;

	int some_blocked = 0;
	if (sem_vector [n_sem].owner_pid == current()->PID) {
		sem_vector [n_sem].owner_pid   = -1;
		sem_vector [n_sem].num_blocked = 0;

		struct list_head* blocked_queue = &(sem_vector[n_sem].blocked_processes);
	
		while (!blocked_queue) {
			if (some_blocked == 0) some_blocked = -1;
			struct list_head* lh = list_first(&(sem_vector [n_sem].blocked_processes));
			struct task_struct* first = list_head_to_task_struct(lh);
			//list_del(lh);

			update_process_state_rr(first, &readyqueue);
		}

		sem_vector [n_sem].max_blocked = 0;

	}
	else 
		return -EINVAL;

	return some_blocked;
}
