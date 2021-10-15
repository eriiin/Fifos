#include "multiboot.h"
#include "types.h"
#include "interrupt.h"

#define N 3
#define STACK_N 1024
#define THREAD_LOOP_N 3
#define THTREAD_SLEEP_TICKS (50000000)


/* Hardware text mode color constants. */
enum vga_color
{
  COLOR_BLACK = 0,
  COLOR_BLUE = 1,
  COLOR_GREEN = 2,
  COLOR_CYAN = 3,
  COLOR_RED = 4,
  COLOR_MAGENTA = 5,
  COLOR_BROWN = 6,
  COLOR_LIGHT_GREY = 7,
  COLOR_DARK_GREY = 8,
  COLOR_LIGHT_BLUE = 9,
  COLOR_LIGHT_GREEN = 10,
  COLOR_LIGHT_CYAN = 11,
  COLOR_LIGHT_RED = 12,
  COLOR_LIGHT_MAGENTA = 13,
  COLOR_LIGHT_BROWN = 14,
  COLOR_WHITE = 15,
};


enum tcb_state {
  UNUSED = 0,
  RUNNABLE,
  RUNNING,
  DONE,
};

static uint32_t stack1[STACK_N];
static uint32_t stack2[STACK_N];
static uint32_t stack3[STACK_N];

struct tcb {
  uint32_t* sp;
  int tid;
  uint32_t* bp;
  enum tcb_state state;
} thread_tcbs[N];

int curr_tcb_index = -1;

typedef void (*thread_entry)();

void thread_create(thread_entry func, void* stack);
void thread_sleep(int ticks);
void thread_schedule();
void thread_yield();

void thread_1();
void thread_2();
void thread_3();


uint8_t make_color(enum vga_color fg, enum vga_color bg)
{
  return fg | bg << 4;
}
 
uint16_t make_vgaentry(char c, uint8_t color)
{
  uint16_t c16 = c;
  uint16_t color16 = color;
  return c16 | color16 << 8;
}
 
size_t strlen(const char* str)
{
  size_t ret = 0;
  while ( str[ret] != 0 )
    ret++;
  return ret;
}
 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 24;
 
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;
 
void terminal_initialize()
{
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
  terminal_buffer = (uint16_t*) 0xB8000;
  for ( size_t y = 0; y < VGA_HEIGHT; y++ )
    {
      for ( size_t x = 0; x < VGA_WIDTH; x++ )
	{
	  const size_t index = y * VGA_WIDTH + x;
	  terminal_buffer[index] = make_vgaentry(' ', terminal_color);
	}
    }
}
 
void terminal_setcolor(uint8_t color)
{
  terminal_color = color;
}
 
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = make_vgaentry(c, color);
}
 
void terminal_putchar(char c)
{
  if ('\n' == c) 
  {
    terminal_row++;
    terminal_column = 0;
    return;
  }

  terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
  if ( ++terminal_column == VGA_WIDTH )
    {
      terminal_column = 0;
      if ( ++terminal_row == VGA_HEIGHT )
	{
	  terminal_row = 0;
	}
    }
}
 
void terminal_writestring(const char* data)
{
  size_t datalen = strlen(data);
  for ( size_t i = 0; i < datalen; i++ )
    terminal_putchar(data[i]);
}
 

/* Convert the integer D to a string and save the string in BUF. If
   BASE is equal to 'd', interpret that D is decimal, and if BASE is
   equal to 'x', interpret that D is hexadecimal. */
void itoa (char *buf, int base, int d)
{
  char *p = buf;
  char *p1, *p2;
  unsigned long ud = d;
  int divisor = 10;
     
  /* If %d is specified and D is minus, put `-' in the head. */
  if (base == 'd' && d < 0)
    {
      *p++ = '-';
      buf++;
      ud = -d;
    }
  else if (base == 'x')
    divisor = 16;
     
  /* Divide UD by DIVISOR until UD == 0. */
  do
    {
      int remainder = ud % divisor;
     
      *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    }
  while (ud /= divisor);
     
  /* Terminate BUF. */
  *p = 0;
     
  /* Reverse BUF. */
  p1 = buf;
  p2 = p - 1;
  while (p1 < p2)
    {
      char tmp = *p1;
      *p1 = *p2;
      *p2 = tmp;
      p1++;
      p2--;
    }
}


void init( multiboot* pmb ) {
   int i;
   memory_map_t *mmap;
   unsigned int memsz = 0;		/* Memory size in MB */
   static char memstr[10];

  for (mmap = (memory_map_t *) pmb->mmap_addr;
       (unsigned long) mmap < pmb->mmap_addr + pmb->mmap_length;
       mmap = (memory_map_t *) ((unsigned long) mmap
				+ mmap->size + 4 /*sizeof (mmap->size)*/)) {
    
    if (mmap->type == 1)	/* Available RAM -- see 'info multiboot' */
      memsz += mmap->length_low;
  }

  /* Convert memsz to MBs */
  memsz = (memsz >> 20) + 1;	/* The + 1 accounts for rounding
				   errors to the nearest MB that are
				   in the machine, because some of the
				   memory is othrwise allocated to
				   multiboot data structures, the
				   kernel image, or is reserved (e.g.,
				   for the BIOS). This guarantees we
				   see the same memory output as
				   specified to QEMU.
				    */

  itoa(memstr, 'd', memsz);

  terminal_initialize();

  terminal_writestring("FIFOS-2: Welcome *** System memory is: ");
  terminal_writestring(memstr);
  terminal_writestring("MB\n");

  init_idt();

  init_pic();

  init_pit();

  for (i=0; i<N; ++i) {
    thread_tcbs[i].tid = i+1;
    thread_tcbs[i].state = UNUSED;
  }

  thread_create(thread_1, &(stack1[STACK_N-1]));
  terminal_writestring("Thread<1> created\n");
  thread_create(thread_2, &(stack2[STACK_N-1]));
  terminal_writestring("Thread<2> created\n");
  thread_create(thread_3, &(stack3[STACK_N-1]));
  terminal_writestring("Thread<3> created\n");

  /* thread_schedule(); */
 
  __asm__ volatile ("sti");
  while (1);
}

