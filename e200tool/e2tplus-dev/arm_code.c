
/*
 * This is the USB stub which will run on the e200. You do
 * not need to compile this yourself. A pre-compiled version
 * should be available in e200_code.c.
 *
 * In case you do not trust me and wish to compile it yourself,
 * make sure that the result is relocated to 0x40004000 and the
 * entrypoint is in the beginning of the file. I did it with
 *
 *     arm-elf-gcc -c -Os -fno-unit-at-a-time -mcpu=arm7tdmi arm_code.c
 *     arm-elf-ld -Ttext 0x40004000 -N arm_code.o
 *     arm-elf-objcopy -Obinary a.out arm_code.bin
 *     bin2c arm_code.bin e200_code
 *     
 * The arm-elf package can be downloaded by running rockboxdev.sh in
 * tools directory of rockbox package. Also, bin2c can be found in 
 * rbutil/sansapatcher directory.
 *
 * I'm sure that there is some better way, feel free to use
 * whatever works for you.
 *
 * Note that my primary concern with this code was to keep
 * it small and simple, not to handle every possible situation
 * properly. The code does not have any real buffer handling,
 * uses only polling, uses only the default control endpoint,
 * does not do real sanity checks and does not even implement
 * all the standard requests.
 *
 * I.e. this is NOT meant to be a good example for a generic
 * USB driver.
 *
 * Original file (c) MrH 2006, 2007
 * Extended by Virtuoso015 - 2008
 */

asm (".global _start   \n"
     "_start: b __init \n"); /* Has to be at the very beginning */

void _start();

/*
 * Init CPU and COP to known state.
 */

asm ("__init:                   \n"

     /* Make sure we run at right address */

     "adr   r0, _start          \n"
     "ldr   r1, =_start         \n"
     "ldr   r2, =__bss_start__  \n"

     "1:                        \n"
     "cmp   r1, r2              \n"
     "ldrlo r3, [r0], #4        \n"
     "strlo r3, [r1], #4        \n"
     "blo   1b                  \n"

     "ldr   r0, =__setup        \n"
     "bx    r0                  \n"
     "__setup:                  \n"

     /* Get processor id */

     "mov   r0, #0x60000000     \n"
     "ldrb  r4, [r0]            \n"

     /* Setup stacks */

     "cmp   r4, #0x55           \n"
     "ldreq r5, =cpu_stack_end  \n"
     "ldrne r5, =cop_stack_end  \n"
     "ldr   sp, [r5]            \n"

     /* Halt COP */

     "ldr   r6, =0x60007004     \n"     
     "cmp   r4, #0x55           \n"
     "ldrne r1, [r6]            \n"
     "orrne r1, r1, #0x80000000 \n"
     "strne r1, [r6]            \n"
     "bne   2f                  \n"

     /* Wait for COP to be halted */

     "1:                        \n"
     "ldr   r0, [r6]            \n"
     "tst   r0, #0x80000000     \n"
     "beq   1b                  \n"

     /* Clear BSS */

     "mov   r0, #0              \n"
     "ldr   r1, =__bss_start__  \n"
     "ldr   r2, =__bss_end__    \n"
     "1:                        \n"
     "cmp   r1, r2              \n"
     "strlo r0, [r1], #4        \n"
     "blo   1b                  \n"

     /* Call main() */

     "bl    main                \n"

     /* Start COP */

     "ldr   r0, [r6]            \n"
     "bic   r0, r0, #0x80000000 \n"
     "str   r0, [r6]            \n"

     /* Go! */

     "2:                        \n"
     "adr   lr, __init          \n"
     "ldr   r4, =cur_addr       \n"
     "ldr   pc, [r4]            \n"

     ".ltorg                    \n");

/*
 * Some hardware specific stuff.
 */

#define CPU_ID (*(volatile unsigned char *)0x60000000)
#define CPU 0x55
#define COP 0xaa

#define CPU_SLEEP (*(volatile int *)0x60007000)
#define COP_SLEEP (*(volatile int *)0x60007004)

#define GPIOG_OUTPUT_VAL (*(volatile int *)0x6000d0a8)

#define USEC_TIME (*(volatile int *)0x60005010)

#define STACK_SIZE 1024

unsigned char cpu_stack[STACK_SIZE];
unsigned char cop_stack[STACK_SIZE];

static unsigned char * const cpu_stack_end = cpu_stack + STACK_SIZE;
static unsigned char * const cop_stack_end = cop_stack + STACK_SIZE;

char * cur_addr;

#define TMPBUF_SIZE 64  	// matches the value of BUFSIZE(size of each packet) in e200tool.c 

char tmpbuf[TMPBUF_SIZE];	// to hold packet data sent from e200tool

/*
 * I don't want to depend on any library, so let's implement
 * some generic utility routines. Some of these try to mimic
 * standard functions, but there may be some differences.
 */

typedef unsigned int u_int;

#define NULL ((void *)0)

#define MIN(x, y) ((x) < (y) ? (x) : (y))

struct timer {
  unsigned long s;
  unsigned long e;
};

void
timer_set(struct timer * timer, unsigned long val)
{
  timer->s = USEC_TIME;
  timer->e = timer->s + val + 1;
}

int
timer_expired(struct timer * timer)
{
  unsigned long val = USEC_TIME;

  if (timer->e > timer->s) {
    return !(val >= timer->s && val <= timer->e);
  } else {
    return (val > timer->e && val < timer->s);
  }
}

typedef struct {
  unsigned int r[16];
} jmp_buf[1];

jmp_buf start_env;
jmp_buf reset_env;

int setjmp(jmp_buf) __attribute((naked));
int longjmp(jmp_buf, int) __attribute((naked));

int
setjmp(jmp_buf env)
{
  asm volatile ("stmia r0, {r0-r15} \n"
		"mov   r0, #0       \n"
		"nop                \n"
		"mov   pc, lr       \n");

  return 0; /* not reached */
}

int
longjmp(jmp_buf env, int val)
{
  asm volatile ("cmp   r1, #0       \n"
		"moveq r1, #1       \n"
		"str   r1, [r0]     \n"
		"ldmia r0, {r0-r15} \n");

  return 0; /* not reached */
}

void
usleep(unsigned long t)
{
  struct timer timer;

  timer_set(&timer, t);

  while (!timer_expired(&timer));
}

void *
memcpy(void * to, const void * from, int count)
{
  unsigned char * __to = to;
  const unsigned char * __from = from;;

  while (count--) {
    *__to++ = *__from++;
  }

  return to;
}

void *
memset(void * ptr, int val, int count)
{
  char * p = ptr;

  while (count--) {
    *p++ = val;
  }
  
  return ptr;
}
/****************************************************
void
cache_flush()
{
  *(volatile int*)0xf000f044 |= 2;

  while ((*(volatile int *)0x6000c000) & 0x8000);
}
*****************************************************/
void
led_set(int on)
{
  if (on) {
    GPIOG_OUTPUT_VAL |= 0x80;
  } else {
    GPIOG_OUTPUT_VAL &= ~0x80;
  }
}

void
blink(int delay)
{
  led_set(0);
  usleep(delay / 2);
  led_set(1);
  usleep(delay / 2);
}
/********************************
void
halt()
{
  for(;;) {
    COP_SLEEP |= (1 << 31);
    CPU_SLEEP |= (1 << 31);
  }
}
*********************************/
void restart();

void
exit(int err)
{
  err = -err;

  while (err--) {
    blink(500000);
  }

  /* Try to restart */

  for (;;) {
    restart();
  }
}

#define ERROR_IO       (-2)
#define ERROR_TIMEOUT  (-3)
#define ERROR_PROTOCOL (-4)
#define ERROR_INVALID  (-5)
#define ERROR_HW       (-6)
#define ERROR_UNKNOWN  (-7)

