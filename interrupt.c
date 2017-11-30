/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>

#include <zeos_interrupt.h>
#include <entry.h>

#include <sched.h>

Gate idt[IDT_ENTRIES];
Register    idtR;
unsigned long int zeos_ticks = 0;


char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','¡','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','ñ',
  '\0','º','\0','ç','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}


void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;

  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler (32, clock_handler, 0);
  setInterruptHandler (33, keyboard_handler, 0);
  setTrapHandler (0x80, system_call_handler, 3);

  set_idt_reg(&idtR);
}

int read_keyboard(unsigned char* letter) {
	unsigned char lecture = inb(0x60);
	unsigned char last_bit = lecture >> 7;
	unsigned char bits = lecture & 0b01111111;

	*letter = char_map [bits];

	if (*letter == '\0') *letter = 'C';
	if (!last_bit) { //0 means the user keep pressing the key, so..	
		return 1;
	}
	else return -1;
	
}

extern struct task_struct *idle_task;

void clock_routine (){
/*	
	current()->stats.user_ticks += get_ticks()-current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
*/	
	++zeos_ticks; //Ticks
	zeos_show_clock();
 	schedule();
/*
	current()->stats.system_ticks += get_ticks()-current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
*/
}

void keyboard_routine () {
  	unsigned char char_to_print = ' ';
	read_keyboard(&char_to_print);
	printc_xy(0x00, 0x00, char_to_print);
	
/*	do {
	  printc_xy(0x00, 0x00, char_to_print);
	}
	while (read_keyboard(&char_to_print));		*/
}

