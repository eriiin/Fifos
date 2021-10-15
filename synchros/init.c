#include "multiboot.h"
#include "types.h"
#include "interrupt.h"


#define STACK_N 1024
#define THREAD_LOOP_N 3
#define THTREAD_SLEEP_TICKS 100000000

#define PRODUCER_N 2
#define CONSUMER_N 2
#define PRODUCT_N 100
#define PRODUCT_SLOT_N 50
#define N (PRODUCER_N + CONSUMER_N)

typedef struct {
  int producer_tid;
  int message_id;
  char message[32];
} message_t;

int in = 0;
int out = 0;
int counter = 0;
// lock_t lock;
// condition_t full_cond;
// condition_t empty_cond;
message_t message_slots[PRODUCT_SLOT_N];


void mutex_lock()
{
  __asm__ volatile ("cli");
}

void mutex_unlock()
{
  __asm__ volatile ("sti");
}


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

static uint32_t stacks[N][STACK_N];

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

void thread_producer();
void thread_consumer();


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

void strcpy(char* dst, const char* src)
{
  while (*src) {
    *dst++ = *src++;
  }

  *dst = 0;
}

void strcat(char* dst, const char* src)
{
  while (*dst++);
  dst--;
  while (*src) {
    *dst++ = *src++;
  }

  *dst++ = 0;
}

const char* strrchr(const char* str, char ch)
{
  size_t l = strlen(str);
  const char* p = str + l - 1;
  while (*p-- != ch);

  return ++p;
}

int strcmp(const char* s1, const char* s2)
{
  const char* p;
  const char* q;
  if (strlen(s1) != strlen(s2)) {
    return 1;
  }

  p = s1;
  q = s2;
  while (*p  && *q) {
    if (*p++ != *q++) {
      return 1;
    }
  }

  return 0;
}


 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 24;
 
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;


void terminal_clear_row(int row)
{

  for (size_t y = row; y<VGA_HEIGHT; ++y) {
    for (size_t x=0; x<VGA_WIDTH; ++x) {
      terminal_buffer[y * VGA_WIDTH + x] = make_vgaentry(' ', terminal_color);
    }
  }

}
 
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
    if (++terminal_row == VGA_HEIGHT) {
      terminal_row = 0;
    }
    terminal_clear_row(terminal_row);
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

    terminal_clear_row(terminal_row);
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

  terminal_writestring("SYNCHROS: Welcome *** System memory is: ");
  terminal_writestring(memstr);
  terminal_writestring("MB\n");

  init_idt();

  init_pic();

  init_pit();

  for (i=0; i<N; ++i) {
    thread_tcbs[i].tid = i+1;
    thread_tcbs[i].state = UNUSED;
  }

  /* init producer threads */
  for (i=0; i<PRODUCER_N; ++i) {
    thread_create(thread_producer, &(stacks[i][STACK_N-1]));
    terminal_writestring("Producer Thread created\n");
  }

  /* init consumer threads */
  for (; i<N; ++i) {
    thread_create(thread_consumer, &(stacks[i][STACK_N -1]));
    terminal_writestring("Consumer Thread created\n");
  }

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
    terminal_writestring("all done!!!!!\n");
    while (1);
  }

  /* switch */
  // {
  //   char buf[64];
  //   terminal_writestring("Schedule thread<");
  //   itoa(buf, 'd', next_tcb_index);
  //   terminal_writestring(buf);
  //   terminal_writestring("> ");
  // }
  
  curr_tcb_index = next_tcb_index;
  __asm__ volatile ("call switch"::"S" (curr_tcb), "D" (next_tcb));
}

void thread_yield()
{
  thread_schedule();
}

void thread_producer()
{
  int i;
  char buf[64];
  message_t* message;
  struct tcb* curr_tcb;
  int next_consumer_thread_index = PRODUCER_N;
  for (i=0; i<PRODUCT_N; ++i) {
    while (1) {
      mutex_lock();
      if (counter < PRODUCT_SLOT_N) {    
        message = &(message_slots[in]);
        curr_tcb = &thread_tcbs[curr_tcb_index];
        message->producer_tid = curr_tcb->tid;
        message->message_id = i;
        strcpy(message->message, "Msg ");
        itoa(buf, 'd', i);
        strcat(message->message, buf);
        strcat(message->message, " for ");
        itoa(buf, 'd', thread_tcbs[next_consumer_thread_index].tid);
        strcat(message->message, buf);
        next_consumer_thread_index = (next_consumer_thread_index + 1) % N;
        if (next_consumer_thread_index == 0) {
          next_consumer_thread_index = PRODUCER_N;
        }

        // terminal_writestring(message->message);
        // terminal_writestring("\n");
        
        counter += 1;
        in = (in + 1) % PRODUCT_SLOT_N;
        // terminal_writestring("produce one slot\n");
        mutex_unlock();
        thread_sleep(10000000);
        break;
      } else {
        // terminal_writestring("slots full\n");
        mutex_unlock();
        thread_sleep(10000000);
      }
    }
  }

  thread_tcbs[curr_tcb_index].state = DONE;
  thread_yield();
}


void thread_consumer()
{
  int i;
  char buf[64];
  const char* p;
  message_t* message;
  struct tcb* curr_tcb;
  for (i=0; i<PRODUCT_N; ++i) {
    while (1) {
      mutex_lock();
      if (counter > 0) {
        message = &(message_slots[out]);
        curr_tcb = &thread_tcbs[curr_tcb_index];
        itoa(buf, 'd', curr_tcb->tid);
        p = strrchr(message->message, ' ');
        // terminal_writestring(buf);
        // terminal_writestring(p+1);

        if (p && strcmp(buf, p+1) == 0) {
          /* message for me */
          counter -= 1;
          out = (out + 1) % PRODUCT_SLOT_N;
          itoa(buf, 'd', curr_tcb->tid);
          strcat(buf, ": ");
          terminal_writestring(buf);
          itoa(buf, 'd', message->producer_tid);
          strcat(buf, ": message: ");
          terminal_writestring(buf);
          itoa(buf, 'd', message->message_id);
          strcat(buf, ": ");
          strcat(buf, message->message);
          strcat(buf, "\n");
          terminal_writestring(buf);
          mutex_unlock();
          thread_sleep(20000000);
          break;
        } else {
          mutex_unlock();
          thread_sleep(100000);
        }
      } else {
        mutex_unlock();
        thread_sleep(100000);
      }
    }
  }

  thread_tcbs[curr_tcb_index].state = DONE;
  thread_yield();
}