/* ---------------------------------------------------------------------------------------------------------------------- */

void thread_sleep(int ticks)
{
  while(--ticks >= 0);
}

void thread_create(thread_entry func, void* stack)
{
  int i;
  struct tcb* tcb;
  uint32_t* istack = (uint32_t*)stack;
  for (i=0; i<N; ++i) {
    if (thread_tcbs[i].state == UNUSED) {
      break;
    }
  }

  if (i == N) {
    return;
  }

  tcb = &(thread_tcbs[i]);
  tcb->bp = istack;
  tcb->state = RUNNABLE;

  /* EIP */
  *(istack--) = (uint32_t)func;
  /* EFLAGS */
  *(istack--) = 2;
  /* EAX */
  *(istack--) = 0;
  /* ECX */
  *(istack--) = 0;
  /* EDX */
  *(istack--) = 0;
  /* EBX */
  *(istack--) = 0;
  /* ESP */
  *(istack--) = 0;
  /* EBP */
  *(istack--) = 0;
  /* ESI */
  *(istack--) = 0;
  /* EDI */
  *(istack--) = 0;

  /* es */
  *((uint16_t*)istack) = 0x10;
  /* ds */
  *(((uint16_t*)istack) - 1) = 0x10;
  /* ss */
  *(((uint16_t*)istack) - 2) = 0x10;
  /* fs */
  *(((uint16_t*)istack) - 3) = 0x10;
   /* gs */
  *(((uint16_t*)istack) - 4) = 0x10;
  *(((uint16_t*)istack) - 5) = 0x10;

  tcb->sp = istack - 2;
}

void thread_schedule()
{
  int i;
  struct tcb* next_tcb;
  __asm__ volatile("cli");
  int next_tcb_index = curr_tcb_index;
  struct tcb* curr_tcb = curr_tcb_index < 0 ? 0 : &(thread_tcbs[curr_tcb_index]);
  for (i=0; i<N; ++i) {
    next_tcb_index = (next_tcb_index + 1) % (N);
    next_tcb = &(thread_tcbs[next_tcb_index]);
    if (next_tcb->state == RUNNABLE) {
      break;
    }
  }

  if (next_tcb->state != RUNNABLE) {
    terminal_writestring("\nall done!!!!!");
    while (1);
  }

  // /* switch */
  // {
  //   char buf[64];
  //   terminal_writestring("Schedule thread<");
  //   itoa(buf, 10, next_tcb_index);
  //   terminal_writestring(buf);
  //   terminal_writestring(">\n");
  // }
  
  curr_tcb_index = next_tcb_index;
  __asm__ volatile ("call switch"::"S" (curr_tcb), "D" (next_tcb));
}

void thread_yield()
{
  thread_schedule();
}

void thread_1()
{
  int i;
  for (i=0; i<THREAD_LOOP_N; i++) {
    __asm__ volatile ("cli");
    terminal_writestring("<1> ");
    __asm__ volatile ("sti");
    thread_sleep(THTREAD_SLEEP_TICKS);
    /* thread_yield(); */
  }
  __asm__ volatile ("cli");
  terminal_writestring("\nDone <1>");
  thread_tcbs[curr_tcb_index].state = DONE;
  __asm__ volatile ("sti");
  thread_sleep(THTREAD_SLEEP_TICKS);
  thread_yield();
}

void thread_2()
{
  int i;
  for (i=0; i<THREAD_LOOP_N; i++) {
    __asm__ volatile ("cli");
    terminal_writestring("<2> ");
    __asm__ volatile ("sti");
    thread_sleep(THTREAD_SLEEP_TICKS);
    /* thread_yield(); */
  }
  __asm__ volatile ("cli");
  terminal_writestring("\nDone <2>");
  thread_tcbs[curr_tcb_index].state = DONE;
  __asm__ volatile ("sti");
  thread_sleep(THTREAD_SLEEP_TICKS);
  thread_yield();
}

void thread_3()
{
  int i;
  for (i=0; i<THREAD_LOOP_N; i++) {
    __asm__ volatile ("cli");
    terminal_writestring("<3> ");
    __asm__ volatile ("sti");
    thread_sleep(THTREAD_SLEEP_TICKS);
    /* thread_yield(); */
  }
  __asm__ volatile ("cli");
  terminal_writestring("\nDone <3>");
  thread_tcbs[curr_tcb_index].state = DONE;
  __asm__ volatile ("sti");
  thread_sleep(THTREAD_SLEEP_TICKS);
  thread_yield();
}