/*
 * The i2c interface.
 */

#define DEV_RESET  (*(volatile u_int *)0x60006004)
#define DEV_ENABLE (*(volatile u_int *)0x6000600c)

#define I2C_CONTROL (*(volatile u_int *)0x7000c000)
#define I2C_ADDRESS (*(volatile u_int *)0x7000c004)
#define I2C_DATA    (*(volatile u_int *)0x7000c00c)
#define I2C_STATUS  (*(volatile u_int *)0x7000c01c)

#define I2C_READ  0x20
#define I2C_START 0x80

#define I2C_BUS_BUSY (I2C_STATUS & (1 << 6))

#define I2C_TIMER 1000000

void
i2c_init()
{
  DEV_ENABLE |= (1 << 12);

  DEV_RESET |=  (1 << 12);
  DEV_RESET &= ~(1 << 12);
  
  // copied from rockbox i2c-pp.c, doesn't seem to
  // correct the "works almost always" phenomenon.
   
  (*(volatile u_int *)0x600060a4) = 0x0;
  (*(volatile u_int *)0x600060a4) = 0x23;
}

int
i2c_busy()
{
  struct timer t;

  timer_set(&t, I2C_TIMER);

  while (I2C_BUS_BUSY) {
    if (timer_expired(&t)) {
      return ERROR_TIMEOUT;
    }
  }

  return 0;
}

void
i2c_set_size(int size)
{
  I2C_CONTROL &= ~(3 << 1);
  I2C_CONTROL |= (size - 1) << 1;
}

int
i2c_read(int addr, void * buf, int size)
{
  unsigned char * ptr = buf;
  int todo, done = 0;
  u_int data;

  if (addr > 127) {
    return ERROR_INVALID;
  }

  addr = (addr << 1) | 1;

  if (i2c_busy()) {
    goto out;
  }

  while (size > 0) {
    I2C_ADDRESS = addr;
    I2C_CONTROL = 0;

    todo = MIN(4, size);

    i2c_set_size(todo);

    I2C_CONTROL |= I2C_START;

    if (i2c_busy()) {
      goto out;
    }

    if (I2C_STATUS & 7) {
      goto out;
    }

    data = I2C_DATA;

    while (todo > 0) {
      *ptr = data;
      data = data >> 8;

      size -= 1;
      todo -= 1;
      done += 1;
      ptr  += 1;
    }
  }

 out:

  return done;
}

int
i2c_write(int addr, void * buf, int size)
{
  unsigned char * ptr = buf;
  int i, todo, done = 0;
  u_int data;

  if (addr > 127) {
    return ERROR_INVALID;
  }

  addr = (addr << 1);

  if (i2c_busy()) {
    goto out;
  }

  while (size > 0) {
    I2C_ADDRESS = addr;

    todo = MIN(4, size);

    i2c_set_size(todo);

    data = 0;

    for (i = 0; i < todo; i++) {
      data |= (u_int)(*ptr++) << (i * 8);
    }

    I2C_DATA = data;

    I2C_CONTROL |= I2C_START;

    if (i2c_busy()) {
      goto out;
    }

    if (I2C_STATUS & 7) {
      goto out;
    }

    size -= todo;
    done += todo;
  }

 out:

  return done;
}

#define I2C_PROGRAM_TIMER 5000000

int
i2c_program(int addr, void * buf, int size)
{
  char * ptr = buf;
  unsigned int pos;
  unsigned char tmp[4];
  int todo, ret = 0;
  struct timer t;

  pos = 0;

  while (size > 0) {
    todo = MIN(size, 2);

    tmp[0] = pos >> 8;
    tmp[1] = pos;
    tmp[2] = ptr[0];
    if (todo > 1) {
      tmp[3] = ptr[1];
    }

    led_set((pos >> 5) & 1);

    ret = i2c_write(addr, tmp, todo + 2);

    if (ret != todo + 2) {
      ret = ERROR_HW;
      goto out;
    }

    /* Wait for program cycle to complete */

    timer_set(&t, I2C_PROGRAM_TIMER);

    ptr  += todo;
    pos  += todo;
    size -= todo;

    tmp[0] = pos >> 8;
    tmp[1] = pos;

    do {
      ret = i2c_write(addr, tmp, 2);
    } while (ret != 2 && !timer_expired(&t));

    if (timer_expired(&t)) {
      ret = ERROR_TIMEOUT;
      goto out;
    }
  }

 out:
  led_set(1);
  
  return ret;
}

void
poweroff()
{
  unsigned char tmp[2];

  tmp[0] = 0x20;

  i2c_write(70, tmp, 1);
  i2c_read(70, &tmp[1], 1);
  tmp[1] &= ~1;
  i2c_write(70, tmp, 2);
}

/*
 * Extension by Virtuoso015
 * The NAND interface.
 * All info gleaned or copied from Rockbox file,
 * /rockbox/firmware/target/arm/ata-sd-pp.c
 */
#define DEV_ATA         0x00004000

#define GPO32_VAL        (*(volatile unsigned long *)(0x70000080))
#define GPO32_ENABLE     (*(volatile unsigned long *)(0x70000084))
#define GPIOA_ENABLE     (*(volatile unsigned long *)(0x6000d000))
#define GPIOD_ENABLE     (*(volatile unsigned long *)(0x6000d00c))
#define GPIOA_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d010))
#define GPIOD_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d01c))
#define GPIOA_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d020))
#define GPIOD_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d02c))
#define GPIOG_ENABLE     (*(volatile unsigned long *)(0x6000d088))
#define GPIOG_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d098))

#define BLOCK_SIZE      512
#define SECTOR_SIZE     512
#define BLOCKS_PER_BANK 0x7a7800

#define STATUS_REG      (*(volatile unsigned int *)(0x70008204))
#define REG_1           (*(volatile unsigned int *)(0x70008208))
#define UNKNOWN         (*(volatile unsigned int *)(0x70008210))
#define BLOCK_SIZE_REG  (*(volatile unsigned int *)(0x7000821c))
#define BLOCK_COUNT_REG (*(volatile unsigned int *)(0x70008220))
#define REG_5           (*(volatile unsigned int *)(0x70008224))
#define CMD_REG0        (*(volatile unsigned int *)(0x70008228))
#define CMD_REG1        (*(volatile unsigned int *)(0x7000822c))
#define CMD_REG2        (*(volatile unsigned int *)(0x70008230))
#define RESPONSE_REG    (*(volatile unsigned int *)(0x70008234))
#define SD_STATE_REG    (*(volatile unsigned int *)(0x70008238))
#define REG_11          (*(volatile unsigned int *)(0x70008240))
#define REG_12          (*(volatile unsigned int *)(0x70008244))
#define DATA_REG        (*(volatile unsigned int *)(0x70008280))

/* STATUS_REG bits */
#define DATA_DONE       (1 << 12)
#define CMD_DONE        (1 << 13)
#define ERROR_BITS      (0x3f)
#define READY_FOR_DATA  (1 << 8)
#define FIFO_FULL       (1 << 7)
#define FIFO_EMPTY      (1 << 6)

#define CMD_OK          0x0 /* Command was successful */
#define CMD_ERROR_2     0x2 /* SD did not respond to command (either it doesn't
                               understand the command or is not inserted) */

/* SD States */
#define IDLE            0
#define READY           1
#define IDENT           2
#define STBY            3
#define TRAN            4
#define DATA            5
#define RCV             6
#define PRG             7
#define DIS             8

#define FIFO_LEN        16          /* FIFO is 16 words deep */

