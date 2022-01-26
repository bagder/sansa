
/*
 * e200tool
 *
 * A tool for accessing the SanDisk e200 series manufacturing mode.
 *
 * !!! It does not get much lower level than this! While this tool
 *     does not intentionally try to harm your player, it still may
 *     very well do so due to a bug or user error. Also the tool
 *     has very minimal sanity checks, so it is very easy to misuse
 *     it. If you still decide to use it, you do so with your own
 *     responsibility!
 *
 * The manufacturing mode can be activated like this:
 *
 * 1. Power off the player
 * 2. Set keylock on
 * 3. Press and hold the centre button and power on the player
 * 4. When the wheel lits release the centre button
 *
 * (c) MrH 2006, 2007
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <usb.h> /* This tool requires libusb */

#define VERSION "v0.2.3-alpha"

#include "e200_protocol.h"
#include "e200_code.c"

/*
 * Weird, but there seems to be no universally portable way
 * of doing delays. If you are compiling this for some new
 * platform, add your implementation here.
 */

#if defined(YourArchHere)
void
do_delay(int secs)
{
}
#else /* POSIX */
#include <unistd.h>
void
do_delay(int secs)
{
  sleep(secs);
}
#endif

#define E200_INTERFACE 0
#define E200_ENDPOINT  1

#define E200_TIMEOUT    5000
#define PROGRAM_TIMEOUT 120000

#define SANDISK_VENDOR_ID  0x0781
#define SANDISK_PRODUCT_ID 0x0720
#define PP_VENDOR_ID       0x0b70
#define PP_PRODUCT_ID      0x0003

#define BUFSIZE 64

char buffer[BUFSIZE];

#undef  MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))

void
put_le32(void * ptr, uint32_t val)
{
  uint8_t * b = ptr;

  b[0] = val;
  b[1] = val >> 8;
  b[2] = val >> 16;
  b[3] = val >> 24;
}

uint32_t
get_le32(void * ptr)
{
  uint8_t * b = ptr;
  uint32_t val;

  val = b[3];
  val = (val << 8) | b[2];
  val = (val << 8) | b[1];
  val = (val << 8) | b[0];

  return val;
}

int
send_dev(usb_dev_handle * ud, const void * data, uint32_t len)
{
  char * buf = (char *)data;
  int32_t todo;
  int ret;

  while (len > 0) {
    todo = len > BUFSIZE ? BUFSIZE : len;

    ret = usb_bulk_write(ud, E200_ENDPOINT, buf, todo, E200_TIMEOUT);

    if (ret < 0) {
      fprintf(stderr, "\nBulk write error (%d, %s)\n", ret, strerror(-ret));
      return ret;
    }

    buf += ret;
    len -= ret;
  }

  return 0;
}

int
device_match(struct usb_device * dev, int vendor, int product)
{
  return (dev->descriptor.idVendor        == vendor &&
	  dev->descriptor.idProduct       == product &&
	  dev->descriptor.bDeviceClass    == 0xff &&
	  dev->descriptor.bDeviceSubClass == 0xff &&
	  dev->descriptor.bDeviceProtocol == 0xff);
}

usb_dev_handle *
device_get(int vendor, int product)
{
  struct usb_bus * bus, * busses;
  struct usb_device * dev;
  usb_dev_handle * ud;
  int ret, retries = 10;

  fprintf(stderr, "Searching for device %04x:%04x ... ", vendor, product);

 retry:

  usb_find_busses();
  usb_find_devices();

  busses = usb_get_busses();

  for (bus = busses; bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      if (!device_match(dev, vendor, product)) {
	continue;
      }

      fprintf(stderr, "found!\n");

      ud = usb_open(dev);

      if (!ud) {
	fprintf(stderr, "Failed to open the device!\n");
	return NULL;
      }

      ret = usb_claim_interface(ud, E200_INTERFACE);

      if (ret < 0) {
	fprintf(stderr, "Failed to claim the interface (%d, %s)\n",
		ret, strerror(-ret));
	usb_close(ud);

	return NULL;
      }

      return ud;
    }
  }

  if (retries-- > 0) {
    do_delay(1);
    fprintf(stderr, "%d ", retries);
    goto retry;
  }

  fprintf(stderr, "not found!\n");

  return NULL;
}

