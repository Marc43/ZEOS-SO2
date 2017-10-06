/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno = 0;

void perror() {
	char* msg;
	char string [128] = "Seems like all has gone nice! (Or not, but I don't care)...";
	msg = &string[0];	
	  
	if (errno < 0) {
	
         if (errno == -38) {
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
	int rt;
	__asm__ __volatile__("movl 8(%ebp), %ebx;" 
						"movl 12(%ebp), %ecx;"
						"movl 16(%ebp), %edx;" 
       					"movl $0x04, %eax;" 
						"int $0x80;"
					    "movl %eax, -4(%ebp);" 
						);

	if (rt < 0) {
		int errno = rt;	
	}
	
	__asm__ __volatile__("movl %ebp, %esp;"
						 "popl %ebp;"
						 "ret;");

/*__asm__ __volatile__("int 0x80;"
                  :"=a"(ret):"+b"(fd),"+c"(buffer),"+d"(size):"a");*/

}

int gettime () {
	unsigned long int ret;
	__asm__ __volatile__("movl $10, %%eax;"
						"int $0x80;"
						:"=a" ( ret ) ::);

	if (ret < 0) {
		errno = ret;
		return -1;
	}	
	
	return ret;
}