/* SD Commands */
#define GO_IDLE_STATE         0
#define ALL_SEND_CID          2
#define SEND_RELATIVE_ADDR    3
#define SET_DSR               4
#define SWITCH_FUNC           6
#define SELECT_CARD           7
#define DESELECT_CARD         7
#define SEND_IF_COND          8
#define SEND_CSD              9
#define SEND_CID             10
#define STOP_TRANSMISSION    12
#define SEND_STATUS          13
#define GO_INACTIVE_STATE    15
#define SET_BLOCKLEN         16
#define READ_SINGLE_BLOCK    17
#define READ_MULTIPLE_BLOCK  18
#define SEND_NUM_WR_BLOCKS   22
#define WRITE_BLOCK          24
#define WRITE_MULTIPLE_BLOCK 25
#define ERASE_WR_BLK_START   32
#define ERASE_WR_BLK_END     33
#define ERASE                38
#define APP_CMD              55

#define EC_OK                    0
#define EC_FAILED                1
#define EC_NOCARD                2
#define EC_WAIT_STATE_FAILED     3
#define EC_CHECK_TIMEOUT_FAILED  4
#define EC_POWER_UP              5
#define EC_READ_TIMEOUT          6
#define EC_WRITE_TIMEOUT         7
#define EC_TRAN_SEL_BANK         8
#define EC_TRAN_READ_ENTRY       9
#define EC_TRAN_READ_EXIT       10
#define EC_TRAN_WRITE_ENTRY     11
#define EC_TRAN_WRITE_EXIT      12
#define EC_FIFO_SEL_BANK_EMPTY  13
#define EC_FIFO_SEL_BANK_DONE   14
#define EC_FIFO_ENA_BANK_EMPTY  15
#define EC_FIFO_READ_FULL       16
#define EC_FIFO_WR_EMPTY        17
#define EC_FIFO_WR_DONE         18
#define EC_COMMAND              19
#define NUM_EC                  20

/* Application Specific commands */
#define SET_BUS_WIDTH   6
#define SD_APP_OP_COND  41

/* Miscellaneous definitions  */
#define NUM_VOLUMES 1
#define TRUE 1
#define FALSE 0
 
#define outl(a,b) (*(volatile unsigned long*)(b)=(a))
#define inl(a)    (*(volatile unsigned long*)(a))

#define GPIO_SET_BITWISE(port, mask) \
        do { *(&port + (0x800/sizeof(long))) = (mask << 8) | mask; } while(0)

#define GPIO_CLEAR_BITWISE(port, mask) \
        do { *(&port + (0x800/sizeof(long))) = mask << 8; } while(0)


/** static, private data **/ 
static int initialized = FALSE;
unsigned int nand_addr; //contains nand_block_address and count

typedef struct
{
    int initialized;

    unsigned int ocr;            /* OCR register */
    unsigned int csd[4];         /* CSD register */
    unsigned int cid[4];         /* CID register */
    unsigned int rca;

    unsigned long long capacity; /* size in bytes */
    unsigned long numblocks;     /* size in flash blocks */
    unsigned int block_size;     /* block size in bytes */
    unsigned int max_read_bl_len;/* max read data block length */
    unsigned int block_exp;      /* block size exponent */
    unsigned char current_bank;  /* The bank that we are working with */
} tSDCardInfo;

static tSDCardInfo card_info, *nand_card= NULL;


struct sd_card_status
{
    int retry;
    int retry_max;
};

static struct sd_card_status sd_status = {
    0,
    1 
};

static int sd_poll_status(unsigned int trigger, long timeout)
{
   
    struct timer t;
    
    timer_set(&t,timeout);   // may need to increase timer

    while ((STATUS_REG & trigger) == 0)
    {
        
        // no need to switch "threads" here :P
  
        if (timer_expired(&t))
            return FALSE;
    }

    return TRUE;
}

static int sd_command(unsigned int cmd, unsigned long arg1,
                      unsigned int *response, unsigned int type)
{
    int i, words; /* Number of 16 bit words to read from RESPONSE_REG */
    unsigned int data[9];

    CMD_REG0 = cmd;
    CMD_REG1 = (unsigned int)((arg1 & 0xffff0000) >> 16);
    CMD_REG2 = (unsigned int)((arg1 & 0xffff));
    UNKNOWN  = type;

    if (!sd_poll_status(CMD_DONE, 100000))
        return -EC_COMMAND;

    if ((STATUS_REG & ERROR_BITS) != CMD_OK)
        /* Error sending command */
        return -EC_COMMAND - (STATUS_REG & ERROR_BITS)*100;

    if (cmd == GO_IDLE_STATE)
        return 0; /* no response here */

    words = (type == 2) ? 9 : 3;

    for (i = 0; i < words; i++) /* RESPONSE_REG is read MSB first */
        data[i] = RESPONSE_REG; /* Read most significant 16-bit word */

    if (response == NULL)
    {
        /* response discarded */
    }
    else if (type == 2)
    {
        response[3] = (data[0]<<24) + (data[1]<<8) + (data[2]>>8);
        response[2] = (data[2]<<24) + (data[3]<<8) + (data[4]>>8);
        response[1] = (data[4]<<24) + (data[5]<<8) + (data[6]>>8);
        response[0] = (data[6]<<24) + (data[7]<<8) + (data[8]>>8);
    }
    else
    {
       response[0] = (data[0]<<24) + (data[1]<<8) + (data[2]>>8);
    }

    return 0;
}

static int sd_wait_for_state(unsigned int state, int id)
{
    unsigned int response = 0;
    unsigned int timeout = 0x80000;   // may need to increase timeout
    struct timer t;
    
    timer_set(&t,timeout);

    //check_time[id] = USEC_TIME;

    while (1)
    {
        int ret = sd_command(SEND_STATUS, nand_card->rca, &response, 1);
        //long us;

        if (ret < 0)
            return ret*100 - id;

        if (((response >> 9) & 0xf) == state)
        {
            SD_STATE_REG = state;
            return 0;
        }

        //if (!sd_check_timeout(timeout, id)) // returns FALSE on timeout
		if (timer_expired(&t))        
            return -EC_WAIT_STATE_FAILED*100 - id;

    }
}