int
device_release(usb_dev_handle * ud)
{
  usb_release_interface(ud, E200_INTERFACE);

  return usb_close(ud);
}

int
parse_uint32(const char * s, uint32_t * v)
{
  char * c;

  *v = strtoul(s, &c, 0);

  if (*c != 0) {
    fprintf(stderr, "Invalid value '%s'\n", s);
    return 0;
  }

  return 1;
}

int
parse_int(const char * s, int * v)
{
  char * c;

  *v = strtol(s, &c, 0);

  if (*c != 0) {
    fprintf(stderr, "Invalid value '%s'\n", s);
    return 0;
  }

  return 1;
}

typedef int (*cmd_func)(int, char **);

struct cmd {
  const char * name;
  cmd_func     func;
  const char * desc;
};

int cmd_recover(int, char **);
int cmd_init(int, char **);
int cmd_off(int, char **);
int cmd_read(int, char **);
int cmd_write(int, char **);
int cmd_run(int, char **);
int cmd_lw(int, char **);
int cmd_lh(int, char **);
int cmd_lb(int, char **);
int cmd_sw(int, char **);
int cmd_sh(int, char **);
int cmd_sb(int, char **);
int cmd_i2cread(int, char **);
int cmd_i2cwrite(int, char **);
int cmd_i2cdump(int, char **);
int cmd_i2cprogram(int, char **);
int cmd_i2cverify(int, char **);

struct cmd cmd_table[] = {
  { "recover",    cmd_recover,    "recover a corrupted bootloader" },
  { "init",       cmd_init,       "initialize the connection" },
  { "off",        cmd_off,        "power off the device" },
  { "read",       cmd_read,       "read from the device memory" },
  { "write",      cmd_write,      "write to the device memory" },
  { "run",        cmd_run,        "execute from specified address" },
  { "lw",         cmd_lw,         "load a word" },
  { "lh",         cmd_lh,         "load a halfword" },
  { "lb",         cmd_lb,         "load a byte" },
  { "sw",         cmd_sw,         "store a word" },
  { "sh",         cmd_sh,         "store a halfword" },
  { "sb",         cmd_sb,         "store a byte" },
  { "i2cread",    cmd_i2cread,    "read from an i2c device" },
  { "i2cwrite",   cmd_i2cwrite,   "write to an i2c device" },
  { "i2cdump",    cmd_i2cdump,    "dump the i2c rom" },
  { "i2cprogram", cmd_i2cprogram, "program the i2c rom" },
  { "i2cverify",  cmd_i2cverify,  "verify the i2c rom" },
};

#define NUM_CMDS (sizeof(cmd_table) / sizeof(cmd_table[0]))

void
help()
{
  int i;

  fprintf(stderr, "\n"
	  "Usage:\te200tool <command> [arg1] ...\n"
	  "\n"
	  "commands:\n");

  for (i = 0; i < NUM_CMDS; i++) {
    fprintf(stderr, "\t%-20.20s%s\n", cmd_table[i].name, cmd_table[i].desc);
  }

  fprintf(stderr,
	  "\nUse 'e200tool <command>' for help on specific command\n\n");

  fprintf(stderr,
	  "DO NOT USE THIS TOOL UNLESS YOU KNOW WHAT YOU ARE DOING! I WILL\n"
	  "TAKE NO RESPONSIBILITY NO MATTER WHAT BAD THINGS MIGHT HAPPEN TO\n"
	  "YOUR PLAYER/COMPUTER/WHATEVER! IF THIS MAKES YOU UNEASY, PLEASE\n"
	  "DELETE THIS TOOL NOW!\n\n");

  exit(1);
}

