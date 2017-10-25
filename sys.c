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
  printk("ddd");
  // creates the child process
  
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