static inline void copy_read_sectors_fast(unsigned char **buf)
{
    /* Copy one chunk of 16 words using best method for start alignment */
    switch ( (long)*buf & 3 )
    {
    case 0:
        asm volatile (
            "ldmia  %[data], { r2-r9 }          \r\n"
            "orr    r2, r2, r3, lsl #16         \r\n"
            "orr    r4, r4, r5, lsl #16         \r\n"
            "orr    r6, r6, r7, lsl #16         \r\n"
            "orr    r8, r8, r9, lsl #16         \r\n"
            "stmia  %[buf]!, { r2, r4, r6, r8 } \r\n"
            "ldmia  %[data], { r2-r9 }          \r\n"
            "orr    r2, r2, r3, lsl #16         \r\n"
            "orr    r4, r4, r5, lsl #16         \r\n"
            "orr    r6, r6, r7, lsl #16         \r\n"
            "orr    r8, r8, r9, lsl #16         \r\n"
            "stmia  %[buf]!, { r2, r4, r6, r8 } \r\n"
            : [buf]"+&r"(*buf)
            : [data]"r"(&DATA_REG)
            : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9"
        );
        break;
    case 1:
        asm volatile (
            "ldmia  %[data], { r2-r9 }          \r\n"
            "orr    r3, r2, r3, lsl #16         \r\n"
            "strb   r3, [%[buf]], #1            \r\n"
            "mov    r3, r3, lsr #8              \r\n"
            "strh   r3, [%[buf]], #2            \r\n"
            "mov    r3, r3, lsr #16             \r\n"
            "orr    r3, r3, r4, lsl #8          \r\n"
            "orr    r3, r3, r5, lsl #24         \r\n"
            "mov    r5, r5, lsr #8              \r\n"
            "orr    r5, r5, r6, lsl #8          \r\n"
            "orr    r5, r5, r7, lsl #24         \r\n"
            "mov    r7, r7, lsr #8              \r\n"
            "orr    r7, r7, r8, lsl #8          \r\n"
            "orr    r7, r7, r9, lsl #24         \r\n"
            "mov    r2, r9, lsr #8              \r\n"
            "stmia  %[buf]!, { r3, r5, r7 }     \r\n"
            "ldmia  %[data], { r3-r10 }         \r\n"
            "orr    r2, r2, r3, lsl #8          \r\n"
            "orr    r2, r2, r4, lsl #24         \r\n"
            "mov    r4, r4, lsr #8              \r\n"
            "orr    r4, r4, r5, lsl #8          \r\n"
            "orr    r4, r4, r6, lsl #24         \r\n"
            "mov    r6, r6, lsr #8              \r\n"
            "orr    r6, r6, r7, lsl #8          \r\n"
            "orr    r6, r6, r8, lsl #24         \r\n"
            "mov    r8, r8, lsr #8              \r\n"
            "orr    r8, r8, r9, lsl #8          \r\n"
            "orr    r8, r8, r10, lsl #24        \r\n"
            "mov    r10, r10, lsr #8            \r\n"
            "stmia  %[buf]!, { r2, r4, r6, r8 } \r\n"
            "strb   r10, [%[buf]], #1           \r\n"
            : [buf]"+&r"(*buf)
            : [data]"r"(&DATA_REG)
            : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"
        );
        break;
    case 2:
        asm volatile (
            "ldmia  %[data], { r2-r9 }          \r\n"
            "strh   r2, [%[buf]], #2            \r\n"
            "orr    r3, r3, r4, lsl #16         \r\n"
            "orr    r5, r5, r6, lsl #16         \r\n"
            "orr    r7, r7, r8, lsl #16         \r\n"
            "stmia  %[buf]!, { r3, r5, r7 }     \r\n"
            "ldmia  %[data], { r2-r8, r10 }     \r\n"
            "orr    r2, r9, r2, lsl #16         \r\n"
            "orr    r3, r3, r4, lsl #16         \r\n"
            "orr    r5, r5, r6, lsl #16         \r\n"
            "orr    r7, r7, r8, lsl #16         \r\n"
            "stmia  %[buf]!, { r2, r3, r5, r7 } \r\n"
            "strh   r10, [%[buf]], #2           \r\n"
            : [buf]"+&r"(*buf)
            : [data]"r"(&DATA_REG)
            : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"
        );
        break;
    case 3:
        asm volatile (
            "ldmia  %[data], { r2-r9 }          \r\n"
            "orr    r3, r2, r3, lsl #16         \r\n"
            "strb   r3, [%[buf]], #1            \r\n"
            "mov    r3, r3, lsr #8              \r\n"
            "orr    r3, r3, r4, lsl #24         \r\n"
            "mov    r4, r4, lsr #8              \r\n"
            "orr    r5, r4, r5, lsl #8          \r\n"
            "orr    r5, r5, r6, lsl #24         \r\n"
            "mov    r6, r6, lsr #8              \r\n"
            "orr    r7, r6, r7, lsl #8          \r\n"
            "orr    r7, r7, r8, lsl #24         \r\n"
            "mov    r8, r8, lsr #8              \r\n"
            "orr    r2, r8, r9, lsl #8          \r\n"
            "stmia  %[buf]!, { r3, r5, r7 }     \r\n"
            "ldmia  %[data], { r3-r10 }         \r\n"
            "orr    r2, r2, r3, lsl #24         \r\n"
            "mov    r3, r3, lsr #8              \r\n"
            "orr    r4, r3, r4, lsl #8          \r\n"
            "orr    r4, r4, r5, lsl #24         \r\n"
            "mov    r5, r5, lsr #8              \r\n"
            "orr    r6, r5, r6, lsl #8          \r\n"
            "orr    r6, r6, r7, lsl #24         \r\n"
            "mov    r7, r7, lsr #8              \r\n"
            "orr    r8, r7, r8, lsl #8          \r\n"
            "orr    r8, r8, r9, lsl #24         \r\n"
            "mov    r9, r9, lsr #8              \r\n"
            "orr    r10, r9, r10, lsl #8        \r\n"
            "stmia  %[buf]!, { r2, r4, r6, r8 } \r\n"
            "strh   r10, [%[buf]], #2           \r\n"
            "mov    r10, r10, lsr #16           \r\n"
            "strb   r10, [%[buf]], #1           \r\n"
            : [buf]"+&r"(*buf)
            : [data]"r"(&DATA_REG)
            : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"
        );
        break;
    }
}

static inline void copy_read_sectors_slow(unsigned char** buf)
{
    int cnt = FIFO_LEN;
    int t;

    /* Copy one chunk of 16 words */
    asm volatile (
    "1:                                     \r\n"
        "ldrh   %[t], [%[data]]             \r\n"
        "strb   %[t], [%[buf]], #1          \r\n"
        "mov    %[t], %[t], lsr #8          \r\n"
        "strb   %[t], [%[buf]], #1          \r\n"
        "subs   %[cnt], %[cnt], #1          \r\n"
        "bgt    1b                          \r\n"
        : [cnt]"+&r"(cnt), [buf]"+&r"(*buf),
          [t]"=&r"(t)
        : [data]"r"(&DATA_REG)
    );
}

/* Writes have to be kept slow for now */
static inline void copy_write_sectors(const unsigned char** buf)
{
    int cnt = FIFO_LEN;
    unsigned t;

    do
    {
        t  = *(*buf)++;
        t |= *(*buf)++ << 8;
        DATA_REG = t;
    } while (--cnt > 0); /* tail loop is faster */
}

static int sd_select_bank(unsigned char bank)
{
    unsigned char card_data[512];
    const unsigned char* write_buf;
    int i, ret;

    memset(card_data, 0, 512);

    ret = sd_wait_for_state(TRAN, EC_TRAN_SEL_BANK);
    if (ret < 0)
        return ret;

    BLOCK_SIZE_REG = 512;
    BLOCK_COUNT_REG = 1;

    ret = sd_command(35, 0, NULL, 0x1c0d); /* CMD35 is vendor specific */
    if (ret < 0)
        return ret;

    SD_STATE_REG = PRG;

    card_data[0] = bank;

    /* Write the card data */
    write_buf = card_data;
    for (i = 0; i < BLOCK_SIZE/2; i += FIFO_LEN)
    {
        /* Wait for the FIFO to empty */
        if (sd_poll_status(FIFO_EMPTY, 10000))
        {
            copy_write_sectors(&write_buf); /* Copy one chunk of 16 words */
            continue;
        }

        return -EC_FIFO_SEL_BANK_EMPTY;
    }

    if (!sd_poll_status(DATA_DONE, 10000))
        return -EC_FIFO_SEL_BANK_DONE;

    nand_card->current_bank = bank;

    return 0;
}

static void sd_card_mux(void)
{
/* Set the current card mux */

        GPO32_VAL |= 0x4;

        GPIO_CLEAR_BITWISE(GPIOA_ENABLE, 0x7a);
        GPIO_CLEAR_BITWISE(GPIOA_OUTPUT_EN, 0x7a);
        GPIO_SET_BITWISE(GPIOD_ENABLE,  0x1f);
        GPIO_SET_BITWISE(GPIOD_OUTPUT_VAL, 0x1f);
        GPIO_SET_BITWISE(GPIOD_OUTPUT_EN,  0x1f);

        outl((inl(0x70000014) & ~(0x3ffff)) | 0x255aa, 0x70000014);
}

