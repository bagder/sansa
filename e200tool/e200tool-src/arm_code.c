
/*
 * This is the USB stub which will run on the e200. You do
 * not need to compile this yourself. A pre-compiled version
 * should be available in e200_code.c.
 *
 * In case you do not trust me and wish to compile it yourself,
 * make sure that the result is relocated to 0x40004000 and the
 * entrypoint is in the beginning of the file. I did it with
 *
 *     arm-elf-gcc -Os -fno-unit-at-a-time -mcpu=arm7tdmi
 *     arm-elf-ld -Ttext 0x40004000 -N
 *     arm-elf-objcopy -Obinary
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
 * (c) MrH 2006, 2007
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

#define TMPBUF_SIZE 64

char tmpbuf[TMPBUF_SIZE];

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

void
cache_flush()
{
  *(volatile int*)0xf000f044 |= 2;

  while ((*(volatile int *)0x6000c000) & 0x8000);
}

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

void
halt()
{
  for(;;) {
    COP_SLEEP |= (1 << 31);
    CPU_SLEEP |= (1 << 31);
  }
}

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
 * The i2c inteface.
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

struct epdsc {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned char  bEndpointAddress;
  unsigned char  bmAttributes;
  unsigned short wMaxPacketSize;
  unsigned char  bInterval;
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

struct dtd dev_td[2] __attribute((aligned (32)));

#define HOST2DEV 0
#define DEV2HOST 1

struct dqh dev_qh[2] __attribute((aligned (1 << 11)));

unsigned char buffer[BUFFER_SIZE] __attribute((aligned (1 << 12)));

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
  int ret;

  switch (s->wValue >> 8) {
  case 0x01 : /* device descriptor */
    ret = usb_send(&devdsc, MIN(sizeof(devdsc), s->wLength));
    break;
  case 0x02 : /* configuration descriptor */
    ret = usb_send(&confdata, MIN(sizeof(confdata), s->wLength));
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
