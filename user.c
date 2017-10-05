#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
 while(1) { 
  /*  char to_print[32];
  	itoa(gettime(), &to_print[0]);
  	write(1, &to_print[0], strlen(&to_print[0])); //Uncomment and have some fun!!!!!! 
  }
}