static void sd_init_device(void)
{
/* SD Protocol registers */

    unsigned int  i;
    unsigned int  c_size;
    unsigned long c_mult;
    unsigned char carddata[512];
    unsigned char *dataptr;
    struct timer t;
    int ret;

/* Enable and initialise controller */
    REG_1 = 6;

/* Initialise card data as blank */
    memset(nand_card, 0, sizeof(*nand_card));

/* Switch card mux to card to initialize */
    sd_card_mux();

/* Init NAND */
    REG_11 |=  (1 << 15);
    REG_12 |=  (1 << 15);
    REG_12 &= ~(3 << 12);
    REG_12 |=  (1 << 13);
    REG_11 &= ~(3 << 12);
    REG_11 |=  (1 << 13);

    DEV_ENABLE |= DEV_ATA; /* Enable controller */
    DEV_RESET |= DEV_ATA; /* Reset controller */
    DEV_RESET &=~DEV_ATA; /* Clear Reset */

    SD_STATE_REG = TRAN;

    REG_5 = 0xf;

    ret = sd_command(GO_IDLE_STATE, 0, NULL, 256);
    if (ret < 0)
        goto card_init_error;

    
	  timer_set(&t,5000000);

    while ((nand_card->ocr & (1 << 31)) == 0) /* until card is powered up */
    {
        ret = sd_command(APP_CMD, nand_card->rca, NULL, 1);
        if (ret < 0)
            goto card_init_error;

        /* SD Standard */
        ret = sd_command(SD_APP_OP_COND, 0x100000, &nand_card->ocr, 3);
        if (ret < 0)
            goto card_init_error;

        
        if (timer_expired(&t))
        {
            ret = -EC_POWER_UP;
            goto card_init_error;
        }
    }

    ret = sd_command(ALL_SEND_CID, 0, nand_card->cid, 2);
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(SEND_RELATIVE_ADDR, 0, &nand_card->rca, 1);
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(SEND_CSD, nand_card->rca, nand_card->csd, 2);
    if (ret < 0)
        goto card_init_error;

    /* These calculations come from the Sandisk SD card product manual */
    if( (nand_card->csd[3]>>30) == 0)
    {
        /* CSD version 1.0 */
        c_size = ((nand_card->csd[2] & 0x3ff) << 2) + (nand_card->csd[1]>>30) + 1;
        c_mult = 4 << ((nand_card->csd[1] >> 15) & 7);
        nand_card->max_read_bl_len = 1 << ((nand_card->csd[2] >> 16) & 15);
        nand_card->block_size = BLOCK_SIZE;     /* Always use 512 byte blocks */
        nand_card->numblocks = c_size * c_mult * (nand_card->max_read_bl_len/512);
        nand_card->capacity = nand_card->numblocks * nand_card->block_size;
    }
    
    REG_1 = 0;

    ret = sd_command(SELECT_CARD, nand_card->rca, NULL, 129);
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(APP_CMD, nand_card->rca, NULL, 1);
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(SET_BUS_WIDTH, nand_card->rca | 2, NULL, 1); /* 4 bit */
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(SET_BLOCKLEN, nand_card->block_size, NULL, 1);
    if (ret < 0)
        goto card_init_error;

    BLOCK_SIZE_REG = nand_card->block_size;

    /* If this card is >4GB & not SDHC, then we need to enable bank switching */
    if( (nand_card->numblocks >= BLOCKS_PER_BANK) &&
        ((nand_card->ocr & (1<<30)) == 0) )
    {
        SD_STATE_REG = TRAN;
        BLOCK_COUNT_REG = 1;

        ret = sd_command(SWITCH_FUNC, 0x80ffffef, NULL, 0x1c05);
        if (ret < 0)
            goto card_init_error;

        /* Read 512 bytes from the card.
        The first 512 bits contain the status information
        */
        dataptr = carddata;
        for (i = 0; i < BLOCK_SIZE/2; i += FIFO_LEN)
        {
            /* Wait for the FIFO to be full */
            if (sd_poll_status(FIFO_FULL, 100000))
            {
                copy_read_sectors_slow(&dataptr);
                continue;
            }

            ret = -EC_FIFO_ENA_BANK_EMPTY;
            goto card_init_error;
        }
    }

    nand_card->initialized = 1;
    return;

    /* Card failed to initialize so disable it */
card_init_error:
    nand_card->initialized = ret;
}


static void sd_select_device(void)
{
    nand_card = &card_info;

    /* Main card always gets a chance */
    sd_status.retry = 0;
    
    // if nand already initialized(using sd_init_device)
    // directly switch to it      
    if (nand_card->initialized > 0)
    {
        /* This card is already initialized - switch to it */
        sd_card_mux();
        return;
    }

    if (nand_card->initialized == 0)
    {
        /* Card needs (re)init */
        sd_init_device();
    }
}



static void ata_led(int onoff)
{
    if (onoff)
      led_set(1);
    else
    	led_set(0);
}

int ata_read_sectors(unsigned long start,		// address of the block from which reading commences
					 					 int incount,           // number of blocks
                     void* inbuf)           // buffer to which blocks are copied
{

    int ret,bank;
    unsigned char *buf, *buf_end;
        
    ata_led(FALSE);

ata_read_retry:
   
    sd_select_device();

    if (nand_card->initialized < 0)
    {
        ret = nand_card->initialized;
        goto ata_read_error;
    }

    /* Only switch banks with non-SDHC cards */
    if((nand_card->ocr & (1<<30))==0)
    {
        // unsigned division ,if refusing to link
        // include libgcc.a when using ld with 
        // the -lgcc switch,preceded by -L search path 
        // as shown in Makefile
        
        bank = start / BLOCKS_PER_BANK;      

        if (nand_card->current_bank != bank)
        {
            ret = sd_select_bank(bank);
            if (ret < 0)
                goto ata_read_error;
        }
    
        start -= bank * BLOCKS_PER_BANK;
    }

    ret = sd_wait_for_state(TRAN, EC_TRAN_READ_ENTRY);
    if (ret < 0)
        goto ata_read_error;

    BLOCK_COUNT_REG = incount;

    ret = sd_command(READ_MULTIPLE_BLOCK, start * BLOCK_SIZE, NULL, 0x1c25);
    
    if (ret < 0)
        goto ata_read_error;

    /* TODO: Don't assume BLOCK_SIZE == SECTOR_SIZE */

    buf_end = (unsigned char *)inbuf + incount * nand_card->block_size;
    for (buf = inbuf; buf < buf_end;)
    {
        /* Wait for the FIFO to be full */
        if (sd_poll_status(FIFO_FULL, 0x80000))
        {
            copy_read_sectors_fast(&buf); /* Copy one chunk of 16 words */
            /* TODO: Switch bank if necessary */
            continue;
        }

        ret = -EC_FIFO_READ_FULL;
        goto ata_read_error;
    }

    ret = sd_command(STOP_TRANSMISSION, 0, NULL, 1);
    if (ret < 0)
        goto ata_read_error;

    ret = sd_wait_for_state(TRAN, EC_TRAN_READ_EXIT);
    if (ret < 0)
        goto ata_read_error;

    while (1)
    {
        ata_led(TRUE);
        return ret;

ata_read_error:
        if (sd_status.retry < sd_status.retry_max
            && ret != -EC_NOCARD)
        {
            sd_status.retry++;
            nand_card->initialized = 0;
            goto ata_read_retry;
        }
    }
}

