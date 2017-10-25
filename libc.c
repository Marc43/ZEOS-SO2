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
	/*__asm__ __volatile__("movl 8(%ebp), %ebx;" 
						"movl 12(%ebp), %ecx;"
						"movl 16(%ebp), %edx;" 
       					"movl $0x04, %eax;" 
						"int $0x80;"
					    "movl %eax, -4(%ebp);" 
						);*/
	
	//Codigo supuestamente seguro ante optimizaciones!
	__asm__ __volatile__ ("movl %1, %%ebx;"
                          "movl %2, %%ecx;"
                          "movl %3, %%edx;"
                          "movl $0x04, %%eax;"
                          "int $0x80;"
                          "movl %%eax, %0"
                          : "=m" (rt) 
						  : "m" (fd), "m" (buffer), "m" (size) 
						  : "%eax", "%ebx", "%ecx", "%edx");
	//poner de input ebx, ecx i edx con fd, buffer y size respectivament...
	if (rt < 0) {errno = rt; return -1;}	
	
	return rt;

}

int gettime () {
	unsigned long int ret = 0;
/*	__asm__ __volatile__("movl $10, %eax;"
						"int $0x80;"
						"mov %eax, -4(%ebp);");*/

	__asm__ __volatile__ ("movl $10, %%eax;"
					      "int $0x80;"
						  "movl %%eax, %0;"
						  : "=m" (ret)
						  :
						  : "eax");

	if (ret < 0) {
		errno = ret;
		return -1;
	}	
	
	return ret;
}

int getpid () {
	unsigned long int ret = 0;
	__asm__ __volatile__ ("movl $20, %%eax;"
						  "int $0x80;"
						  "movl %%eax, %0;"
						  : "=m" (ret)
						  :
						  : "eax");
	
	return ret;
}

int fork () {
	unsigned long int ret = 0;
	__asm__ __volatile__ ("movl $2, %%eax;"
						  "int $0x80;"
					      "movl %%eax, %0;" //At this point, child and parent will have different return values
						  : "=m" (ret)
						  :	
						  : "eax");

	return ret;
}

