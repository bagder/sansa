
#ifndef __E200_PROTOCOL_H__
#define __E200_PROTOCOL_H__

#define REQ_IN  ((1 << 7) | (2 << 5)) 
#define REQ_OUT ((0 << 7) | (2 << 5))

#define REQ_NONE        0x00
#define REQ_SETPTR      0x01
#define REQ_GETPTR      0x02
#define REQ_READ        0x03
#define REQ_WRITE       0x04
#define REQ_RUN         0x05
#define REQ_LOAD        0x06
#define REQ_STORE       0x07
#define REQ_I2C_READ    0x08
#define REQ_I2C_WRITE   0x09
#define REQ_I2C_PROGRAM 0x0a
#define REQ_POWEROFF    0x0b

#define OWN_VENDOR_ID   0x6666
#define OWN_PRODUCT_ID  0xe200
#define OWN_PRODUCT_REV 0x0001

#endif