int ata_write_sectors(unsigned long start,
											int count,
                      const void* outbuf)
{
// write support is not perfect, use at your own risk

    int ret;
    const unsigned char *buf, *buf_end;
    int bank;

    ata_led(FALSE);

ata_write_retry:
   

    sd_select_device();

    if (nand_card->initialized < 0)
    {
        ret = nand_card->initialized;
        goto ata_write_error;
    }

    /* Only switch banks with non-SDHC cards */
    if((nand_card->ocr & (1<<30))==0)
    {
        bank = start / BLOCKS_PER_BANK;

        if (nand_card->current_bank != bank)
        {
            ret = sd_select_bank(bank);
            if (ret < 0)
                goto ata_write_error;
        }
    
        start -= bank * BLOCKS_PER_BANK;
    }


    ret = sd_wait_for_state(TRAN, EC_TRAN_WRITE_ENTRY);
    if (ret < 0)
        goto ata_write_error;

    BLOCK_COUNT_REG = count;

    ret = sd_command(WRITE_MULTIPLE_BLOCK, start*BLOCK_SIZE, NULL, 0x1c2d);
    
    if (ret < 0)
        goto ata_write_error;

    buf_end = outbuf + count * nand_card->block_size - 2*FIFO_LEN;

    for (buf = outbuf; buf <= buf_end;)
    {
        if (buf == buf_end)
        {
            /* Set SD_STATE_REG to PRG for the last buffer fill */
            SD_STATE_REG = PRG;
        }

        usleep(2); /* needed here (loop is too fast :-) */

        /* Wait for the FIFO to empty */
        if (sd_poll_status(FIFO_EMPTY, 0x80000))
        {
            copy_write_sectors(&buf); /* Copy one chunk of 16 words */
            /* TODO: Switch bank if necessary */
            continue;
        }

        ret = -EC_FIFO_WR_EMPTY;
        goto ata_write_error;
    }

    
    if (!sd_poll_status(DATA_DONE, 0x80000))
    {
        ret = -EC_FIFO_WR_DONE;
        goto ata_write_error;
    }

    ret = sd_command(STOP_TRANSMISSION, 0, NULL, 1);
    if (ret < 0)
        goto ata_write_error;

    ret = sd_wait_for_state(TRAN, EC_TRAN_WRITE_EXIT);
    if (ret < 0)
        goto ata_write_error;

    while (1)
    {
        ata_led(TRUE);
        return ret;

ata_write_error:
        if (sd_status.retry < sd_status.retry_max
            && ret != -EC_NOCARD)
        {
            sd_status.retry++;
            nand_card->initialized = 0;
            goto ata_write_retry;
        }
    }
}

int ata_init(void)
{
    int ret = 0;

    ata_led(FALSE);

    if (!initialized)
    {
        initialized = TRUE;

        /* init controller */
        outl(inl(0x70000088) & ~(0x4), 0x70000088);
        outl(inl(0x7000008c) & ~(0x4), 0x7000008c);
        GPO32_ENABLE |= 0x4;

        GPIO_SET_BITWISE(GPIOG_ENABLE, (0x3 << 5));
        GPIO_SET_BITWISE(GPIOG_OUTPUT_EN, (0x3 << 5));
        GPIO_SET_BITWISE(GPIOG_OUTPUT_VAL, (0x3 << 5));

        sd_select_device();

        if (nand_card->initialized < 0)
            ret = nand_card->initialized;

    }
    ata_led(TRUE);
    return ret;
}
    
/*
 * The USB HW interface was taken from Freescale i.MX31
 * documentation. Apparently PP5024 uses the same USB core.
 */

#include "e200_protocol.h"

#define UOG_USBCMD           (*(volatile int *)0xc5000140)
#define UOG_USBSTS           (*(volatile int *)0xc5000144)
#define UOG_DEVICEADDR       (*(volatile int *)0xc5000154)
#define UOG_ENDPOINTLISTADDR (*(volatile int *)0xc5000158)
#define UOG_PORTSC0          (*(volatile int *)0xc5000184)
#define UOG_USBMODE          (*(volatile int *)0xc50001a8)
#define UOG_ENDPTSETUPSTAT   (*(volatile int *)0xc50001ac)
#define UOG_ENDPTPRIME       (*(volatile int *)0xc50001b0)
#define UOG_ENDPTFLUSH       (*(volatile int *)0xc50001b4)
#define UOG_ENDPTSTAT        (*(volatile int *)0xc50001b8)
#define UOG_ENDPTCOMPLETE    (*(volatile int *)0xc50001bc)
#define UOG_ENDPTCTRL0       (*(volatile int *)0xc50001c0)

#define MAX_PACKET_SIZE 64

#define BUFFER_SIZE 4096   

#define USB_BUS_RESET (UOG_USBSTS & (1 << 6))

#define PRIME_TIMER    100000
#define TRANSFER_TIMER 1000000
#define RESET_TIMER    5000000

struct dtd {
  unsigned int next_dtd;
  unsigned int dtd_token;
  unsigned int buf_ptr0;
  unsigned int buf_ptr1;
  unsigned int buf_ptr2;
  unsigned int buf_ptr3;
  unsigned int buf_ptr4;
  unsigned int pad; /* pad to 32 bytes */
} __attribute((packed));

struct dqh {
  unsigned int endpt_cap;
  unsigned int cur_dtd;
  struct dtd   dtd_ovrl;
  unsigned int setup_buf[2];
  unsigned int pad[4]; /* pad to 64 bytes */
} __attribute((packed));

struct spkt {
  unsigned char  bmRequestType;
  unsigned char  bRequest;
  unsigned short wValue;
  unsigned short wIndex;
  unsigned short wLength;
} __attribute((packed));

struct devdsc {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned short bcdUSB;
  unsigned char  bDeviceClass;
  unsigned char  bDeviceSubClass;
  unsigned char  bDeviceProtocol;
  unsigned char  bMaxPacketSize0;
  unsigned short idVendor;
  unsigned short idProduct;
  unsigned short bcdDevice;
  unsigned char  iManufacturer;
  unsigned char  iProduct;
  unsigned char  iSerialNumber;
  unsigned char  bNumConfigurations;
} __attribute((packed));

struct dqdsc {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned short bcdUSB;
  unsigned char  bDeviceClass;
  unsigned char  bDeviceSubClass;
  unsigned char  bDeviceProtocol;
  unsigned char  bMaxPacketSize0;
  unsigned char  bNumConfigurations;
  unsigned char  bReserved;
} __attribute((packed));

struct confdsc {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned short wTotalLength;
  unsigned char  bNumInterfaces;
  unsigned char  bConfigurationValue;
  unsigned char  iConfiguration;
  unsigned char  bmAttributes;
  unsigned char  bMaxPower;
} __attribute((packed));

struct ifdsc {
  unsigned char bLength;
  unsigned char bDescriptorType;
  unsigned char bInterfaceNumber;
  unsigned char bAlternateSetting;
  unsigned char bNumEndpoints;
  unsigned char bInterfaceClass;
  unsigned char bInterfaceSubClass;
  unsigned char bInterfaceProtocol;
  unsigned char iInterface;
} __attribute((packed));
/****************************************
struct epdsc {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned char  bEndpointAddress;
  unsigned char  bmAttributes;
  unsigned short wMaxPacketSize;
  unsigned char  bInterval;
} __attribute((packed));
*****************************************/

struct strdsc {				
    unsigned char  bLength;					
    unsigned char  bDescriptorType;
    unsigned short wString[]; /* UTF-16LE encoded */
} __attribute((packed));

#define DEVDSC_INIT \
  .bcdUSB             = 0x0200, \
  .bDeviceClass       = 0xff, \
  .bDeviceSubClass    = 0xff, \
  .bDeviceProtocol    = 0xff, \
  .bMaxPacketSize0    = MAX_PACKET_SIZE, \
  .bNumConfigurations = 1


const struct devdsc devdsc = {
  .bLength         = sizeof(struct devdsc),
  .bDescriptorType = 1,
  .idVendor        = OWN_VENDOR_ID,
  .idProduct       = OWN_PRODUCT_ID,
  .bcdDevice       = OWN_PRODUCT_REV,
  .iManufacturer   = 1,									
  .iProduct        = 2,			
  .iSerialNumber   = 3,
  DEVDSC_INIT
};

