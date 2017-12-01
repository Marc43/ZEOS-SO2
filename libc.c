/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

#include <errno.h>

int errno = 0;

void perror() {
	char* msg;
	char string [128] = "Seems like";
	msg = &string[0];	
	  
	if (errno < 0) {
	
         if (errno == -EMSGSIZE) {
             char aux [128] = "Function not implemented in ZeOS...";
        	 msg = &aux [0];
		}
 
	}

	write (1, msg, strlen(msg));
}

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

int write (int fd, char* buffer, int size) {
	int rt = 0;
	
	__asm__ __volatile__ ("movl $0x04, %%eax;"
                          "int $0x80;"
                          "movl %%eax, %0;"
                          : "=a" (rt) 
						  : "b" (fd), "c" (buffer), "d" (size));
	
	if (-rt > 0) {errno = -rt; return -1;}	
	
	return rt;

}

int gettime () {
	long int ret = 0;
	__asm__ __volatile__ ("movl $10, %%eax;"
					      "int $0x80;"
						  "movl %%eax, %0;"
						  : "=m" (ret)
						  :
						  : "eax");

	if (-ret > 0) {
		errno = -ret;
		
		return -1;
	}	
	
	return ret;
}

int getpid () {
	long int ret = 0;
	__asm__ __volatile__ ("movl $20, %%eax;"
						  "int $0x80;"
						  "movl %%eax, %0;"
						  : "=m" (ret)
						  :
						  : "eax");
	
	if (-ret > 0) {
		errno = -ret;

		return -1;
	}

	return ret;
}

int fork () {
	long int ret = 0;
	__asm__ __volatile__ ("movl $2, %%eax;"
						  "int $0x80;"
					      "movl %%eax, %0;" //At this point, child and parent will have different return values
						  : "=m" (ret)
						  :	
						  : "eax");

	if (-ret > 0) {
		errno = -ret;
		
		return -1;
	}	

	return ret;
}

void exit () {
	long int ret = 0;
	__asm__ __volatile__ ("movl $1, %%eax;"
						  "int $0x80;"
						  "movl %%eax, %0;"
						  :"=m" (ret) 
						  : 
						  : "eax");

	if (-ret > 0) {
		errno = -ret;
	}
		
	return ;
}

int clone (void (*function) (void), void *stack) {
	long int ret;
	__asm__ __volatile__ ("movl $19, %%eax;"
						  "int $0x80;"
						  "movl %%eax, %0;"
						  : "=m" (ret) 
						  : "b" (function), "c" (stack) 
						  : "eax");
	
	if (-ret > 0) {
		errno = -ret;
		 
		return -1;
	}
	
	return ret;
}
 
int get_stats (int pid, struct stats *st){
	int ret = 0;				// Identificador de llamada a sistema: 35
	__asm__ __volatile__ ("movl 	$35, %%eax;"
			      "int 	$0x80;"
			      "movl 	%%eax, %0;"
			      : "=m" (ret)
			      : "b" (pid), "c" (st)
			      : "eax");
	if (ret < 0){
		errno = -ret;
		ret = -1;
	}

	return ret;
}

int sem_init (int n_sem, unsigned int value) {
	unsigned long int ret = -1;
	__asm__ __volatile__ ("movl $21, %%eax;"
						  "int  $0x80;"
                          "movl %%eax, %0;"
             			  : "=m" (ret)
						  : "b" (n_sem), "c" (value)
                          : "eax");

	if (ret < 0) {
		errno = ret;
		ret = 0;
	}

	return ret;
}

int sem_wait (int n_sem) {
	unsigned long int ret = -1;
	__asm__ __volatile__ ("movl $22, %%eax;"
						  "int  $0x80;"
                          "movl %%eax, %0;"
             			  : "=m" (ret)
						  : "b" (n_sem)
                          : "eax");

	if (ret < 0) {
		errno = ret;
		ret = 0;
	}

	return ret;
}

int sem_signal (int n_sem) {
	unsigned long int ret = -1;
	__asm__ __volatile__ ("movl $23, %%eax;"
						  "int  $0x80;"
                          "movl %%eax, %0;"
             			  : "=m" (ret)
						  : "b" (n_sem)
                          : "eax");

	if (ret < 0) {
		errno = ret;
		ret = 0;
	}

	return ret;
}

int sem_destroy (int n_sem) {
	unsigned long int ret = -1;
	__asm__ __volatile__ ("movl $24, %%eax;"
						  "int  $0x80;"
                          "movl %%eax, %0;"
             			  : "=m" (ret)
						  : "b" (n_sem)
                          : "eax");

	if (ret < 0) {
		errno = ret;
		ret = 0;
	}

	return ret;
}
