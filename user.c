#include <libc.h>

int pid;

char buff[128] = "Clone fucking worked!";

void dummy_function () {
	write(1, &buff[0], strlen(buff));
	exit();
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

	char* k [1024];
	clone((unsigned long*) dummy_function, &k[0]);
	
	while(1) {}	
}