int
cmd_init(int argc, char * argv[])
{
  static const uint32_t len = sizeof(e200code);
  usb_dev_handle * ud;
  int ret;

  ud = device_get(SANDISK_VENDOR_ID, SANDISK_PRODUCT_ID);

  if (!ud) {
    ud = device_get(PP_VENDOR_ID, PP_PRODUCT_ID);
    if (!ud) {
      exit(1);
    }
  }

  fprintf(stderr, "Initializing USB stub (%d bytes) ... ", len);

  put_le32(buffer, len);

  ret = usb_bulk_write(ud, E200_ENDPOINT, buffer, sizeof(len), E200_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "\nLength write error (%d, %s)\n", ret, strerror(-ret));
    exit(1);
  }

  ret = send_dev(ud, e200code, len);

  if (ret < 0) {
    exit(1);
  }

  fprintf(stderr, "done!\n");

  device_release(ud);

  return 0;
}

int
cmd_off(int arcg, char * argv[])
{
  usb_dev_handle * ud;
  int ret;

  fprintf(stderr, "Powering off the device\n");

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  ret = usb_control_msg(ud, REQ_OUT, REQ_POWEROFF,
			0, 0, NULL, 0, E200_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "Control message (%d, %s)\n", ret, strerror(-ret));
    exit(1);
  }

  fprintf(stderr, "Done!\n");

  device_release(ud);

  return ret;
}

int
set_ptr(usb_dev_handle * ud, uint32_t addr)
{
  int ret;

  put_le32(buffer, addr);

  ret = usb_control_msg(ud, REQ_OUT, REQ_SETPTR, 0, 0,
			buffer, sizeof(addr), E200_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "Set ptr - error (%d, %s)\n", ret, strerror(-ret));
    exit(1);
  }  

  return 0;
}

void
help_read()
{
  fprintf(stderr, "\nUsage:\te200tool read <file> <address> <length>\n\n");
  exit(1);
}

int
cmd_read(int argc, char * argv[])
{
  usb_dev_handle * ud;
  uint32_t addr, total;
  int len, ret;
  char * fn;
  FILE * fp;

  if (argc != 3) {
    help_read();
  }

  if (!strcmp(argv[0], "-")) {
    fn = "stdout";
    fp = stdout;
  } else {
    fn = argv[0];
    fp = fopen(argv[0], "wb");
    if (!fp) {
      perror(argv[0]);
      exit(1);
    }
  }

  if (!parse_uint32(argv[1], &addr) || !parse_uint32(argv[2], &total)) {
    exit(1);
  }

  fprintf(stderr, "Reading '%s' from range 0x%08x-0x%08x\n",
	  fn, addr, addr + total);

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  set_ptr(ud, addr);

  while (total) {
    len = MIN(total, BUFSIZE - 1);

    ret = usb_control_msg(ud, REQ_IN, REQ_READ, 0, 0,
			  buffer, len, E200_TIMEOUT);

    if (ret < 0) {
      fprintf(stderr, "\nControl message (%d, %s)\n", ret, strerror(-ret));
      exit(1);
    }

    fwrite(buffer, 1, ret, fp);

    fprintf(stderr, "\rRead from 0x%08x", addr);

    addr  += ret;
    total -= ret;
  }

  fprintf(stderr, "\rRead from 0x%08x\nRead done!\n", addr);

  fclose(fp);

  device_release(ud);

  return 0;
}

void
help_write()
{
  fprintf(stderr, "\nUsage:\te200tool write <file> <address>\n\n");
  exit(1);
}

int
cmd_write(int argc, char * argv[])
{
  usb_dev_handle * ud;
  uint32_t addr;
  int len, ret;
  char * fn;
  FILE * fp;

  if (argc != 2) {
    help_write();
  }

  if (!strcmp(argv[0], "-")) {
    fn = "stdin";
    fp = stdin;
  } else {
    fn = argv[0];
    fp = fopen(argv[0], "rb");
    if (!fp) {
      perror(argv[0]);
      exit(1);
    }
  }

  if (!parse_uint32(argv[1], &addr)) {
    exit(1);
  }

  fprintf(stderr, "Writing '%s' to address 0x%08x\n", fn, addr);

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  set_ptr(ud, addr);

  while ((len = fread(buffer, 1, BUFSIZE - 1, fp)) > 0) { /* XXX */
    fprintf(stderr, "\rWrite at 0x%08x", addr);

    ret = usb_control_msg(ud, REQ_OUT, REQ_WRITE, 0, 0,
			  buffer, len, E200_TIMEOUT);

    if (ret < 0) {
      fprintf(stderr, "\nControl message (%d, %s)\n", ret, strerror(-ret));
      exit(1);
    }

    addr += ret;
  }

  fprintf(stderr, "\rWrite at 0x%08x\nWrite done!\n", addr);

  fclose(fp);

  device_release(ud);

  return 0;
}