const struct dqdsc dqdsc = {
  .bLength         = sizeof(struct dqdsc),
  .bDescriptorType = 6,
  DEVDSC_INIT
};

struct confdata {
  struct confdsc conf;
  struct ifdsc   iface;
} __attribute((packed));

const struct confdata confdata = {
  .conf.bLength             = sizeof(struct confdsc),
  .conf.bDescriptorType     = 2,
  .conf.wTotalLength        = sizeof(struct confdata),
  .conf.bNumInterfaces      = 1,
  .conf.bConfigurationValue = 1,
  .conf.iConfiguration      = 0,           
  .conf.bmAttributes        = 0xc0,
  .conf.bMaxPower           = 250,

  .iface.bLength            = sizeof(struct ifdsc),
  .iface.bDescriptorType    = 4,
  .iface.bInterfaceNumber   = 0,
  .iface.bAlternateSetting  = 0,
  .iface.bNumEndpoints      = 0,
  .iface.bInterfaceClass    = 0xff,
  .iface.bInterfaceSubClass = 0xff,
  .iface.bInterfaceProtocol = 0xff,
};


#define USB_DT_STRING 0x03

const struct strdsc string_iManufacturer = {
    34,									
    USB_DT_STRING,
    {'S','t','i','l','l',' ','S','a','n','d','i','s','k',' ',':','P'}
};

const struct strdsc string_iProduct = {
    56,
    USB_DT_STRING,
    {'S','a','n','s','a',' ','e','2','0','0',' ','i','n',' ',
     'e','2','0','0','t','o','o','l',' ','m','o','d','e'}
};

const struct strdsc string_iSerial = {
    8,
    USB_DT_STRING,
    {'0','0','7'}
};

const struct strdsc lang_descriptor = {
    4,
    USB_DT_STRING,
    {0x0409} /* LANGID US English */
};

const struct strdsc* const usb_strings[] = {
   &lang_descriptor,
   &string_iManufacturer,
   &string_iProduct,
   &string_iSerial
};

struct dtd dev_td[2] __attribute((aligned (32)));

#define HOST2DEV 0   // for REQ_OUT - host giving output to device
#define DEV2HOST 1   // for REQ_IN  - host requesting input from device

struct dqh dev_qh[2] __attribute((aligned (1 << 11)));

unsigned char buffer[BUFFER_SIZE] __attribute((aligned (1 << 12))); // never gets filled by more than 64 bytes

void
dqh_init(int dir)
{
  struct dqh * const dqh = &dev_qh[dir];

  memset(dqh, 0, sizeof(*dqh));

  dqh->endpt_cap = (MAX_PACKET_SIZE << 16);

  dqh->dtd_ovrl.next_dtd  = 1;
  dqh->dtd_ovrl.dtd_token = 0;
}

void
dtd_init(int dir, void * buf, int size)
{
  struct dtd * const dtd = &dev_td[dir];

  dtd->next_dtd  = 1;
  dtd->dtd_token = (size << 16) | (1 << 7);

  dtd->buf_ptr0 = (unsigned int)buf;
  dtd->buf_ptr1 = 0;
  dtd->buf_ptr2 = 0;
  dtd->buf_ptr3 = 0;
  dtd->buf_ptr4 = 0;
}

int
dtd_enqueue(int dir)
{
  const unsigned int mask = 1 << (dir * 16);
  struct dqh * const dqh = &dev_qh[dir];
  struct dtd * const dtd = &dev_td[dir];
  struct timer t;

  dqh->dtd_ovrl.next_dtd   = (unsigned int)dtd;
  dqh->dtd_ovrl.dtd_token &= ~0xc0;

  //cache_flush();

  timer_set(&t, PRIME_TIMER);

  UOG_ENDPTPRIME |= mask;

  while (UOG_ENDPTPRIME & mask) {
    if (USB_BUS_RESET) {
      longjmp(reset_env, 1);
    }

    if (timer_expired(&t)) {
      exit(ERROR_TIMEOUT);
    }
  }

  if ((UOG_ENDPTSTAT & mask) == 0) {
    exit(ERROR_HW);
  }

  return 0;
}

int
dtd_wait(int dir)
{
  const unsigned int mask = 1 << (dir * 16);
  struct dtd * const dtd = &dev_td[dir];
  struct timer t;

  timer_set(&t, TRANSFER_TIMER);

  for (;;) {
    if (USB_BUS_RESET) {
      longjmp(reset_env, 1);
    }

    if ((UOG_ENDPTCOMPLETE & mask) != 0) {
      UOG_ENDPTCOMPLETE |= mask;
    }

    if ((dtd->dtd_token & (1 << 7)) == 0) {
      return 0;
    }

    if (timer_expired(&t)) {
      return ERROR_TIMEOUT;
    }
  }
}

int
dtd_execute(int dir)
{
  int ret;

  ret = dtd_enqueue(dir);

  if (ret == 0) {
    ret = dtd_wait(dir);
  }

  return ret;
}

int
usb_send(const void * buf, int size)
{
  const char * ptr = buf;
  int todo, done = 0;
  int error;

  do {
    if (USB_BUS_RESET) {
      longjmp(reset_env, 1);
    }

    todo = MIN(size, BUFFER_SIZE);

    memcpy(buffer, ptr, todo);

    dtd_init(DEV2HOST, buffer, todo);
    error = dtd_execute(DEV2HOST);

    if (error) {
      done = error;
      break;
    }

    size -= todo;
    ptr  += todo;
    done += todo;

  } while (size > 0);

  return done;
}

int
usb_receive(void * buf, int size)
{
  char * ptr = buf;
  int todo, done = 0;
  int error;

  do {
    if (USB_BUS_RESET) {
      longjmp(reset_env, 1);
    }

    todo = MIN(size, BUFFER_SIZE);

    dtd_init(HOST2DEV, buffer, size);
    error = dtd_execute(HOST2DEV);

    if (error) {
      done = error;
      break;
    }

    memcpy(ptr, buffer, todo);

    size -= todo;
    ptr  += todo;
    done += todo;

  } while (size > 0);

  return done;
}

int
usb_ack(struct spkt * s, int error)
{
  if (USB_BUS_RESET) {
    longjmp(reset_env, 1);
  }

  if (error) {
    UOG_ENDPTCTRL0 |= 1 << 16; /* stall */
    return 0;
  }

  if (s->bmRequestType & 0x80) {
    return usb_receive(NULL, 0);
  } else {
    return usb_send(NULL, 0);
  }
}
  
int
usb_handle_setup_dev_get_desc(struct spkt * s)
{
  int ret,str_index= s->wValue & 0xff;

  switch (s->wValue >> 8) {
  case 0x01 : /* device descriptor */
    ret = usb_send(&devdsc, MIN(sizeof(devdsc), s->wLength));
    break;
  case 0x02 : /* configuration descriptor */
    ret = usb_send(&confdata, MIN(sizeof(confdata), s->wLength));
    break;
  case 0x03 : /* string descriptor  */								
    ret = usb_send(usb_strings[str_index], MIN(usb_strings[str_index]->bLength, s->wLength));
    break;
  case 0x06 : /* device qualifier descriptor */
    ret = usb_send(&dqdsc, MIN(sizeof(dqdsc), s->wLength));
    break;
  default :
    ret = ERROR_UNKNOWN;
  }

  return ret;
}

