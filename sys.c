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

#define LECTURA 0
#define ESCRIPTURA 1

extern struct list_head freequeue;

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
		copy_from_user(&parent_union, &child_union, KERNEL_STACK_SIZE*sizeof(unsigned long));
		allocate_DIR(&(child_union->task)); //Does not contemplate any error...
	} 
  	else {
		printk("Insert an error code, that means there are no more pcb's");
	}
	
	int i = 0;
	page_table_entry* PT_child = get_PT(&(child_union->task));
	page_table_entry* PT_parent = get_PT(&(parent_union->task));

	while (i < NUM_PAG_KERNEL) {
		unsigned int kernel_frame_number = get_frame(PT_parent, i);
		set_ss_pag(PT_child, i, kernel_frame_number);
		//from 0 to 255 because we know that addresses (kernel ph_pages)
		i++;
	}
	
	i = NUM_PAG_CODE; //Actually 'i' has that value...	
	while (i < NUM_PAG_CODE) {
		unsigned int code_frame_number = get_frame(PT_parent, i);
		set_ss_pag(PT_child, i, code_frame_number);
		i++;
	}

	i = NUM_PAG_DATA; //Actually 'i' has that value...
	int data_frame_number = alloc_frame(); //That pages are for the child
	while (data_frame_number && i < NUM_PAG_DATA) {
		set_ss_pag(PT_child, i, data_frame_number);
		set_ss_pag(PT_parent, i+NUM_PAG_DATA, data_frame_number);		

		/* Note that I am supposing that the parent has nothing after its own data pages */
	}

	if (!data_frame_number) {
		//That means that no PH frames are left, remember to requeue the PCB we took from the freequeue
		printk("Insert an error code! No more Physical Pages left!");
	}
	else { //Do the copy
		copy_data(PT_parent[NUM_PAG_DATA].bits.pbase_addr, PT_parent[2*NUM_PAG_DATA].bits.pbase_addr, NUM_PAG_DATA*sizeof(unsigned long));
		//And erase the entries from parent PT!!!
		
		for (i = 2*NUM_PAG_DATA; i < 3*NUM_PAG_DATA; i++) {
			del_ss_pag(PT_parent, i);
		}

	}	
	
	//Give a free PID (which??)

	//Task variables != Parent, modify them
	// -kernel_esp
	// -the Directory is modified at the start of the fork...

	//Registers that may be different?
	// -esp0
	// -esp
	// -ebp
	
	//Push eip and ebp (dynamic link)
	//probably they are already made by the parent process

	/* Once we do the following return, we will have to restore the hardware context, and continue the execution of the user process (the one who created us) by using 'ret_from_fork' what will be a task_switch to our parent, or something like that, insert the child process in the ready queue*/

	return PID;
}

void sys_exit()
{  
}

int sys_write (int fd, char* buffer, int size) {
	char mkernel_buff[4];
    int bytesWritten = 0;
/*
	if (check_fd (fd, 1) != 0) {

	}

	if (buffer == NULL) {

	}

	if (size < 0) {

	} Falta meterle los ERRNO y quitar lo de abajo...*/
	   

    if ((check_fd(fd, 1) == 0)){
        if (buffer != NULL && size >= 0) {
                
            // traspaso de bloques de 4 en 4 bytes
            for (;size > 4; buffer -=  4, size -= 4){
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
        else return -EMSGSIZE;
	}

	else return -EACCES; //-1? what means? check the errno table
}

int sys_gettime () {
	
	return zeos_ticks;
}
