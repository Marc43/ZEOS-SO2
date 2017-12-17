#include <io.h>
#include <utils.h>
#include <list.h>
#include <sched.h>

#define IO_BUFFER_SIZE 1024

// Queue for blocked processes in I/O 
struct list_head blocked;

char io_buffer [IO_BUFFER_SIZE];
char *write;
char * read;

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

int is_empty_iobuf() {
	if (write == read)
		return 1;
	
	return 0;
}

void update_rw_pointer (char** pointer) {
	//This is used for io_buffer
	if (*pointer - &(io_buffer[0]) == IO_BUFFER_SIZE-1) *pointer = io_buffer [0];
	else ++(*pointer);

	/* If we've reached the end of the structure start over again,
	   else add one more position */ 
} 

int copy_to_ubuf (int count) { 
	//Copy to the IORB buffer (ubuf)
	int to_read = write - read;

	if (is_empty_iobuf() && to_read < count) return -1;
	
	int i = current()->iorb.last_pos;
	int readen = 1;	
	while (!is_empty_iobuf() && readen <= count) {
		current()->iorb.ubuf [i] = (char)(*read);
		update_rw_pointer(&read);
		++i; readen++;
	}

	if (readen != 1) {
		current()->iorb.remaining -= readen-1;
		current()->iorb.last_pos = i;
	}

	return current()->iorb.remaining; 
} //Returns the amount of characters left to read...

void write_char_to_iobuf(char c) {
	*(write) = c;
	 update_rw_pointer(&write);
}

int len_iobuf() {
	return write - read;
}

int sys_read_keyboard() {
	int count = current()->iorb.remaining;
	if (!list_empty(&blocked)) { //Is possible that some process is currently reading? I think so... TODO Check.
		//Means that someone is reading, so we have to block the current process
		update_process_state_rr (current(), &blocked);
		sched_next_rr();
	}

	//Now, we actually read
	while (1) {
		if (copy_to_ubuf(current()->iorb.remaining) != 0) {
			current()->state = ST_BLOCKED;
			list_add(&(current()->list), &blocked);
			sched_next_rr();
		}
		else 
			return 0;
	}
}
