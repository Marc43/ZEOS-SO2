#ifndef DEVICES_H__
#define  DEVICES_H__

int sys_write_console(char *buffer,int size);
void init_io_structures();
void update_rw_pointer (char* pointer);
int copy_to_ubuf (int count);
void write_char_to_iobuf(char c);
int sys_read_keyboard();

#endif /* DEVICES_H__*/