int
usb_handle_setup_dev_std(struct spkt * s)
{
  const unsigned char  conf   = 1;
  const unsigned short status = 0;
  int ret, error = 0;

  switch (s->bRequest) {
  case 0x00 : /* get status */
    ret = usb_send(&status, MIN(sizeof(status), s->wLength));
    break;
  case 0x05 : /* set address */
    ret = 0;
    break;
  case 0x06 : /* get descriptor */
    ret = usb_handle_setup_dev_get_desc(s);
    break;
  case 0x08 : /* get configuration */
    ret = usb_send(&conf, MIN(sizeof(conf), s->wLength));
    break;
  case 0x09 : /* set configuration */
    ret = 0;
    break;
  default :
    ret = ERROR_UNKNOWN;
  }

  if (ret < 0) {
    error = ret;
  }

  /* ACK the transfer */

  usb_ack(s, error);

  if (s->bRequest == 0x05) { /* set address, must be set after ack */
    UOG_DEVICEADDR = s->wValue << 25;
  }

  return 0;
}

int
usb_handle_setup_dev_vnd(struct spkt * s)
{
  int ret, error = 0;
  unsigned int val = 0;
  struct timer t;

  switch (s->bRequest) {
  case REQ_NONE :
    ret = 0;
    break;
  case REQ_SETPTR :
    ret = usb_receive(&cur_addr, sizeof(cur_addr));
    break;
  case REQ_WRITE :
    ret = usb_receive(cur_addr, s->wLength);
    cur_addr += ret;
    break;
  case REQ_READ :
    ret = usb_send(cur_addr, s->wLength);
    cur_addr += ret;
    break;
  case REQ_RUN :
    ret = 0;
    break;
  case REQ_LOAD :
    switch (s->wLength) {
    case 1 :
      val = *(volatile unsigned char *)cur_addr;
      break;
    case 2 :
      val = *(volatile unsigned short *)cur_addr;
      break;
    case 4 :
      val = *(volatile unsigned int *)cur_addr;
      break;
    default :
      exit(ERROR_INVALID);
    }
    ret = usb_send(&val, s->wLength);
    break;
  case REQ_STORE :
    ret = usb_receive(&val, s->wLength);
    if (ret == s->wLength) {
      switch (s->wLength) {
      case 1 :
				*(volatile unsigned char *)cur_addr = val;
				break;
      case 2 :
				*(volatile unsigned short *)cur_addr = val;
				break;
      case 4 :
				*(volatile unsigned int *)cur_addr = val;
				break;
      default :
				exit(ERROR_INVALID);
      }
    }
    break;
  case REQ_I2C_READ :
    if (s->wLength > TMPBUF_SIZE) {
      ret = ERROR_INVALID;
    } else {
      ret = i2c_read(s->wIndex, tmpbuf, s->wLength);
      if (ret >= 0) {
				ret = usb_send(tmpbuf, ret);
      }
    }
    break;
  case REQ_I2C_WRITE :
    ret = ERROR_INVALID;
    if (s->wLength <= 4) {
      ret = usb_receive(tmpbuf, s->wLength);
      if (ret == s->wLength) {
	timer_set(&t, s->wValue * 1000);
				do {
					ret = i2c_write(s->wIndex, tmpbuf, s->wLength);
				} while (ret != s->wLength && !timer_expired(&t));
      }
    }
    break;
  case REQ_I2C_PROGRAM :
    ret = i2c_program(s->wIndex, cur_addr, s->wValue);
    break;
  case REQ_POWEROFF :
    ret = 0;
    break;
  case REQ_NANDADDR :
  	ret= usb_receive(&nand_addr,s->wLength);
  	break;
  case REQ_NANDREAD :
  	ret = ata_read_sectors(nand_addr,s->wValue,cur_addr);
  	break;
  case REQ_NANDWRITE :
    ret = ata_write_sectors(nand_addr,s->wValue,cur_addr);
    break;
  default :
    ret = ERROR_UNKNOWN;
  }

  if (ret < 0) {
    error = ret;
  }

  /* ACK the transfer */

  usb_ack(s, error);

  if (s->bRequest == REQ_RUN) { /* ack before jumping */
    longjmp(start_env, 2);
  }

  if (s->bRequest == REQ_POWEROFF) { /* ack before power off */
    poweroff();
  }

  return 0;
}

int
usb_handle_setup_dev(struct spkt * s)
{
  switch ((s->bmRequestType >> 5) & 3) {
  case 0x00 :
    return usb_handle_setup_dev_std(s);
  case 0x02 :
    return usb_handle_setup_dev_vnd(s);
  }

  exit(ERROR_UNKNOWN);

  return 0;
}

int
usb_handle_setup_if_std(struct spkt * s)
{
  int error;

  switch (s->bRequest) {
  case 0x0b : /* set interface */
    error = 0;
    break;
  default :
    error = ERROR_UNKNOWN;
    break;
  }

  usb_ack(s, error);

  return 0;
}

int
usb_handle_setup_if(struct spkt * s)
{
  switch ((s->bmRequestType >> 5) & 3) {
  case 0x00 :
    return usb_handle_setup_if_std(s);
  }
  
  return usb_ack(s, ERROR_UNKNOWN);
}

int
usb_handle_setup_ep(struct spkt * s)
{
  return usb_ack(s, ERROR_UNKNOWN);
}

int
usb_handle_setup(struct spkt * s)
{
  switch (s->bmRequestType & 0x1f) {
  case 0x00 :
    return usb_handle_setup_dev(s);
  case 0x01 :
    return usb_handle_setup_if(s);
  case 0x02 :
    return usb_handle_setup_ep(s);
  }

  exit(ERROR_UNKNOWN);
}

void
usb_handle_reset()
{
  struct timer t;

  timer_set(&t, RESET_TIMER);

  UOG_ENDPTSETUPSTAT = UOG_ENDPTSETUPSTAT;
  UOG_ENDPTCOMPLETE  = UOG_ENDPTCOMPLETE;

  while (UOG_ENDPTPRIME) { /* prime and flush pending transfers */
    if (timer_expired(&t)) {
      exit(ERROR_TIMEOUT);
    }
  }

  UOG_ENDPTFLUSH = ~0;

  if ((UOG_PORTSC0 & (1 << 8)) == 0) {
    exit(ERROR_TIMEOUT); /* init too slow ! */
  }

  UOG_USBSTS = (1 << 6);

  while ((UOG_USBSTS & (1 << 2)) == 0) { /* wait for port change */
    if (timer_expired(&t)) {
      exit(ERROR_TIMEOUT);
    }
  }

  UOG_USBSTS = (1 << 2);
}

void
restart()
{
  UOG_USBCMD &= ~(1 << 0); /* stop */

  usleep(1000000);

  cur_addr = (void *)_start;
  longjmp(start_env, 2);
}

int
main()
{
  struct spkt spkt;
  struct timer t;
  int ret;

  i2c_init();
  ata_init();

  ret = setjmp(start_env);

  if (ret > 1) {
    return 0;
  }

  /* Reset the USB controller */

  led_set(0);

  usleep(200000);

  UOG_USBCMD &= ~(1 << 0); /* stop */

  usleep(500000);

  timer_set(&t, RESET_TIMER);

  UOG_USBCMD |= (1 << 1); /* reset */
  while (UOG_USBCMD & (1 << 1)) {
    if (timer_expired(&t)) {
      exit(ERROR_HW);
    }
  }

  led_set(1);

  /* Configure the controller */

  UOG_USBMODE = 2; /* device mode */

  /* Init queue heads */

  dqh_init(HOST2DEV);
  dqh_init(DEV2HOST);

  UOG_ENDPOINTLISTADDR = (unsigned int)dev_qh;

  /* Start the device */

  UOG_USBCMD |= (1 << 0); /* run */

  setjmp(reset_env);

  for (;;) {
    if (USB_BUS_RESET) {
      usb_handle_reset();
      continue;
    }

    if ((UOG_ENDPTSETUPSTAT & 0x1) != 0) {
      memcpy(&spkt, dev_qh[0].setup_buf, sizeof(spkt));
      UOG_ENDPTSETUPSTAT = UOG_ENDPTSETUPSTAT;
      usb_handle_setup(&spkt);
    }
  }
}
