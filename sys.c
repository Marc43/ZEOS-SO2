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

  	int PID=-1;
  	
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
		
		//The size of the union is the max(task_struct, stack)	
// copy_data(parent_union, child_union, sizeof(union task_union));

// allocate_DIR(&(child_union->task));
	}

	//Busqueda de páginas físicas de memoria (frames)
	int i = 0; int frame=0;
	unsigned int ph_pages [NUM_PAG_DATA];
	
	while (frame >= 0 && i < NUM_PAG_DATA) {
		frame = alloc_frame();	
		//ph_pages [i] = frame;
		//i++;		
	
		if (frame < 0) {
			// En este punto es que no tenemos más frames libres
			// Por tanto hay que liberar los frames obtenidos hasta ahora

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
	allocate_DIR(&(child_union->task));

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
	child_union->task.kernel_esp = &(child_union->stack [1023-18]);	
	child_union->stack[1023-17] = (unsigned long *)&ret_from_fork;
	
	// Init estadisticas
	child_union->task.stats.user_ticks = 0;
	child_union->task.stats.system_ticks = 0;
	child_union->task.stats.blocked_ticks = 0;
	child_union->task.stats.ready_ticks = 0;
	child_union->task.stats.total_trans = 0;
	child_union->task.stats.remaining_ticks = 0;
	child_union->task.stats.elapsed_total_ticks = get_ticks();
	
	//Ponemos el proceso hijo en estado READY
	child_union->task.state = ST_READY;
	//y lo ponemos en la cola de ready's
	list_add_tail(&(child_union->task.list), &readyqueue);
	
	//Estadisticas RUN_system a RUN_user
	current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();

	return last_PID; //Se devuelve el siquiente PID que se puede usar
}

void sys_exit() {
	//Estadisticas RUN_user a RUN_system
	current()->stats.user_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
/*
	struct task_struct* in_cpu = current();

	page_table_entry* PT = get_PT(in_cpu);
	
	int i = 0;
	while (i < (NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA)) {
		if (i > (NUM_PAG_KERNEL+NUM_PAG_CODE)) {
			unsigned int frame = get_frame(PT, i);
			free_frame(frame);
		}
		del_ss_pag(PT, i);
		++i;	
	}
*/
	free_user_pages(current());
	current()->PID = -1; //To ensure our 'search' algorithm does not match at any cost
	current()->state = 0;
	update_process_state_rr(current(), &freequeue);
	--last_PID;
	
	sched_next_rr();
	
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

	//Comprobaciones
	if (pid < 0 ){
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

 		return -EINVAL;
	}
	
	if (access_ok(VERIFY_WRITE,st,sizeof(struct stats_s)) == 0){
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

		return -EFAULT;
	}

	//Busqueda del PCB
	if (current()->PID == pid){
		copy_to_user(&(current()->stats), st, sizeof(struct stats_s));
		
		//Estadisticas RUN_system a RUN_user
		current()->stats.system_ticks += get_ticks() - current()->stats.elapsed_total_ticks;
		current()->stats.elapsed_total_ticks = get_ticks();

		return 0;
	}

	for (i = 0; i < NR_TASKS; ++i){
		if (task[i].task.PID == pid && task[i].task.state == ST_READY){
			copy_to_user(&(task[i].task.stats), st, sizeof(struct stats_s));

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
