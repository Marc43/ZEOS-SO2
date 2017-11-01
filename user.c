#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
	
	int k =	fork();	
	char* verifica = "";
	if (k == 0) {
		verifica = "Son\n";
	}
	else verifica = "Dad\n";
	write(1, verifica, strlen(verifica));

 	while(1) {}
	
}
