/*
 * entry.S - Entry point to system mode from user mode
 */

#include <asm.h>
#include <segment.h>
#include <errno.h>

/**************************************************/
/**** Save & Restore ******************************/
/**                                              **/
/** When we change to privilege level 0 (kernel) **/
/** (through an interrupt, a system call, an     **/
/** exception ...) we must save the state of the **/
/** currently running task (save).               **/
/**                                              **/
/** Stack layout in 'systemCall':                **/
/**                                              **/
/**   0(%esp) - %ebx    \                        **/
/**   4(%esp) - %ecx     |                       **/
/**   8(%esp) - %edx     |                       **/
/**   C(%esp) - %esi     | Register saved        **/
/**  10(%esp) - %edi     |  by 'save'            **/
/**  14(%esp) - %ebp     |                       **/
/**  18(%esp) - %eax     |                       **/
/**  1C(%esp) - %ds      |                       **/
/**  20(%esp) - %es      |                       **/
/**  24(%esp) - %fs      |                       **/
/**  28(%esp) - %gs     /                        **/
/**  2C(%esp) - %eip    \                        **/
/**  30(%esp) - %cs      |                       **/
/**  34(%esp) - %eflags  |  Return context saved **/
/**  38(%esp) - %oldesp  |   by the processor.   **/
/**  3C(%esp) - %oldss  /                        **/
/**                                              **/
/**************************************************/

#define SAVE_ALL \
      pushl %gs; \
      pushl %fs; \
      pushl %es; \
      pushl %ds; \
      pushl %eax; \
      pushl %ebp; \
      pushl %edi; \
      pushl %esi; \
      pushl %edx; \
      pushl %ecx; \
      pushl %ebx; \
      movl $__KERNEL_DS, %edx;    \
      movl %edx, %ds;           \
      movl %edx, %es;			\

#define RESTORE_ALL \
      popl %ebx; \
      popl %ecx; \
      popl %edx; \
      popl %esi; \
      popl %edi; \
      popl %ebp; \
      popl %eax; \
      popl %ds;  \
      popl %es;  \
      popl %fs;  \
      popl %gs;  \ 

#define EOI \
      movb $0x20, %al; \
      outb %al, $0x20;  

ENTRY (clock_handler)\
	SAVE_ALL
	EOI
	call clock_routine;
	RESTORE_ALL
	iret;

ENTRY (keyboard_handler)
	SAVE_ALL  
	EOI 
	call keyboard_routine; \
	RESTORE_ALL 
	iret; \

ENTRY (system_call_handler)
	SAVE_ALL 
	cmpl $0, %eax; //eax < 0 ?
	jl err;
	cmpl $MAX_SYSCALL, %eax; //eax > 0 ?
	jg err;
	call *sys_call_table (, %eax, 0x04);
	jmp fin;
err:
	movl $-ENOSYS, %eax;	
fin:
	movl %eax, 0x18(%esp); //Modify the value in the Stack-eax so when we restore the context actually we changed a little bit the context
	RESTORE_ALL
	iret; //Return from kernel privilege level...

ENTRY (ret_from_fork)
	RESTORE_ALL
	movl $0, %eax;
	iret;	
