/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <global_vars.h>

#define LECTURA 0
#define ESCRIPTURA 1

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
  
  return PID;
}

void sys_exit()
{  
}

int sys_write (int fd, char* buffer, int size) {
	
	//char* mkernel_buff;
	//copy_to_user(buffer, mkernel_buff, size);
	//printk(buffer);
	if ((check_fd(fd, 1) == 0) && buffer != NULL && size >= 0) {
		sys_write_console(buffer, size);
		return 0;		
	}
	else return -1; //-1? what means? check the errno table

	return 0;
}

int sys_getticks () {
	return zeos_ticks;
}