void
help_run()
{
  fprintf(stderr, "\nUsage:\te200tool run <address>\n\n");
  exit(1);
}

int
cmd_run(int argc, char * argv[])
{
  usb_dev_handle * ud;
  uint32_t addr;
  int ret;

  if (argc != 1) {
    help_run();
  }

  if (!parse_uint32(argv[0], &addr)) {
    exit(1);
  }

  fprintf(stderr, "Running from address 0x%08x\n", addr);

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  set_ptr(ud, addr);

  ret = usb_control_msg(ud, REQ_OUT, REQ_RUN, 0, 0, NULL, 0, E200_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "Control message (%d, %s)\n", ret, strerror(-ret));
    exit(1);
  }

  fprintf(stderr, "Execution started!\n");

  device_release(ud);

  return ret;
}

void
help_load()
{
  fprintf(stderr, "\nUsage:\te200tool l<w/h/b< <address>\n\n");
  exit(1);
}

int
load(int argc, char * argv[], int size)
{
  usb_dev_handle * ud;
  uint32_t addr, val;
  int ret, i;

  if (argc != 1) {
    help_load();
  }

  if (!parse_uint32(argv[0], &addr)) {
    exit(1);
  }

  if ((addr % size) != 0) {
    fprintf(stderr, "Address not properly aligned!\n");
    exit(1);
  }

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  set_ptr(ud, addr);

  memset(buffer, 0, sizeof(val));

  ret = usb_control_msg(ud, REQ_IN, REQ_LOAD, 0, 0,
			buffer, size, E200_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "\nControl message (%d, %s)\n", ret, strerror(-ret));
    exit(1);
  }

  val = get_le32(buffer);

  device_release(ud);

  fprintf(stderr, "0x%0*x (%u) '", size * 2, val, val);

  for (i = size - 1; i >= 0; i--) { 
    fprintf(stderr, "%c", isprint(buffer[i]) ? buffer[i] : '.');
  }

  fprintf(stderr, "'\n");

  return 0;
}

int
cmd_lw(int argc, char * argv[])
{
  return load(argc, argv, 4);
}

int
cmd_lh(int argc, char * argv[])
{
  return load(argc, argv, 2);
}

int
cmd_lb(int argc, char * argv[])
{
  return load(argc, argv, 1);
}

void
help_store()
{
  fprintf(stderr, "\nUsage:\te200tool s<w/h/b> <address> <value>\n\n");
  exit(1);
}

int
store(int argc, char * argv[], int size)
{
  usb_dev_handle * ud;
  uint32_t addr, val;
  int ret;

  if (argc != 2) {
    help_store();
  }

  if (!parse_uint32(argv[0], &addr) || !parse_uint32(argv[1], &val)) {
    exit(1);
  }

  if ((addr % size) != 0) {
    fprintf(stderr, "Address not properly aligned!\n");
    exit(1);
  }

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  set_ptr(ud, addr);

  put_le32(buffer, val);

  ret = usb_control_msg(ud, REQ_OUT, REQ_STORE, 0, 0,
			buffer, size, E200_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "\nControl message (%d, %s)\n", ret, strerror(-ret));
    exit(1);
  }

  device_release(ud);

  fprintf(stderr, "Done!\n");

  return 0;
}

int
cmd_sw(int argc, char * argv[])
{
  return store(argc, argv, 4);
}

int
cmd_sh(int argc, char * argv[])
{
  return store(argc, argv, 2);
}

int
cmd_sb(int argc, char * argv[])
{
  return store(argc, argv, 1);
}

void
help_recover()
{
  fprintf(stderr, "\nUsage:\te200tool recover <bootloader_file>\n\n");
  exit(1);
}

#define DEFAULT_BL_ADDR "0x10600000"

