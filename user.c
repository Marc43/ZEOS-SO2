#include <libc.h>
#include <errno.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
	
/*	char *buff;

        char buffer[20] = "hola\n";
	buff = &buffer[0];

	int ret = write(0,buff, strlen(buff));
	if (ret == -EBADF){
		char buffer2[20] = "Error\n";
		buff = &buffer2;
		write(1,buff,strlen(buff));
	}
*/	runjp();	
	while(1) {}	
}
