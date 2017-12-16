#include <io.h>
#include <utils.h>
#include <list.h>

#define IO_BUFFER_SIZE 1024

// Queue for blocked processes in I/O 
struct list_head blocked;

char io_buffer [IO_BUFFER_SIZE];
char* write, read;

/* A read from the keyboard blocks a process if there are other
   processess waiting for their I/O or in it's defect if it did not
   got enough characters (waiting more input...) */

int sys_write_console(char *buffer,int size)
{
  int i;
  
  for (i=0; i<size; i++)
    printc(buffer[i]);
  
  return size;
}

void init_io_structures () {
	INIT_LIST_HEAD(&blocked);
	write = read = &(io_buffer [0]); 
}

void reset_iobuf () {
	write = read = &(io_buffer [0]);
}

int is_iobuf_full () {
	if (write == &(io_buffer [IO_BUFFER_SIZE-1])) return 1;
	
	return -1;
}

void copy_to_ubuf (int count) {
	//Copy to the IORB buffer (ubuf)
	
}

int sys_read_keyboard() {
	if (!list_empty(&blocked)) { //Is possible that some process is currently reading? I think so... TODO Check.
		//Means that someone is reading, so we have to block the current process
		update_process_state_rr (current(), &blocked);
		sched_next_rr();
	}

	//Now, we actually read
	while (current()->iorb.remaining != 0) { //Until read is done
			
	}


	
}