int
cmd_recover(int argc, char * argv[])
{
  char * write_arg[2];
  char * run_arg[1] = { DEFAULT_BL_ADDR };

  if (argc != 1) {
    help_recover();
  }

  write_arg[0] = argv[0];
  write_arg[1] = DEFAULT_BL_ADDR;

  cmd_init(0, NULL);
  cmd_write(2, write_arg);
  cmd_run(1, run_arg);

  return 0;
}

int
i2c_read(usb_dev_handle * ud, int addr, void * ptr, int len)
{
  int ret;

  ret = usb_control_msg(ud, REQ_IN, REQ_I2C_READ, 0, addr,
			ptr, len, E200_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "\nControl message (%d, %s)\n", ret, strerror(-ret));
  }

  return ret;
}

int
i2c_write(usb_dev_handle * ud, int addr, void * ptr, int len)
{
  int ret;

  ret = usb_control_msg(ud, REQ_OUT, REQ_I2C_WRITE, 0, addr,
			ptr, len, E200_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "\nControl message (%d, %s)\n", ret, strerror(-ret));
  }

  return ret;
}

int
i2c_rom_seek(usb_dev_handle * ud, int addr, uint32_t pos)
{
  unsigned char tmp[2];
  int ret;

  tmp[0] = pos >> 8;
  tmp[1] = pos;

  ret = usb_control_msg(ud, REQ_OUT, REQ_I2C_WRITE, E200_TIMEOUT,
			addr, (char *)tmp, 2, E200_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "\nControl message (%d, %s)\n", ret, strerror(-ret));
  }

  return ret;
}

int
parse_i2c_addr(const char * s, int * v)
{
  if (parse_int(s, v)) {
    if (*v < 1 || *v > 127) {
      fprintf(stderr, "Invalid i2c address %d!\n", *v);
      return 0;
    }
  }

  return 1;
}

#define BYTES_PER_LINE 16

int
hexdump(uint32_t addr, void * data, int len)
{
  const unsigned char * ptr;
  int i;

  ptr = data;

  printf("%08x: ", addr);

  for (i = 0; i < len; i++) {
    printf(" %02x", ptr[i]);
  }

  for (; i < BYTES_PER_LINE; i++) {
    printf("   ");
  }

  printf("  '");

  for (i = 0; i < len; i++) {
    printf("%c", isprint(ptr[i]) ? ptr[i] : '.');
  }

  printf("'\n");

  return len;
}

void
help_i2cread()
{
  fprintf(stderr, "\nUsage:\te200tool i2cread <i2c_addr> <len>\n\n");
  exit(1);
}

int
cmd_i2cread(int argc, char * argv[])
{
  usb_dev_handle * ud;
  uint32_t addr, total;
  int i2c_addr, len, ret;

  if (argc != 2) {
    help_i2cread();
  }

  if (!parse_i2c_addr(argv[0], &i2c_addr) || !parse_uint32(argv[1], &total)) {
    exit(1);
  }

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  addr = 0;

  while (total) {
    len = MIN(total, BYTES_PER_LINE);

    ret = i2c_read(ud, i2c_addr, buffer, len);

    if (ret < 0) {
      exit(1);
    }

    hexdump(addr, buffer, len);

    addr  += len;
    total -= len;
  }

  device_release(ud);

  return 0;
}

void
help_i2cwrite()
{
  fprintf(stderr, "\nUsage:\te200tool "
	  "i2cwrite <i2c_addr> <b1> [b2] [b3] [b4]\n\n");
  exit(1);
}

int
cmd_i2cwrite(int argc, char * argv[])
{
  usb_dev_handle * ud;
  uint32_t val;
  int i2c_addr, i, ret;

  if (argc < 2 || argc > 5) {
    help_i2cwrite();
  }

  if (!parse_i2c_addr(argv[0], &i2c_addr)) {
    exit(1);
  }

  argc -= 1;
  argv += 1;

  for (i = 0; i < argc; i++) {
    if (!parse_uint32(argv[i], &val)) {
      exit(1);
    }

    if (val > 0xff) {
      fprintf(stderr, "Value too large (%s)!\n", argv[i]);
      exit(1);
    }

    buffer[i] = val;
  }

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  fprintf(stderr, "Writing %d byte%s (address=%u)\n",
	  argc, argc > 1 ? "s" : "", i2c_addr);

  ret = i2c_write(ud, i2c_addr, buffer, argc);

  fprintf(stderr, "Done!\n");

  device_release(ud);

  return 0;
}

