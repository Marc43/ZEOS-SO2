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

#include <entry.h> //Better extern ret_from_fork() ?

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

	//We duplicate the dynamic link in local variables
	unsigned long ebp_p, eip_p;
	eip_p = (unsigned long)(&ret_from_fork);
	__asm__ __volatile__ ("movl 0(%%ebp), %%eax;"
						  "movl %%eax, %0;"
						  :"=m" (ebp_p)
						  :
						  : "%eax");
	//I move them to vars at the start just to ensure

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

	i = NUM_PAG_KERNEL + NUM_PAG_CODE;
	int j = 0;
	int data_frame_number = ph_pages [j];
	
	while (data_frame_number >= 0 && (i < NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA && j < NUM_PAG_DATA)) {
		set_ss_pag(PT_child, i, data_frame_number);
		set_ss_pag(PT_parent, i+NUM_PAG_DATA, data_frame_number);		
		++i; ++j;
		data_frame_number = ph_pages [j];
		/* Note that I am supposing that the parent has nothing after its own data pages */
	}

	if (data_frame_number < 0) {
		//That means that no PH frames are left, remember to requeue the PCB we took from the freequeue
		printk("Insert an error code! No more Physical Pages left!");
		return -ENOMEM;
	}
	else { //Do the copy
		printk("Aqui reviento muy facil");
		i = L_USER_START + (NUM_PAG_CODE*PAGE_SIZE);	
		copy_data((void *)i, (void *)(L_USER_START) + (NUM_PAG_CODE+NUM_PAG_DATA)*PAGE_SIZE, NUM_PAG_DATA*PAGE_SIZE);	
		//test copy_data(L_USER_START, L_USER_START + 1, 1);
		for (i = NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA; i < NUM_PAG_KERNEL + NUM_PAG_CODE + 2*NUM_PAG_DATA; i++) {
			del_ss_pag(PT_parent, i);
		}

	}	

	PID = last_PID++;	
	child_union->task.PID = PID;

	set_cr3(child_union->task.dir_pages_baseAddr);	
	
	//Push eip and ebp (extra dynamic link)
	__asm__ __volatile__ ("movl %%esp, %0;"
						  "push %%ebx;"
						  "push %%eax;"
						  :"=m" (child_union->task.kernel_esp)
						  :"a" (ebp_p), "b" (eip_p));

	list_add_tail(&(child_union->task.list), &readyqueue);

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