#define I2C_ROM_ADDR   87
#define I2C_ROM_SIZE   8192
#define I2C_ROM_PAGE   32
#define I2C_ROM_BUFFER 0x40000000

void
help_i2cdump()
{
  fprintf(stderr, "\nUsage:\te200tool i2cdump <outfile>\n\n");
  exit(1);
}

int
cmd_i2cdump(int argc, char * argv[])
{
  usb_dev_handle * ud;
  uint32_t addr, total;
  int len, ret;
  char * fn;
  FILE * fp;

  if (argc != 1) {
    help_i2cdump();
  }

  if (!strcmp(argv[0], "-")) {
    fn = "stdout";
    fp = stdout;
  } else {
    fn = argv[0];
    fp = fopen(argv[0], "wb");
    if (!fp) {
      perror(argv[0]);
      exit(1);
    }
  }

  total = I2C_ROM_SIZE;
  addr  = 0;

  fprintf(stderr, "Dumping i2c rom (address=%u) range 0x0000-0x%04x to '%s'\n",
	  I2C_ROM_ADDR, total, fn);

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  ret = i2c_rom_seek(ud, I2C_ROM_ADDR, 0);

  if (ret < 0) {
    exit(1);
  }

  while (total) {
    len = MIN(total, I2C_ROM_PAGE);

    ret = i2c_read(ud, I2C_ROM_ADDR, buffer, len);

    if (ret < 0) {
      exit(1);
    }

    fwrite(buffer, 1, ret, fp);

    fprintf(stderr, "\rDumping at 0x%04x", addr);

    addr  += ret;
    total -= ret;
  }

  fprintf(stderr, "\rDumping at 0x%04x\nDump done!\n", addr);

  fclose(fp);

  device_release(ud);

  return 0;
}

int
check_file_size(FILE * fp, int size)
{
  int len, ret;

  ret = 0;

  if (fseek(fp, 0, SEEK_END) < 0) {
    perror("fseek");
    exit(1);
  }

  len = ftell(fp);

  if (len != size) {
    fprintf(stderr, "Incorrect file length %d (0x%04x)!\n", len, len);
    ret = -1;
  }

  if (fseek(fp, 0, SEEK_SET) < 0) {
    perror("fseek");
    exit(1);
  }

  return ret;
}

int
read_file(FILE * fp, void * ptr, int len, int pos)
{
  if (fseek(fp, pos, SEEK_SET) < 0) {
    perror("fseek");
    return -1;
  }

  if (len != fread(ptr, 1, len, fp)) {
    return -1;
  }

  return len;
}

void
help_i2cprogram()
{
  fprintf(stderr, "\nUsage:\te200tool i2cprogram <infile>\n\n");
  exit(1);
}

int
cmd_i2cprogram(int argc, char * argv[])
{
  usb_dev_handle * ud;
  uint32_t addr, total;
  int ret, len, todo;
  char * fn;
  FILE * fp;

  if (argc != 1) {
    help_i2cprogram();
  }

  fn = argv[0];
  fp = fopen(argv[0], "rb");
  if (!fp) {
    perror(argv[0]);
    exit(1);
  }

  total = I2C_ROM_SIZE;
  addr  = 0;

  fprintf(stderr,
	  "Programming i2c rom (address=%u) range 0x0000-0x%04x from '%s'\n",
	  I2C_ROM_ADDR, total, fn);

  if (check_file_size(fp, total) < 0) {
    exit(1);
  }

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  set_ptr(ud, I2C_ROM_BUFFER);

  while (total) {
    todo = MIN(total, BUFSIZE - 1);

    len = read_file(fp, buffer, todo, addr);

    if (len != todo) {
      perror("File reading failed!");
      exit(1);
    }

    fprintf(stderr, "\rUploading at 0x%04x", addr);

    ret = usb_control_msg(ud, REQ_OUT, REQ_WRITE, 0, 0,
			  buffer, todo, E200_TIMEOUT);

    if (ret < 0) {
      fprintf(stderr, "\nControl message (%d, %s)\n", ret, strerror(-ret));
      exit(1);
    }

    addr  += todo;
    total -= todo;
  }

  fprintf(stderr, "\rUploading at 0x%04x\nUploading done!\n", addr);

  fclose(fp);

  fprintf(stderr, "Programming, please wait...\n");

  set_ptr(ud, I2C_ROM_BUFFER);

  ret = usb_control_msg(ud, REQ_OUT, REQ_I2C_PROGRAM, I2C_ROM_SIZE,
			I2C_ROM_ADDR, NULL, 0, PROGRAM_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr,
	    "Programming failed (%d, %s)\n"
	    "*DANGER*, player might not be bootable now! Please retry!\n",
	    ret, strerror(-ret));
    exit(1);
  }

  fprintf(stderr, "Programming done!\n");

  device_release(ud);

  cmd_i2cverify(argc, argv);

  return 0;
}

void
help_i2cverify()
{
  fprintf(stderr, "\nUsage:\te200tool i2cverify <infile>\n\n");
  exit(1);
}

int
cmd_i2cverify(int argc, char * argv[])
{
  usb_dev_handle * ud;
  char tmp[I2C_ROM_PAGE];
  uint32_t addr, total;
  int i, len, ret;
  char * fn;
  FILE * fp;

  if (argc != 1) {
    help_i2cverify();
  }

  fn = argv[0];
  fp = fopen(argv[0], "rb");
  if (!fp) {
    perror(argv[0]);
    exit(1);
  }

  total = I2C_ROM_SIZE;
  addr  = 0;

  fprintf(stderr,
	  "Verifying i2c rom (address=%u) range 0x0000-0x%04x from '%s'\n",
	  I2C_ROM_ADDR, total, fn);

  if (check_file_size(fp, total) < 0) {
    exit(1);
  }

  ud = device_get(OWN_VENDOR_ID, OWN_PRODUCT_ID);

  if (!ud) {
    exit(1);
  }

  ret = i2c_rom_seek(ud, I2C_ROM_ADDR, 0);

  if (ret < 0) {
    exit(1);
  }

  while (total) {
    len = MIN(total, I2C_ROM_PAGE);

    fprintf(stderr, "\rVerifying at 0x%04x", addr);

    ret = i2c_read(ud, I2C_ROM_ADDR, buffer, len);

    if (ret < 0) {
      exit(1);
    }

    fread(tmp, 1, len, fp);

    for (i = 0; i < len; i++) {
      if (buffer[i] != tmp[i]) {
	fprintf(stderr, "\nVerify error at 0x%04x!\n", addr + i);
	exit(1);
      }
    }

    addr  += len;
    total -= len;
  }

  fprintf(stderr, "\rVerifying at 0x%04x\nVerify ok!\n", addr);

  fclose(fp);

  device_release(ud);

  return 0;
}

cmd_func
get_cmd(const char * s)
{
  int i, len, cmd;

  cmd = -1;

  len = strlen(s);

  for (i = 0; i < NUM_CMDS; i++) {
    if (!memcmp(cmd_table[i].name, s, len) && cmd_table[i].func) {
      if (cmd < 0) {
        cmd = i;
      } else {
        fprintf(stderr, "Ambiguous command, could be '%s' or '%s'!\n",
                cmd_table[cmd].name, cmd_table[i].name);
        return NULL;
      }
    }
  }

  return cmd >= 0 ? cmd_table[cmd].func : NULL;
}

int
main(int argc, char * argv[])
{
  cmd_func func;

  fprintf(stderr, "e200tool " VERSION " (c) by MrH 2006, 2007\n");

  if (argc < 2) {
    help();
  }

  usb_init();

  func = get_cmd(argv[1]);

  if (!func) {
    fprintf(stderr, "Unknown command '%s'!\n", argv[1]);
    help();
  }

  return func(argc - 2, argv + 2);
}
