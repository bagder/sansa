
/*
 * mi4 (de/en)coder
 *
 * Notes:
 *        - The crypto algorithm is a 16 round variation of TEA.
 *        - So it is a 'real' crypto algorithm and not just something
 *          some idiot hacked together in a hurry.
 *        - The keys so far have been stored in the bootloader. So
 *          if new mi4 files are found, a corresponding bootloader
 *          is needed.
 *        - In the new 010301 header version the integrity check uses
 *          DSA. Unless we somehow get the private key, we just can't
 *          sign our own firmware with the original key.
 *        - We can however replace the public key in the bootloader
 *          and then sign with our own private key.
 *        - Or we can try to fool the DSA check completely. There seems
 *          to be some 'interesting' features in the DSA implementation
 *          in the bootloader.
 *        - DSA support now requires libgcrypt. It should be possible
 *          to turn it off to build mi4code without real DSA signing
 *          or verifying capability.
 *
 * Thanks to
 *
 *        Daniel Stenberg      for hosting the project.
 *        Benjamin Larsson     for the MSI P640 key + some ideas.
 *
 * (c) MrH 2006, 2007
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#define SUPPORT_DSA /* requires libgcrypt, undef if not available */

#define VERSION "v0.9.33-beta"

/* Known TEA keys */

struct tea_key {
  const char * name;
  uint32_t     key[4];
};

struct tea_key tea_keytable[] = {
  { "default" ,          { 0x20d36cc0, 0x10e8c07d, 0xc0e7dcaa, 0x107eb080 } },
  { "sansa",             { 0xe494e96e, 0x3ee32966, 0x6f48512b, 0xa93fbb42 } },
  { "sansa_gh",          { 0xd7b10538, 0xc662945b, 0x1b3fce68, 0xf389c0e6 } },
  { "rhapsody",          { 0x7aa9c8dc, 0xbed0a82a, 0x16204cc7, 0x5904ef38 } },
  { "p610",              { 0x950e83dc, 0xec4907f9, 0x023734b9, 0x10cfb7c7 } },
  { "p640",              { 0x220c5f23, 0xd04df68e, 0x431b5e25, 0x4dcc1fa1 } },
  { "virgin",            { 0xe83c29a1, 0x04862973, 0xa9b3f0d4, 0x38be2a9c } },
  { "20gc_eng",          { 0x0240772c, 0x6f3329b5, 0x3ec9a6c5, 0xb0c9e493 } },
  { "20gc_fre",          { 0xbede8817, 0xb23bfe4f, 0x80aa682d, 0xd13f598c } },
  { "elio_p722",         { 0x6af3b9f8, 0x777483f5, 0xae8181cc, 0xfa6d8a84 } },
  { "c200",              { 0xbf2d06fa, 0xf0e23d59, 0x29738132, 0xe2d04ca7 } },
};

#define NUM_TEA_KEYS (sizeof(tea_keytable) / sizeof(tea_keytable[0])) 

/* Known DSA keys */

struct dsa_key {
  const char * name;
  int          known;
  uint8_t      p[128];
  uint8_t      q[20];
  uint8_t      g[128];
  uint8_t      y[128];
  uint8_t      x[20];
};

struct dsa_key dsa_keytable[] = {
  {
    "mi4code", 1,
    {
      0xbe, 0x9e, 0x39, 0xb6, 0xae, 0x8c, 0x57, 0xe2,
      0x82, 0xfb, 0x45, 0xaf, 0x9d, 0x88, 0x25, 0x60,
      0x6f, 0x2a, 0x2b, 0x1c, 0x74, 0x6e, 0x41, 0xe0,
      0xc0, 0xfc, 0xc4, 0xb6, 0x78, 0x98, 0x54, 0x71,
      0x49, 0xbe, 0x18, 0x88, 0xdc, 0x02, 0x8a, 0x62,
      0xe5, 0xbd, 0xae, 0x62, 0xfe, 0xa6, 0x19, 0xda,
      0x5f, 0x8c, 0xa8, 0xec, 0xd0, 0x67, 0x9a, 0xf0,
      0x34, 0x45, 0x8f, 0x05, 0x50, 0xdc, 0x48, 0x9d,
      0xb4, 0xee, 0xb1, 0x1c, 0xb4, 0x73, 0x80, 0xb2,
      0x67, 0x8d, 0xd7, 0x32, 0xed, 0x6d, 0x87, 0xa3,
      0xd6, 0x15, 0xa6, 0x3c, 0xde, 0x44, 0x0a, 0x79,
      0xe6, 0xed, 0x53, 0xea, 0x8c, 0x6b, 0x1d, 0xd4,
      0x53, 0x3f, 0x3d, 0x64, 0xc2, 0x32, 0x30, 0x39,
      0x5f, 0x8e, 0x51, 0x3f, 0x09, 0xf0, 0xa2, 0x39,
      0x53, 0x1e, 0xd2, 0xa3, 0xc0, 0x9e, 0x86, 0x26,
      0x3a, 0x87, 0x4a, 0x8b, 0x4c, 0xf8, 0x46, 0xdf
    },
    {
      0xe1, 0x27, 0xd5, 0x23, 0x2c, 0x5d, 0xb9, 0x72,
      0x52, 0x44, 0xd2, 0xd7, 0xaf, 0x03, 0xee, 0x37,
      0xd5, 0x67, 0x21, 0xfd
    },
    {
      0x55, 0x06, 0x71, 0x4b, 0x0a, 0xde, 0x46, 0xb7,
      0x42, 0xde, 0xf6, 0xae, 0x96, 0xc7, 0x20, 0x06,
      0x99, 0x2c, 0x72, 0x7f, 0xd5, 0x40, 0xfd, 0xc0,
      0x40, 0xd7, 0xe5, 0xd0, 0x0e, 0x4d, 0x03, 0x0f,
      0x61, 0xfd, 0x75, 0x36, 0x61, 0x6a, 0x80, 0x8f,
      0x63, 0xa9, 0xfd, 0x50, 0x67, 0xd2, 0x35, 0xd0,
      0xea, 0xb3, 0x18, 0x6e, 0xf0, 0x62, 0xd2, 0x60,
      0x86, 0x6c, 0x62, 0x58, 0x5b, 0xc4, 0x8f, 0xf2,
      0x5f, 0x9e, 0x54, 0x3b, 0xd4, 0xcb, 0xfe, 0xc4,
      0x91, 0xe9, 0x3a, 0xb2, 0xe4, 0x12, 0x90, 0x9e,
      0xfe, 0xd5, 0xd4, 0x6e, 0x6b, 0xf8, 0x33, 0x5b,
      0xea, 0x57, 0x63, 0x43, 0xe2, 0xfc, 0x3b, 0x80,
      0xca, 0x51, 0x50, 0xa1, 0x8e, 0x51, 0x62, 0xe8,
      0x3d, 0xeb, 0x38, 0x56, 0xce, 0x4e, 0x54, 0x07,
      0x1f, 0x52, 0x35, 0x2e, 0x77, 0x33, 0xc3, 0x67,
      0x9e, 0xf3, 0x47, 0xb7, 0xe7, 0x32, 0xd9, 0x63
    },
    {
      0x3f, 0xe6, 0x4e, 0x71, 0x3c, 0x49, 0x73, 0x17,
      0xab, 0xcc, 0x06, 0x20, 0x0c, 0xff, 0x76, 0x0c,
      0xcc, 0xee, 0x2b, 0x86, 0xa0, 0x82, 0x2e, 0xed,
      0x11, 0x85, 0x04, 0x68, 0x55, 0xdb, 0x42, 0x18,
      0xd3, 0x06, 0x02, 0x58, 0xcb, 0x2f, 0x21, 0xe7,
      0x9d, 0xec, 0x1e, 0xd2, 0x8f, 0xce, 0xad, 0x58,
      0x9a, 0xb1, 0xba, 0x7e, 0x69, 0xcf, 0x4f, 0x2f,
      0x8a, 0x8f, 0x4a, 0x4f, 0xd5, 0x81, 0xbe, 0x88,
      0xa6, 0x92, 0xad, 0x9b, 0x70, 0x84, 0x84, 0x96,
      0x5f, 0xde, 0x70, 0xc6, 0xdc, 0xb8, 0x56, 0x63,
      0x9b, 0xef, 0x58, 0xf6, 0x58, 0xe6, 0xc5, 0xeb,
      0xeb, 0x1b, 0xa0, 0x7e, 0x9e, 0x52, 0xde, 0x00,
      0xf5, 0x05, 0x94, 0x05, 0x74, 0x68, 0x0e, 0x27,
      0x30, 0x1e, 0x2c, 0xb1, 0x98, 0x06, 0xdb, 0x1c,
      0x6c, 0x35, 0x9f, 0xda, 0x5a, 0x52, 0xec, 0x1b,
      0xba, 0x98, 0x59, 0xc1, 0xc3, 0x9c, 0x55, 0x8f
    },
    {
      0x0e, 0x85, 0xd8, 0x46, 0xa2, 0x69, 0x86, 0x14,
      0x6f, 0xb8, 0x42, 0x83, 0xeb, 0xbc, 0x2b, 0x94,
      0xd2, 0xb3, 0x75, 0xd7
    }
  },
  {
    "sansa", 0,
    {
      0xa8, 0x0c, 0x7c, 0x63, 0x0b, 0xd0, 0x0b, 0xd2,
      0xa1, 0x9e, 0xd4, 0xeb, 0x15, 0x1c, 0x8b, 0xd5,
      0xc1, 0x11, 0x55, 0xa3, 0x60, 0x50, 0x45, 0xdb,
      0x7d, 0x44, 0xe4, 0xb4, 0xde, 0xed, 0xcb, 0xe2,
      0xdb, 0x28, 0xd5, 0xcf, 0xc0, 0xbc, 0x44, 0x56,
      0x9d, 0x62, 0x7b, 0x5a, 0x97, 0x96, 0x13, 0xac,
      0x8c, 0x4c, 0xf7, 0x6b, 0x06, 0xa6, 0xf3, 0x9b,
      0xd5, 0xe4, 0xe0, 0x14, 0x4e, 0xb1, 0x4a, 0x5d,
      0x2f, 0xb3, 0xa0, 0x66, 0x89, 0xeb, 0x1e, 0x46,
      0xc3, 0x21, 0x65, 0x15, 0x63, 0x68, 0x7e, 0xf3,
      0x75, 0xd5, 0x5d, 0xd3, 0x70, 0x17, 0xcc, 0x97,
      0xb4, 0xce, 0xe3, 0x21, 0x0a, 0x99, 0xfc, 0xcb,
      0x60, 0x05, 0xc7, 0x13, 0xd7, 0x0b, 0x34, 0xcd,
      0x56, 0xaf, 0x8d, 0x4c, 0x59, 0xe8, 0xe3, 0xd7,
      0x2f, 0xef, 0xf1, 0x58, 0xf1, 0xa0, 0xb2, 0x07,
      0xac, 0x07, 0xce, 0x2c, 0x5d, 0xe1, 0x1d, 0x85
    },
    {
      0xa9, 0xd0, 0xdf, 0xef, 0xa3, 0x3a, 0x10, 0x65,
      0xb6, 0x4f, 0x86, 0x46, 0x39, 0xa9, 0xea, 0xc9,
      0x00, 0xea, 0x48, 0xed
    },
    {
      0x1b, 0x5b, 0xe5, 0x3e, 0xf6, 0xd6, 0x50, 0x17,
      0xc0, 0x93, 0x87, 0x73, 0x5b, 0x6d, 0x0e, 0xb0,
      0x86, 0xb9, 0x78, 0x7b, 0x4e, 0x6c, 0x2d, 0x2a,
      0xf3, 0x2c, 0x4c, 0x49, 0xac, 0xca, 0x01, 0xa4,
      0x10, 0x83, 0x1a, 0xac, 0x1c, 0x1e, 0x24, 0xbf,
      0x25, 0x2d, 0x02, 0x3b, 0x78, 0x29, 0xb6, 0xde,
      0x1b, 0x67, 0xef, 0x95, 0xc9, 0xe9, 0x60, 0x95,
      0x95, 0x4f, 0xfd, 0xb2, 0x69, 0x16, 0x78, 0xdf,
      0x8b, 0x4a, 0xf2, 0xf8, 0xbc, 0xa4, 0x31, 0x7a,
      0x1c, 0xb7, 0xc6, 0xa4, 0x0a, 0xb1, 0x36, 0xa2,
      0x31, 0x9f, 0x2b, 0x20, 0x31, 0x72, 0x65, 0xa0,
      0x4b, 0x99, 0x9a, 0xd0, 0x39, 0xb2, 0x50, 0x27,
      0x7b, 0x4f, 0x2b, 0x2f, 0x08, 0x58, 0x6a, 0x62,
      0x28, 0xb3, 0xa9, 0x9f, 0x87, 0xe9, 0x33, 0xe4,
      0x4d, 0x73, 0x52, 0x1a, 0xcf, 0x5a, 0x79, 0x98,
      0xa6, 0xd1, 0x60, 0xc6, 0x59, 0xc9, 0x6e, 0xb7
    },
    {
      0x5b, 0xb9, 0xd2, 0x9b, 0x6e, 0x29, 0x2d, 0x38,
      0xd4, 0x11, 0x12, 0x6f, 0xb0, 0x97, 0x40, 0x2d,
      0xef, 0xde, 0xd4, 0x93, 0xfc, 0x5e, 0x70, 0x5d,
      0x1f, 0xfe, 0x7f, 0xac, 0x56, 0xba, 0x48, 0x8a,
      0x9c, 0xa0, 0x39, 0x9c, 0xea, 0x8b, 0xa6, 0x2a,
      0x1d, 0x08, 0x53, 0x79, 0x6a, 0xf7, 0xeb, 0xb6,
      0xde, 0xf3, 0x58, 0x0a, 0x63, 0x89, 0x98, 0x0d,
      0x83, 0x8e, 0x3c, 0x6c, 0xd3, 0x77, 0x8e, 0x71,
      0x90, 0x2d, 0xc1, 0xe3, 0x31, 0xb5, 0x26, 0x21,
      0xb2, 0x9b, 0x47, 0x7d, 0x56, 0x9b, 0x4c, 0x87,
      0x2b, 0x74, 0xc5, 0xbc, 0xa4, 0x35, 0xc9, 0x26,
      0xe0, 0x88, 0xff, 0xc6, 0xaf, 0x69, 0x19, 0x5f,
      0x6c, 0x1d, 0xff, 0xd9, 0x79, 0x46, 0x33, 0x14,
      0xea, 0xb1, 0x2b, 0x50, 0xe4, 0xa9, 0xce, 0x9e,
      0x13, 0x4b, 0x87, 0x92, 0x4a, 0x5f, 0x65, 0x1e,
      0x83, 0x71, 0x25, 0x56, 0x6d, 0x05, 0x2f, 0x7f
    },
    {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00
    }
  },
  { "sansa_gh", 0,
    {
      0xe2, 0x63, 0xaa, 0x46, 0x26, 0x43, 0xd7, 0x78,
      0xf0, 0x4e, 0x6c, 0x91, 0x7c, 0xc1, 0xca, 0x0d,
      0xa8, 0xff, 0x1d, 0xbb, 0xcd, 0x30, 0x2a, 0x98,
      0x16, 0x9e, 0x48, 0x3a, 0x3f, 0x39, 0xbc, 0xe4,
      0xd1, 0xb6, 0xdd, 0xf7, 0x86, 0x50, 0x4b, 0x69,
      0x14, 0xa4, 0xe7, 0xd1, 0x36, 0xdd, 0xf7, 0xd5,
      0x7f, 0xf0, 0x31, 0xdb, 0xa0, 0x3b, 0x3a, 0x7b,
      0xc3, 0x88, 0xa6, 0xce, 0x03, 0x54, 0x3d, 0x8a,
      0xf6, 0x8a, 0xf0, 0x07, 0xd1, 0x0b, 0x28, 0x84,
      0xe6, 0x3a, 0xbb, 0x7b, 0x51, 0x8e, 0xb0, 0xe5,
      0x2f, 0xdd, 0x81, 0xde, 0x17, 0x0c, 0x77, 0xac,
      0x47, 0xdf, 0x0f, 0xf8, 0x2a, 0x1a, 0xf6, 0x10,
      0xb4, 0xca, 0x04, 0xa5, 0x9d, 0xda, 0xa7, 0xc3,
      0xb3, 0xf2, 0x26, 0xd0, 0x2c, 0x01, 0x80, 0x4c,
      0xf9, 0xf1, 0xb4, 0xe9, 0x20, 0x9d, 0xc8, 0xf4,
      0x79, 0x1e, 0x64, 0x8d, 0x52, 0x35, 0x9e, 0xa9
    },
    {
      0xa8, 0xca, 0xd9, 0x11, 0xab, 0xba, 0x6e, 0x7a,
      0x99, 0xce, 0x5d, 0x96, 0xf4, 0x47, 0xa2, 0xf8,
      0xb9, 0xbc, 0xf4, 0x95
    },
    {
      0x4e, 0x89, 0x34, 0x05, 0x8d, 0x6f, 0xbf, 0x5f,
      0x2f, 0x3d, 0x01, 0x81, 0xdf, 0x53, 0xbd, 0x18,
      0x9a, 0x41, 0x79, 0xfc, 0x50, 0x12, 0x3f, 0x29,
      0xbb, 0xf0, 0x04, 0x4b, 0x81, 0xd7, 0x7c, 0x36,
      0x58, 0xeb, 0x6d, 0xca, 0x5f, 0xb5, 0xac, 0xf4,
      0x95, 0xdc, 0x9d, 0x3b, 0x64, 0xba, 0x1b, 0x02,
      0xc4, 0x0a, 0xdf, 0x4f, 0xa1, 0x1f, 0x47, 0xae,
      0xe0, 0x96, 0x79, 0x14, 0x02, 0x17, 0xc0, 0x26,
      0x16, 0xf5, 0xbd, 0x13, 0x81, 0x64, 0x2c, 0xd6,
      0x2d, 0x0d, 0xf9, 0x06, 0x3e, 0xbe, 0x0b, 0x9a,
      0xdf, 0x25, 0xe9, 0xa8, 0x50, 0xcb, 0x65, 0x31,
      0x3f, 0x32, 0xc8, 0xe2, 0xc4, 0x51, 0x2a, 0x29,
      0xdc, 0x8f, 0xb2, 0x7a, 0xd7, 0x8a, 0xe4, 0x5f,
      0x68, 0xd0, 0x61, 0x7b, 0x66, 0xaf, 0x5f, 0x0c,
      0x7d, 0xc1, 0x74, 0x11, 0x8d, 0x3d, 0xdf, 0x36,
      0xcc, 0x06, 0x27, 0x19, 0xac, 0x88, 0x2f, 0xdd
    },
    {
      0x8c, 0x2f, 0x28, 0x3f, 0x94, 0x60, 0xc5, 0xb6,
      0xae, 0xa7, 0x0e, 0xea, 0x92, 0x24, 0xbd, 0x65,
      0xc1, 0xc4, 0x6c, 0xe1, 0x7e, 0xc0, 0x0b, 0x29,
      0xdd, 0xcf, 0xbe, 0x5f, 0xab, 0x7d, 0x12, 0xda,
      0x46, 0x4c, 0x57, 0xda, 0xff, 0x63, 0xd3, 0x18,
      0xab, 0x4f, 0x4e, 0xcf, 0x18, 0x93, 0x4b, 0xb8,
      0x05, 0x44, 0xc6, 0x64, 0x3a, 0xe1, 0x8d, 0x76,
      0x94, 0xc6, 0x4c, 0xa6, 0x4b, 0x16, 0xec, 0x00,
      0xcc, 0xe6, 0xc0, 0xc8, 0x73, 0x70, 0x14, 0x81,
      0xea, 0xde, 0x4e, 0xae, 0x67, 0x78, 0x61, 0x90,
      0x7d, 0x4b, 0x45, 0xca, 0x23, 0x69, 0x26, 0x68,
      0xf1, 0xa0, 0x77, 0x03, 0x09, 0x33, 0x25, 0xa1,
      0x12, 0xe8, 0x30, 0x8f, 0x30, 0xf4, 0xaf, 0xbd,
      0xea, 0xe0, 0x66, 0x59, 0x5f, 0x81, 0xd6, 0x3b,
      0x20, 0xc9, 0x80, 0x61, 0xb1, 0xa1, 0x6b, 0x81,
      0x21, 0x83, 0x5f, 0xeb, 0x43, 0x0e, 0xf1, 0xe1
    },
    {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00
    }
  },
  { "rhapsody", 0,
    {
      0xdc, 0xa3, 0xcd, 0xec, 0x52, 0x1a, 0x04, 0xc5,
      0xd4, 0xf6, 0x6f, 0x9d, 0x66, 0x31, 0x48, 0xd4,
      0x3b, 0xe8, 0x2c, 0x18, 0x33, 0x66, 0x11, 0x22,
      0x05, 0xf5, 0xce, 0x09, 0xaf, 0x63, 0x9b, 0x45,
      0xc0, 0x74, 0x5d, 0x2f, 0xd0, 0x53, 0x4f, 0xbd,
      0xdf, 0xe5, 0xa0, 0x47, 0xc1, 0xe5, 0x18, 0x11,
      0xee, 0x70, 0x57, 0x37, 0xeb, 0x8d, 0x48, 0xe5,
      0x91, 0x09, 0x82, 0xf3, 0x20, 0x95, 0x44, 0xbb,
      0xcf, 0xc2, 0x2f, 0x47, 0x65, 0x4b, 0x3c, 0xf2,
      0xe4, 0xbf, 0x7c, 0x92, 0x22, 0xd6, 0x54, 0xb7,
      0x2c, 0x15, 0x19, 0x99, 0x10, 0x54, 0xe6, 0x9b,
      0x50, 0xf3, 0xdc, 0xae, 0x7f, 0x35, 0xca, 0xeb,
      0xe8, 0x51, 0xa5, 0x9a, 0xfc, 0x9b, 0x30, 0x58,
      0x5a, 0xe9, 0x17, 0xc8, 0x58, 0x19, 0x51, 0x92,
      0xa6, 0xe3, 0xd6, 0x50, 0x0b, 0x43, 0x02, 0x6f,
      0x5c, 0x0c, 0x71, 0x6f, 0x66, 0x17, 0xb6, 0xf5
    },
    {
      0x87, 0xa0, 0x96, 0xfe, 0x31, 0x4e, 0x8e, 0xe0,
      0x5c, 0xcf, 0xc0, 0x0d, 0x2c, 0xa9, 0xe8, 0xd9,
      0x3d, 0x9f, 0x9c, 0x63
    },
    {
      0x47, 0xee, 0xad, 0x3c, 0x5f, 0x41, 0xe3, 0x46,
      0x75, 0xa9, 0x51, 0xeb, 0xed, 0xbc, 0xb3, 0xe1,
      0x4e, 0x99, 0x05, 0x62, 0x37, 0xc6, 0xd5, 0x53,
      0x2c, 0x5f, 0xd9, 0xf0, 0xe9, 0xcd, 0x18, 0x25,
      0xc2, 0x5b, 0x45, 0xc8, 0xfe, 0x6f, 0x6c, 0x63,
      0x1f, 0xe2, 0x50, 0xeb, 0xba, 0x95, 0x08, 0xb8,
      0x35, 0x1a, 0x83, 0x30, 0xd2, 0x3d, 0x64, 0x6f,
      0xae, 0xec, 0x9f, 0x69, 0xbb, 0xa1, 0x89, 0xcd,
      0x49, 0x4c, 0x8a, 0x71, 0x4f, 0xb1, 0x78, 0xc9,
      0x05, 0x75, 0x27, 0xcb, 0x96, 0xe3, 0x48, 0x84,
      0xf2, 0xb6, 0xec, 0x07, 0x89, 0x8a, 0x6e, 0xa6,
      0xf3, 0x38, 0x3e, 0xb8, 0x50, 0x2c, 0xa3, 0x4e,
      0x31, 0xf8, 0xaa, 0x1f, 0xe9, 0xea, 0xc4, 0x88,
      0xaf, 0xd9, 0x1e, 0xa2, 0x9d, 0xeb, 0x03, 0x07,
      0x83, 0x73, 0xd9, 0x79, 0xbd, 0xa8, 0xf7, 0x14,
      0xd1, 0xbf, 0xbf, 0x1a, 0x92, 0x3a, 0x84, 0x7e
    },
    {
      0x2d, 0x77, 0x4f, 0xb2, 0x52, 0x21, 0x92, 0x52,
      0xf9, 0x9a, 0xa5, 0xb9, 0x81, 0xbf, 0x7e, 0x38,
      0x60, 0xa0, 0x0a, 0x93, 0x0a, 0x3f, 0xec, 0xc6,
      0x24, 0x70, 0xce, 0x8c, 0x1f, 0xf4, 0x1b, 0x2a,
      0xdd, 0xf4, 0x4a, 0xaa, 0x79, 0xf6, 0x84, 0xc0,
      0x2b, 0x7b, 0x55, 0x4b, 0xec, 0x23, 0x2c, 0x9f,
      0xe7, 0x70, 0xf5, 0x68, 0xea, 0xf8, 0xf9, 0x09,
      0x7c, 0x4f, 0x3a, 0x70, 0x7c, 0xb9, 0x7b, 0xf9,
      0x7d, 0x79, 0x42, 0xfc, 0xd8, 0xfd, 0x54, 0xa4,
      0x3e, 0xb6, 0xe4, 0x9a, 0xff, 0x0b, 0x2b, 0x59,
      0x5c, 0xe5, 0x39, 0x35, 0xb7, 0x30, 0xfe, 0xa0,
      0x56, 0xc5, 0x5e, 0xbb, 0x52, 0x95, 0xbf, 0xf4,
      0x59, 0xf4, 0x0b, 0xc4, 0x24, 0xe9, 0x35, 0x17,
      0x6c, 0x10, 0xc6, 0xb4, 0x69, 0xf2, 0xf3, 0x61,
      0x86, 0x54, 0x9a, 0x94, 0x43, 0x1c, 0x1c, 0xa5,
      0xd5, 0x20, 0xfa, 0xcb, 0xbc, 0x88, 0xef, 0x05
    },
    {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00
    }
  },
  {
    "20gc_eng", 0,
    {
      0xfa, 0xdb, 0x17, 0x9c, 0x7a, 0x07, 0x70, 0xb7,
      0x2e, 0x8b, 0xdf, 0x33, 0x82, 0x1c, 0x00, 0xa8,
      0x31, 0xce, 0x64, 0x8f, 0xa4, 0x64, 0xb4, 0x0b,
      0x23, 0x01, 0x0d, 0xcd, 0x49, 0x68, 0x1e, 0x32,
      0xf6, 0x07, 0x89, 0xa7, 0x89, 0x8b, 0x40, 0xca,
      0x7e, 0xd9, 0x78, 0xe7, 0x11, 0x1b, 0x17, 0x58,
      0x9f, 0x90, 0xea, 0x2a, 0x01, 0xfd, 0xce, 0xfe,
      0x17, 0x63, 0x9b, 0xf1, 0xdd, 0x18, 0x30, 0xc7,
      0xfa, 0x10, 0x12, 0xb6, 0x82, 0x18, 0x0f, 0xf5,
      0xc1, 0xb1, 0x88, 0xdb, 0xbb, 0xbd, 0x57, 0xcf,
      0x93, 0x32, 0xc2, 0x35, 0x4d, 0x72, 0x92, 0xbb,
      0x7c, 0xcd, 0xaa, 0x9c, 0x33, 0x1a, 0x41, 0xb0,
      0xd9, 0x58, 0x0d, 0xbd, 0xc1, 0x2f, 0xb8, 0x1f,
      0x45, 0xf4, 0x06, 0xc5, 0x61, 0x24, 0x2d, 0x6e,
      0x4a, 0xe8, 0x55, 0x24, 0x41, 0x58, 0x05, 0xbf,
      0x21, 0x75, 0x5b, 0x77, 0x88, 0x10, 0x93, 0x5d
    },
    {
      0xad, 0xf1, 0xbe, 0x09, 0x32, 0x14, 0x76, 0x7c,
      0x0b, 0xae, 0xc4, 0x74, 0xf5, 0xf7, 0xb1, 0x8b,
      0x20, 0xd1, 0xac, 0x65
    },
    {
      0x4d, 0xce, 0xf4, 0x78, 0x5d, 0x75, 0xed, 0x39,
      0xd5, 0x58, 0x70, 0xfe, 0x1e, 0x25, 0x91, 0x9c,
      0xaf, 0x20, 0x4f, 0x53, 0x38, 0x42, 0xea, 0x71,
      0x26, 0x63, 0xe9, 0xcf, 0x54, 0x31, 0xe6, 0x77,
      0xbd, 0x70, 0xd5, 0xcd, 0x94, 0x80, 0xcf, 0x05,
      0xa3, 0x92, 0xac, 0x20, 0x77, 0xf2, 0x0f, 0xe5,
      0xa1, 0x09, 0x90, 0xe0, 0x4b, 0xb6, 0x4b, 0x3c,
      0x59, 0xdb, 0x2b, 0x63, 0x04, 0x15, 0xa0, 0xe9,
      0xfc, 0x64, 0x94, 0x9e, 0x82, 0x8b, 0xa7, 0x3f,
      0xa1, 0x80, 0x67, 0xfd, 0xf9, 0xc9, 0x30, 0x72,
      0x2f, 0x9b, 0x2a, 0x37, 0xa2, 0xa4, 0x34, 0xc7,
      0x87, 0x0d, 0x61, 0x37, 0x05, 0xa8, 0x91, 0x45,
      0xde, 0xe6, 0xc1, 0xad, 0x93, 0xb3, 0x01, 0x84,
      0x1b, 0x99, 0xa2, 0x79, 0xd5, 0xe9, 0x75, 0x49,
      0x3e, 0x11, 0xd3, 0x3c, 0xaf, 0x18, 0xe8, 0x64,
      0x9e, 0x1d, 0xf6, 0xd7, 0x4e, 0x77, 0xb2, 0xc8 
    },
    {
      0x18, 0x8c, 0xcd, 0x79, 0x0d, 0x2e, 0x5c, 0xd7,
      0x82, 0x53, 0x69, 0xf2, 0x21, 0x15, 0x8d, 0x3e,
      0x77, 0xae, 0xb5, 0x34, 0x3b, 0x1b, 0x4c, 0xe0,
      0x67, 0xbe, 0x05, 0x99, 0x70, 0xd4, 0x53, 0x63,
      0xb4, 0x84, 0xd1, 0xa7, 0xc0, 0xb3, 0xcb, 0xde,
      0xc1, 0x25, 0x37, 0x81, 0xb8, 0x2c, 0x4b, 0x26,
      0x85, 0xdb, 0xf9, 0xdd, 0xbd, 0xb6, 0x04, 0x57,
      0xdd, 0xf0, 0xff, 0x74, 0xfb, 0x16, 0x71, 0xd6,
      0xab, 0xcc, 0xfd, 0x05, 0x95, 0xd9, 0x77, 0xe5,
      0x86, 0x06, 0x57, 0x1c, 0x7d, 0x37, 0xc3, 0x49,
      0xcd, 0xcc, 0x60, 0x80, 0x12, 0x86, 0xc7, 0xac,
      0xd5, 0xac, 0xd0, 0x90, 0x94, 0x0c, 0x05, 0xbc,
      0x6c, 0x58, 0xdf, 0x3d, 0x51, 0xee, 0x92, 0x28,
      0x39, 0x7b, 0xc1, 0x5f, 0x5e, 0xb9, 0x83, 0x59,
      0xad, 0xf3, 0xfb, 0xe6, 0xd9, 0x37, 0xc8, 0x8b,
      0xe7, 0x24, 0x65, 0xb1, 0xaa, 0xdd, 0x1e, 0x82 
    },
    {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00
    }
  },
  {
    "20gc_fre", 0,
    {
      0xa1, 0xe2, 0x86, 0x4b, 0x4b, 0x73, 0x73, 0x5a,
      0x52, 0xb8, 0x5b, 0x79, 0xdb, 0xd6, 0xc9, 0x19,
      0x98, 0xb4, 0x19, 0x24, 0xa6, 0x31, 0x68, 0x2f,
      0x42, 0xe5, 0x16, 0x64, 0xd3, 0xb4, 0xf7, 0x6d,
      0x8c, 0xf4, 0x51, 0x01, 0x91, 0xbb, 0xbe, 0x5d,
      0xe5, 0x2f, 0x49, 0xe0, 0xfd, 0x20, 0x7f, 0xcf,
      0xa7, 0xc0, 0xd0, 0x4f, 0x6d, 0x54, 0x58, 0xa7,
      0xf0, 0x6b, 0x51, 0x19, 0x72, 0x5f, 0x4b, 0x27,
      0xe9, 0xdf, 0x4a, 0x2e, 0xce, 0xa9, 0x32, 0xc7,
      0x65, 0x61, 0x45, 0xba, 0x7a, 0x48, 0x8b, 0xe4,
      0x84, 0x68, 0xb0, 0x68, 0x03, 0x65, 0x55, 0x23,
      0x67, 0x58, 0xe3, 0x22, 0x1e, 0x02, 0xb4, 0x50,
      0xf2, 0x74, 0x9a, 0x8b, 0x5d, 0x8e, 0x11, 0xa3,
      0x66, 0xf4, 0x12, 0xfa, 0x91, 0x70, 0xf6, 0x3a,
      0x47, 0x8c, 0xf6, 0x10, 0xa1, 0x22, 0xc5, 0xb5,
      0xa4, 0xa0, 0x6f, 0x77, 0x2f, 0x90, 0x41, 0xcb
    },
    {
      0x99, 0xe7, 0xd9, 0x89, 0x34, 0x0b, 0xf4, 0x35,
      0xfb, 0xb8, 0xaf, 0x05, 0x55, 0xb6, 0x30, 0x9a,
      0x47, 0xba, 0xd4, 0xb1
    },
    {
      0x04, 0xea, 0x5e, 0x31, 0x8a, 0xd3, 0x57, 0x1a,
      0x9a, 0xd2, 0x24, 0x78, 0xfa, 0xd7, 0xd5, 0x35,
      0x46, 0xcf, 0x4a, 0xc4, 0xb9, 0x8d, 0x5d, 0x80,
      0x3d, 0x23, 0xc5, 0x42, 0x06, 0x38, 0x59, 0x0c,
      0x32, 0x2e, 0x52, 0xbd, 0x54, 0xd0, 0x40, 0xee,
      0x78, 0x02, 0xb5, 0x5d, 0xd4, 0x87, 0xe2, 0xe7,
      0x69, 0x90, 0x5a, 0x8b, 0xa1, 0x39, 0xcb, 0x40,
      0x5f, 0xb9, 0xf0, 0xf7, 0x3d, 0x97, 0x32, 0x74,
      0x20, 0x45, 0xeb, 0xbf, 0xcc, 0xd7, 0xf7, 0x32,
      0x8a, 0x06, 0x1b, 0x35, 0xac, 0xaf, 0x12, 0xdb,
      0x4e, 0x63, 0x57, 0x46, 0x64, 0xbf, 0x3b, 0xe3,
      0x1a, 0xec, 0x50, 0x11, 0x39, 0x12, 0x8f, 0x8a,
      0xe7, 0x58, 0xfa, 0xd5, 0x66, 0x30, 0x91, 0x3f,
      0x81, 0x2c, 0x14, 0xa7, 0x21, 0xc3, 0x16, 0xd7,
      0x5e, 0xf6, 0xc8, 0x09, 0x1f, 0x8b, 0x77, 0x99,
      0x7e, 0x38, 0xcb, 0x08, 0xc1, 0x10, 0xb1, 0xd7
    },
    {
      0x11, 0x82, 0x78, 0x83, 0x8d, 0x13, 0x49, 0xef,
      0x86, 0x12, 0xa5, 0x5f, 0x34, 0x87, 0x02, 0x72,
      0x04, 0x51, 0x6d, 0x77, 0x18, 0xbb, 0xf8, 0x23,
      0xe0, 0x1e, 0xc8, 0x4d, 0x2e, 0x7d, 0x9b, 0x4c,
      0xf0, 0xfc, 0x20, 0x7c, 0xe8, 0x83, 0x20, 0xb2,
      0x18, 0x45, 0x0e, 0x91, 0x47, 0x4e, 0x46, 0xa7,
      0x5c, 0xe9, 0x28, 0xff, 0x8a, 0xd5, 0xc2, 0xc3,
      0xca, 0x40, 0xa0, 0xe0, 0x83, 0xca, 0x48, 0x1d,
      0x15, 0x4c, 0xe4, 0xce, 0xf1, 0x11, 0x27, 0xcb,
      0x4a, 0x63, 0xdc, 0x35, 0xe0, 0x40, 0xbd, 0x46,
      0x6e, 0x08, 0x14, 0x60, 0x9f, 0xd2, 0x22, 0xde,
      0x73, 0x9e, 0x6e, 0xc2, 0xf0, 0x7e, 0xa0, 0xc7,
      0x48, 0xa3, 0xa2, 0x30, 0x8c, 0xb0, 0xcf, 0xd5,
      0x7c, 0xdf, 0x2e, 0xdf, 0xc7, 0xe2, 0x4a, 0x2b,
      0x1a, 0xf0, 0x2c, 0xe8, 0x89, 0x88, 0xb2, 0x94,
      0x23, 0xfa, 0x92, 0x5c, 0x88, 0x06, 0x3a, 0xe4
    }
  },
  { "c200", 0,
    {
      0xcc, 0x4a, 0x93, 0x54, 0x92, 0xaf, 0x53, 0x4a,
      0x35, 0xfe, 0xf5, 0x1f, 0xda, 0xe8, 0x9a, 0x44,
      0x79, 0x67, 0xb8, 0xb0, 0xc4, 0xbe, 0x54, 0x9b,
      0xfe, 0x90, 0x9e, 0x2b, 0x38, 0x3f, 0xdc, 0xc9,
      0x9e, 0x5a, 0x21, 0x97, 0x76, 0x78, 0xb1, 0xa0,
      0x65, 0x90, 0xb4, 0x19, 0x94, 0xc6, 0xee, 0xac,
      0x37, 0x85, 0x1a, 0x5e, 0xda, 0xd2, 0x38, 0xbf,
      0x1c, 0x5e, 0x62, 0x6b, 0x7e, 0x18, 0xbd, 0x54,
      0x2d, 0xf8, 0x42, 0x48, 0xf1, 0x17, 0x79, 0x98,
      0x0e, 0xe3, 0xba, 0x11, 0xbd, 0x2d, 0xc7, 0xd8,
      0x79, 0xcc, 0x08, 0x5d, 0x07, 0x5b, 0x81, 0x23,
      0x13, 0xf3, 0x93, 0x02, 0x36, 0x85, 0xa2, 0x85,
      0x66, 0x9c, 0x19, 0x7f, 0x41, 0x24, 0x62, 0xc8,
      0x0a, 0x41, 0xad, 0x0a, 0x4c, 0x47, 0x13, 0x18,
      0xc8, 0x22, 0xb4, 0x11, 0x5e, 0xc9, 0x3e, 0xc2,
      0x1b, 0xd9, 0xd6, 0x0b, 0x53, 0x6a, 0x52, 0x93
    },
    {
      0xed, 0x8c, 0x60, 0x8f, 0xfe, 0x6f, 0xeb, 0xdf,
      0xbd, 0x6c, 0xdf, 0x61, 0x98, 0x85, 0xe4, 0x66,
      0x84, 0x63, 0xae, 0x99
    },
    {
      0x5e, 0xd6, 0xde, 0x19, 0x4d, 0xc3, 0xea, 0x65,
      0xe7, 0x87, 0x7a, 0x81, 0x01, 0xec, 0x58, 0x9d,
      0xf7, 0xab, 0xf8, 0x08, 0x0e, 0x9c, 0x1f, 0x1a,
      0xce, 0x81, 0x5a, 0xf3, 0x62, 0x0f, 0xa8, 0x72,
      0x43, 0xb4, 0xf3, 0x1f, 0x9b, 0xe3, 0x8b, 0x05,
      0x62, 0x36, 0xcc, 0x01, 0x72, 0x5d, 0x5e, 0x0c,
      0x58, 0xd2, 0x76, 0x06, 0x0e, 0x03, 0x6f, 0xb5,
      0xc6, 0xad, 0x29, 0x2a, 0xe1, 0x57, 0x59, 0xe1,
      0x46, 0xde, 0x36, 0x7b, 0x5c, 0xde, 0xa7, 0x63,
      0x70, 0x10, 0xd0, 0x6f, 0x65, 0xb4, 0x04, 0xfb,
      0xe0, 0x25, 0x46, 0xf9, 0xd6, 0x93, 0xd5, 0x04,
      0x8b, 0x31, 0x66, 0x6f, 0xb1, 0x7a, 0xe9, 0xae,
      0x26, 0x7e, 0x3e, 0xd9, 0x41, 0x8f, 0x5d, 0x3d,
      0xe7, 0x17, 0xe1, 0x58, 0x24, 0x64, 0x46, 0xa5,
      0x68, 0x49, 0x66, 0x2f, 0x6c, 0xb3, 0x3c, 0x47,
      0x74, 0x5a, 0x10, 0x3e, 0x35, 0xdf, 0xe0, 0x33
    },
    {
      0x6d, 0x00, 0xd4, 0x8e, 0x0e, 0xa1, 0x37, 0x32,
      0x83, 0xe3, 0x15, 0x13, 0x2a, 0xf5, 0x41, 0x7d, 
      0xcf, 0xb9, 0xde, 0x96, 0xd8, 0xb9, 0xed, 0x99,
      0xc4, 0xb7, 0xf7, 0x0c, 0x9a, 0x54, 0x4b, 0x93,
      0x3a, 0x93, 0x83, 0xb2, 0x99, 0x84, 0x60, 0xa7,
      0x3e, 0x95, 0xc1, 0x1c, 0x8c, 0xf2, 0xb8, 0xa5,
      0xfd, 0x19, 0x69, 0xf9, 0x57, 0xa5, 0xfc, 0x07,
      0xb6, 0xc3, 0x90, 0x39, 0x35, 0xe3, 0x2c, 0x47,
      0xd1, 0x20, 0x35, 0x77, 0x04, 0xdd, 0xed, 0xa4,
      0x9b, 0x2f, 0x49, 0x14, 0xab, 0xf4, 0xdf, 0xa6,
      0xbf, 0x78, 0x8b, 0x4d, 0x19, 0xf4, 0x8f, 0x79,
      0xa0, 0x08, 0x65, 0x7e, 0x9b, 0xca, 0x6f, 0x9a,
      0xb6, 0x25, 0xe9, 0x82, 0xfa, 0x3c, 0x80, 0x38,
      0xfc, 0xa8, 0xb3, 0x9d, 0x5a, 0x2f, 0x93, 0x03,
      0x89, 0xf9, 0x54, 0xc5, 0xa8, 0xb5, 0x6c, 0xd4,
      0xfd, 0x14, 0x4a, 0x02, 0xcb, 0x9d, 0xd7, 0xb9
    },
    {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00
    }
  }
};

#define NUM_DSA_KEYS (sizeof(dsa_keytable) / sizeof(dsa_keytable[0]))

/* Known HEX keys */

#define HEX_BUFSIZE 512
#define HEX_BLKSIZE (HEX_BUFSIZE / 4)
#define HEX_KEYSIZE (HEX_BLKSIZE / 2)

struct hex_key {
  const char * name;
  uint8_t      key[HEX_KEYSIZE];
};

struct hex_key hex_keytable[] = {
  {
    "20gc_eng",
    {
      0x07, 0x2b, 0x00, 0x18, 0x11, 0x26, 0x1a, 0x33,
      0x2f, 0x0d, 0x39, 0x3b, 0x02, 0x19, 0x35, 0x1b,
      0x32, 0x34, 0x0f, 0x21, 0x0a, 0x08, 0x03, 0x1f,
      0x0e, 0x23, 0x09, 0x13, 0x06, 0x29, 0x12, 0x3e,
      0x0c, 0x10, 0x05, 0x31, 0x0b, 0x37, 0x2d, 0x22,
      0x20, 0x3a, 0x24, 0x2a, 0x1d, 0x01, 0x1c, 0x3c,
      0x28, 0x27, 0x2e, 0x3d, 0x2c, 0x36, 0x38, 0x30,
      0x25, 0x3f, 0x17, 0x14, 0x1e, 0x16, 0x15, 0x04
    }
  },
  {
    "20gc_fre",
    {
      0x05, 0x0b, 0x3d, 0x23, 0x07, 0x3c, 0x36, 0x34,
      0x31, 0x09, 0x18, 0x1f, 0x3b, 0x3f, 0x21, 0x24,
      0x03, 0x06, 0x1c, 0x16, 0x37, 0x1b, 0x20, 0x0e,
      0x2c, 0x2a, 0x0c, 0x1e, 0x10, 0x22, 0x2e, 0x32,
      0x00, 0x2b, 0x1a, 0x30, 0x0f, 0x15, 0x13, 0x3a,
      0x38, 0x3e, 0x26, 0x19, 0x2d, 0x01, 0x02, 0x25,
      0x11, 0x14, 0x1d, 0x17, 0x28, 0x08, 0x04, 0x0a,
      0x29, 0x35, 0x0d, 0x2f, 0x33, 0x27, 0x12, 0x39
    }
  }
};

#define NUM_HEX_KEYS (sizeof(hex_keytable) / sizeof(hex_keytable[0]))

/* Known label end offsets */

struct le_offs {
  const char * name;
  uint32_t     offs;
};

struct le_offs le_offs_table[] = {
  { "default",  0xec },
  { "rhapsody", 0xfc }
};

#define NUM_LE_OFFS (sizeof(le_offs_table) / sizeof(le_offs_table[0]))

#define LABEL_MAGIC 0x100

/* Known commands */

typedef int (*cmd_func)(int, char **);

struct cmd {
  const char * name;
  cmd_func     func;
  const char * desc;
};

int cmd_decrypt(int, char **);
int cmd_encrypt(int, char **);
int cmd_build(int, char **);
int cmd_sign(int, char **);
int cmd_verify(int, char **);
int cmd_keyscan(int, char **);
int cmd_blpatch(int, char **);
int cmd_hexdec(int, char **);
int cmd_hexenc(int, char **);

struct cmd cmd_table[] = {
  { "decrypt",   cmd_decrypt, "decrypt mi4 image" },
  { "encrypt",   cmd_encrypt, "encrypt mi4 image" },
  { "build",     cmd_build,   "build mi4 image" },
  { "sign",      cmd_sign,    "sign mi4 with DSA" },
  { "verify",    cmd_verify,  "verify DSA siganture" },
  { "keyscan",   cmd_keyscan, "scan file for potential keys" },
  { "blpatch",   cmd_blpatch, "patch bootloader with custom DSA key" },
  { "hexdecode", cmd_hexdec,  "decode 'hex' bootloader file" },
  { "hexencode", cmd_hexenc,  "encode 'hex' bootloader file" },
};

#define NUM_CMDS (sizeof(cmd_table) / sizeof(cmd_table[0]))

void
help()
{
  int i;

  printf("\n"
	 "Usage:\tmi4code <command> [options] [arg1] ...\n"
	 "\n"
	 "commands:\n");

  for (i = 0; i < NUM_CMDS; i++) {
    printf("\t%-20.20s%s\n", cmd_table[i].name, cmd_table[i].desc);
  }

  printf("\nUse 'mi4code <command> -h' for help on specific command\n\n");

  exit(1);
}

/* Big endian machines do exist too. */

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

void
put_le32(void * ptr, uint32_t val)
{
  uint8_t * b = ptr;

  b[0] = val;
  b[1] = val >> 8;
  b[2] = val >> 16;
  b[3] = val >> 24;
}

uint32_t crc32table[] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
  0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
  0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
  0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
  0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
  0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
  0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
  0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
  0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
  0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
  0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
  0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
  0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
  0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
  0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
  0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
  0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
  0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
  0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
  0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
  0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t
update_crc32(void * ptr, int size, uint32_t crc)
{
  uint8_t * p = ptr;

  while (size--) {
    crc = (crc >> 8) ^ crc32table[(crc ^ *p++) & 0xff];
  }

  return crc;
}

uint32_t
update_sum32(void * ptr, int size, uint32_t sum)
{
  uint32_t * d = ptr;

  while (size > 0) {
    sum += get_le32(d++);
    size -= 4;
  }

  return sum;
}

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

enum verbosity {
  V_ALL   = 0,
  V_LOW   = 1,
  V_MED   = 2,
  V_HIGH  = 3,
  V_DEBUG = 4
};

int verbose;

int
printv(int lvl, const char * fmt, ...)
{
  va_list ap;
  int r = 0;

  if (verbose >= lvl) {
    va_start(ap, fmt);
    r = vprintf(fmt, ap);
    va_end(ap);
  }

  return r;
}

#define BUFSIZE 0x10000

uint32_t inbuf[BUFSIZE / 4];
uint32_t outbuf[BUFSIZE / 4];

#define MI4MAGIC 0x534f5050 /* 'PPOS' */

struct mi4hdr {
  uint32_t magic;
  uint32_t version;
  uint32_t datalen;
  uint32_t crc32;
  uint32_t crypto;
  uint32_t totallen;
  uint32_t plainlen;
  uint32_t dsa_r[5];
  uint32_t dsa_s[5];
  uint32_t pad[111];     /* The size has to be 512 bytes */
};

struct mi4hdr mi4hdr;

int
mi4hdr_read(FILE * ifp)
{
  fseek(ifp, 0, SEEK_SET);
  fread(&mi4hdr, 1, sizeof(mi4hdr), ifp);

  mi4hdr.magic    = get_le32(&mi4hdr.magic);
  mi4hdr.version  = get_le32(&mi4hdr.version);
  mi4hdr.datalen  = get_le32(&mi4hdr.datalen);
  mi4hdr.crc32    = get_le32(&mi4hdr.crc32);
  mi4hdr.crypto   = get_le32(&mi4hdr.crypto);
  mi4hdr.totallen = get_le32(&mi4hdr.totallen);
  mi4hdr.plainlen = get_le32(&mi4hdr.plainlen);

  if (mi4hdr.magic != MI4MAGIC) {
    fprintf(stderr, "Invalid magic in mi4 header (maybe not mi4 file)!\n");
    return 0;
  }

  return 1;
}

void
mi4hdr_write(FILE * ofp)
{
  struct mi4hdr tmp;

  memcpy(&tmp, &mi4hdr, sizeof(tmp));

  put_le32(&tmp.magic, mi4hdr.magic);
  put_le32(&tmp.version, mi4hdr.version);
  put_le32(&tmp.datalen, mi4hdr.datalen);
  put_le32(&tmp.crc32, mi4hdr.crc32);
  put_le32(&tmp.crypto, mi4hdr.crypto);
  put_le32(&tmp.totallen, mi4hdr.totallen);
  put_le32(&tmp.plainlen, mi4hdr.plainlen);

  fseek(ofp, 0, SEEK_SET);
  fwrite(&tmp, 1, sizeof(tmp), ofp);
}

int
mi4hdr_dsa_check()
{
  if (mi4hdr.version != 0x010301) {
    fprintf(stderr,
	    "The mi4 has version %06x header. Only version 010301 uses DSA!\n"
	    "Nothing to do!\n", mi4hdr.version);
    return 0;
  }

  return 1;
}

struct hexhdr {
  uint32_t id[8];
  uint32_t sum1;
  uint32_t len;
  uint32_t sum2;
  uint32_t pad[5]; /* the size must be 64 bytes */
};

struct hexhdr hexhdr;

int
hexhdr_read(FILE * ifp)
{
  fseek(ifp, 0, SEEK_SET);
  fread(&hexhdr, 1, sizeof(hexhdr), ifp);

  if (memcmp(hexhdr.id, "iriver", 6)) {
    fprintf(stderr, "Invalid file header (maybe not a iriver hex file)!\n");
    return 0;
  }

  hexhdr.sum1 = get_le32(&hexhdr.sum1);
  hexhdr.sum2 = get_le32(&hexhdr.sum2);
  hexhdr.len  = get_le32(&hexhdr.len);

  return 1;
}

void
hexhdr_write(FILE * ofp)
{
  struct hexhdr tmp;

  tmp = hexhdr;

  put_le32(&tmp.sum1, hexhdr.sum1);
  put_le32(&tmp.sum2, hexhdr.sum2);
  put_le32(&tmp.len,  hexhdr.len);

  fseek(ofp, 0, SEEK_SET);
  fwrite(&tmp, 1, sizeof(tmp), ofp);
}


#ifdef SUPPORT_DSA
#include <gcrypt.h>

void
init_dsa()
{
  gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);
}

int
get_sha1(FILE * fp, uint8_t * hash)
{
  gcry_md_hd_t md_handle;
  gcry_error_t error;
  long pos;
  int len;

  error = gcry_md_open(&md_handle, GCRY_MD_SHA1, 0);

  if (error) {
    fprintf(stderr, "%s\n", gcry_strerror(error));
    return 0;
  }

  pos = ftell(fp);

  fseek(fp, sizeof(struct mi4hdr), SEEK_SET);

  while ((len = fread(inbuf, 1, BUFSIZE, fp)) > 0) {
    gcry_md_write(md_handle, inbuf, len);
  }

  memcpy(hash, gcry_md_read(md_handle, GCRY_MD_SHA1), 20);

  gcry_md_close(md_handle);

  fseek(fp, pos, SEEK_SET);

  return 1;
}

void
get_mpi(gcry_mpi_t * mpi, void * buf, int len)
{
  gcry_error_t error;

  error = gcry_mpi_scan(mpi, GCRYMPI_FMT_USG, buf, len, NULL);

  if (error) {
    fprintf(stderr, "Error %s getting mpi\n", gcry_strerror(error));
    exit(1);
  }

  gcry_mpi_set_flag(*mpi, GCRYMPI_FLAG_SECURE);
}

void
get_signature(void * ptr, gcry_sexp_t sexp, const char * part)
{
  gcry_sexp_t tmp;
  const char * src;
  char * dst;
  size_t len;

  dst = ptr;

  tmp = gcry_sexp_find_token(sexp, part, 1);

  if (!tmp) {
    fprintf(stderr, "No '%s' in signature!?\n", part);
    exit(1);
  }

  src = gcry_sexp_nth_data(tmp, 1, &len);

  if (!src) {
    fprintf(stderr, "No data in '%s' part of signature!?\n", part);
    exit(1);
  }

  if (len < 20) {
    memset(dst, 0, 20 - len);
    dst += 20 - len;
  } else if (len > 20) {
    if (len == 21 && src[0] == 0) {
      src++;
      len--;
    } else {
      fprintf(stderr, "Invalid '%s' in signature (len = %d)!?\n",
	      part, (int)len);
      gcry_sexp_dump(tmp);
      exit(1);
    }
  }

  memcpy(dst, src, len);
}

void
sign_image(FILE * fp, void * p, void * q, void * g, void * y,
	   void * x, void * r, void * s)
{
  uint8_t hash[20];
  gcry_mpi_t h_mpi, p_mpi, q_mpi, g_mpi, y_mpi, x_mpi;
  gcry_sexp_t key_exp, hash_exp, sig_exp;
  gcry_error_t error;

  get_sha1(fp, hash);

  get_mpi(&h_mpi, hash, 20);
  get_mpi(&p_mpi, p, 128);
  get_mpi(&q_mpi, q, 20);
  get_mpi(&g_mpi, g, 128);
  get_mpi(&y_mpi, y, 128);
  get_mpi(&x_mpi, x, 20);

  error = gcry_sexp_build(&key_exp, NULL,
			  "(private-key "
			  "(dsa (p %m) (q %m) (g %m) (y %m) (x %m)))",
			  p_mpi, q_mpi, g_mpi, y_mpi, x_mpi);

  if (error) {
    fprintf(stderr, "Error %s building private key\n", gcry_strerror(error));
    exit(1);
  }

  error = gcry_sexp_build(&hash_exp, NULL,
			  "(data (flags raw) (value %m))", h_mpi);

  if (error) {
    fprintf(stderr, "Error %s building hash\n", gcry_strerror(error));
    exit(1);
  }

  error = gcry_pk_sign(&sig_exp, hash_exp, key_exp);

  if (error) {
    fprintf(stderr, "Error %s signing the hash\n", gcry_strerror(error));
    exit(1);
  }

  get_signature(r, sig_exp, "r");
  get_signature(s, sig_exp, "s");

  gcry_mpi_release(h_mpi);
  gcry_mpi_release(p_mpi);
  gcry_mpi_release(q_mpi);
  gcry_mpi_release(g_mpi);
  gcry_mpi_release(y_mpi);
  gcry_mpi_release(x_mpi);

  gcry_sexp_release(sig_exp);
  gcry_sexp_release(key_exp);
  gcry_sexp_release(hash_exp);
}

int
verify_image(FILE * fp, void * p, void * q, void * g, void * y,
	     void * r, void * s)
{
  uint8_t hash[20];
  gcry_mpi_t h_mpi, p_mpi, q_mpi, g_mpi, y_mpi, r_mpi, s_mpi;
  gcry_sexp_t key_exp, hash_exp, sig_exp;
  gcry_error_t error;

  get_sha1(fp, hash);

  get_mpi(&h_mpi, hash, 20);
  get_mpi(&p_mpi, p, 128);
  get_mpi(&q_mpi, q, 20);
  get_mpi(&g_mpi, g, 128);
  get_mpi(&y_mpi, y, 128);
  get_mpi(&r_mpi, r, 20);
  get_mpi(&s_mpi, s, 20);

  error = gcry_sexp_build(&key_exp, NULL,
			  "(public-key "
			  "(dsa (p %m) (q %m) (g %m) (y %m)))",
			  p_mpi, q_mpi, g_mpi, y_mpi);

  if (error) {
    fprintf(stderr, "Error %s building public key\n", gcry_strerror(error));
    exit(1);
  }

  error = gcry_sexp_build(&hash_exp, NULL,
			  "(data (flags raw) (value %m))", h_mpi);

  if (error) {
    fprintf(stderr, "Error %s building hash\n", gcry_strerror(error));
    exit(1);
  }

  error = gcry_sexp_build(&sig_exp, NULL,
			  "(sig-val (dsa (r %m) (s %m)))", r_mpi, s_mpi);

  if (error) {
    fprintf(stderr, "Error %s building signature\n", gcry_strerror(error));
    exit(1);
  }

  error = gcry_pk_verify(sig_exp, hash_exp, key_exp);

  gcry_mpi_release(h_mpi);
  gcry_mpi_release(p_mpi);
  gcry_mpi_release(q_mpi);
  gcry_mpi_release(g_mpi);
  gcry_mpi_release(y_mpi);
  gcry_mpi_release(r_mpi);
  gcry_mpi_release(s_mpi);

  gcry_sexp_release(sig_exp);
  gcry_sexp_release(key_exp);
  gcry_sexp_release(hash_exp);

  return error == 0;
}

#else /* SUPPORT_DSA */

void init_dsa()
{
  fprintf(stderr,
	  "DSA support disabled in compilation!\n"
	  "If you wish to perform real DSA operations you have to enable\n"
	  "SUPPORT_DSA and recompile (requires libgcrypt).\n");

  exit(1);
}

void sign_image(FILE * fp, void * p, void * q, void * g, void * y,
		void * x, void * r, void * s) {}

int verify_image(FILE * fp, void * p, void * q, void * g, void * y,
		 void * r, void * s) { return 0; }

#endif /* SUPPORT_DSA */

void
inc_key(uint32_t * key)
{
  if (++key[0] != 0) {
    return;
  }
  if (++key[1] != 0) {
    return;
  }
  if (++key[2] != 0) {
    return;
  }
  ++key[3];
}

#define Y_INIT 0xf1bbcdc8
#define Y_STEP 0x61c88647

void
tea_decrypt(uint32_t * iptr, uint32_t * optr, size_t len, uint32_t * key)
{
  uint32_t d1, d2;
  uint32_t step, y;
  uint32_t x;
  int i;

  step = Y_STEP;

  for (; len >= 8; len -= 8) {
    d1 = get_le32(iptr++);
    d2 = get_le32(iptr++);

    y = Y_INIT;

    for (i = 0; i < 8; i++) {
      x  = key[2] + (d1 << 4);
      x ^= d1 + y;
      x ^= key[3] + (d1 >> 5);
      d2 -= x;
      
      x  = key[0] + (d2 << 4);
      x ^= d2 + y;
      x ^= key[1] + (d2 >> 5);
      d1 -= x;
      
      y += step;
    }

    put_le32(optr++, d1);
    put_le32(optr++, d2);
    
    inc_key(key);
  }
}

void
tea_encrypt(uint32_t * iptr, uint32_t * optr, size_t len, uint32_t * key)
{
  uint32_t d1, d2;
  uint32_t step, y;
  uint32_t x;
  int i;

  step = Y_STEP;

  for (; len >= 8; len -= 8) {
    d1 = get_le32(iptr++);
    d2 = get_le32(iptr++);

    y = Y_INIT + 7 * step;

    for (i = 0; i < 8; i++) {
      x  = key[0] + (d2 << 4);
      x ^= d2 + y;
      x ^= key[1] + (d2 >> 5);
      d1 += x;

      x  = key[2] + (d1 << 4);
      x ^= d1 + y;
      x ^= key[3] + (d1 >> 5);
      d2 += x;

      y -= step;
    }

    put_le32(optr++, d1);
    put_le32(optr++, d2);
    
    inc_key(key);
  }
}

#define VERIFY_MAGIC 0xaa55aa55

void
key_shift(uint32_t * key, int x)
{
  key[0] <<= 8;
  key[0] |= key[1] >> 24;
  key[1] <<= 8;
  key[1] |= key[2] >> 24;
  key[2] <<= 8;
  key[2] |= key[3] >> 24;
  key[3] <<= 8;
  key[3] |= x & 0xff;
}

void
key_add(uint32_t * okey, uint32_t * ikey, uint32_t x)
{
  uint32_t c;

  okey[0] = ikey[0] + x;
  c = okey[0] < ikey[0] ? 1 : 0;
  okey[1] = ikey[1] + c;
  c = okey[1] < ikey[1] ? 1 : 0;
  okey[2] = ikey[2] + c;
  c = okey[2] < ikey[2] ? 1 : 0;
  okey[3] = ikey[3] + c;
}

uint32_t
bswap(uint32_t x)
{
  x = ((x & 0xff00ff00) >> 8)  | ((x & 0x00ff00ff) << 8);
  x = ((x & 0xffff0000) >> 16) | ((x & 0x0000ffff) << 16);

  return x;
}

void
key_swap(uint32_t * key)
{
  key[0] = bswap(key[0]);
  key[1] = bswap(key[1]);
  key[2] = bswap(key[2]);
  key[3] = bswap(key[3]);
}

int
key_first(FILE * kfp, uint32_t * key)
{
  return 4 == fread(key, sizeof(uint32_t), 4, kfp);
}

int
key_next(FILE * kfp, uint32_t * key, int complete)
{
  int x;

  if (complete) {
    return key_first(kfp, key);
  }

  x = fgetc(kfp);

  if (x == EOF) {
    return 0;
  }

  key_shift(key, x);

  return 1;
}

int
key_scan(FILE * ifp, FILE * kfp, int complete)
{
  uint32_t key[4];
  uint32_t tmp[4];
  uint32_t c_data[2];
  uint32_t p_data[2];
  uint32_t pos;
  int i, count;

  printv(V_ALL, "Scanning potential keys\n");

  fseek(ifp, 8, SEEK_SET);
  fread(&pos, 1, sizeof(pos), ifp);
  pos = get_le32(&pos) - 4;

  fseek(ifp, sizeof(struct mi4hdr) + (pos & ~7), SEEK_SET);
  fread(c_data, 1, sizeof(c_data), ifp);

  i = pos & 0x7 ? 1 : 0;

  count = 0;

  key_first(kfp, key);

  do {
    key_add(tmp, key, (pos - mi4hdr.plainlen) / 8);
    tea_decrypt(c_data, p_data, sizeof(c_data), tmp);

    if (get_le32(&p_data[i]) == VERIFY_MAGIC) {
      printf("Potential key %08x %08x %08x %08x\n",
	     key[0], key[1], key[2], key[3]);
      count++;
    }

    key_swap(key);

    key_add(tmp, key, (pos - mi4hdr.plainlen) / 8);
    tea_decrypt(c_data, p_data, sizeof(c_data), tmp);

    if (get_le32(&p_data[i]) == VERIFY_MAGIC) {
      printf("Potential key %08x %08x %08x %08x\n",
	     key[0], key[1], key[2], key[3]);
      count++;
    }

    key_swap(key);

  } while (key_next(kfp, key, complete));

  if (!count) {
    printv(V_ALL, "No potential keys found\n");
  }

  return 0;
}

void
le_offs_check(uint32_t offs)
{
  int i;

  for (i = 0; i < NUM_LE_OFFS; i++) {
    if (le_offs_table[i].offs == offs) {
      printv(V_ALL, "Label end offset '%s' (0x%x)\n",
	     le_offs_table[i].name, offs);
      return;
    }
  }

  printv(V_ALL, "Warning: unknown label end offset 0x%x !\n", offs);
}

int
decode_ok(FILE * fp, int skip_header)
{
  long offset = skip_header ? sizeof(struct mi4hdr) : 0;
  uint32_t x, leo;

  fseek(fp, offset + 0xe0, SEEK_SET);

  fread(&x, 1, sizeof(x), fp);
  x = get_le32(&x);

  if (x != LABEL_MAGIC) {
    goto fail;
  }

  fread(&x, 1, sizeof(x), fp);
  leo = get_le32(&x);

  fread(&x, 1, sizeof(x), fp);
  x = get_le32(&x);

  fseek(fp, offset + x - 4, SEEK_SET);

  fread(&x, 1, sizeof(x), fp);
  x = get_le32(&x);

  if (x != VERIFY_MAGIC) {
    goto fail;
  }

  le_offs_check(leo);

  return 1;

 fail:

  return 0;
}

uint32_t
sum_bytes(void * ptr, int size)
{
  uint8_t * b = ptr;
  uint32_t sum = 0;
  
  while (size--) {
    sum += *b++;
  }

  return sum;
}

uint32_t
get_hex_key(struct hex_key * hkey, uint32_t index)
{
  index &= (HEX_KEYSIZE - 1);

  return hkey->key[index];
}

uint32_t
hex_code(uint32_t * buf, struct hex_key * hkey, uint32_t x, int encode)
{
  uint32_t tmp;
  int i, k;

  for (i = 0; i < (HEX_BLKSIZE / 2); i++) {
    k = get_hex_key(hkey, x + i) + (HEX_BLKSIZE / 2);
    tmp    = buf[k];
    buf[k] = buf[i];
    buf[i] = tmp;
  }

  return get_hex_key(hkey, sum_bytes(&buf[encode ? k : (i - 1)], 4));
}

int
file_copy(FILE * ifp, FILE * ofp, int total, uint32_t * crc)
{
  int len, todo, done;

  todo = total >= 0 ? total : 0x7fffffff;
  done = 0;

  while (todo > 0) {
    len = fread(inbuf, 1, todo < BUFSIZE ? todo : BUFSIZE, ifp);
    if (!len) {
      break;
    }
    fwrite(inbuf, 1, len, ofp);
    if (crc) {
      *crc = update_crc32(inbuf, len, *crc);
    }
    todo -= len;
    done += len;
  }

  if (todo > 0 && total > 0) {
    fprintf(stderr,
	    "Unexpected EOF while copying, %d bytes not copied!\n", todo);
  }

  return done;
}

int
__file_crypt(int encrypt, FILE * ifp, FILE * ofp,
	     const uint32_t * key, uint32_t * crc)
{
  uint32_t tkey[4];
  int len, done;

  memcpy(tkey, key, sizeof(tkey));

  done = 0;

  while ((len = fread(inbuf, 1, BUFSIZE, ifp)) > 0) {
    if (encrypt) {
      tea_encrypt(inbuf, outbuf, len, tkey);
      if (crc) {
	*crc = update_crc32(outbuf, len, *crc);
      }
    } else {
      if (crc) {
	*crc = update_crc32(inbuf, len, *crc);
      }
      tea_decrypt(inbuf, outbuf, len, tkey);
    }

    fwrite(outbuf, 1, len, ofp);
    done += len;
  }

  return done;
}

int
file_decrypt(FILE * ifp, FILE * ofp, uint32_t * key, uint32_t * crc)
{
  return __file_crypt(0, ifp, ofp, key, crc);
}

int
file_encrypt(FILE * ifp, FILE * ofp, uint32_t * key, uint32_t * crc)
{
  return __file_crypt(1, ifp, ofp, key, crc);
}

int
__file_hexcode(int encode, FILE * ifp, FILE * ofp,
	       struct hex_key * hkey, uint32_t * sum)
{
  int x, len;

  x = hexhdr.sum1 & (HEX_KEYSIZE - 1);

  while ((len = fread(inbuf, 1, HEX_BUFSIZE, ifp)) == HEX_BUFSIZE) {
    x = hex_code(inbuf, hkey, x, encode);
    fwrite(inbuf, 1, len, ofp);
    if (sum) {
      *sum = update_sum32(inbuf, len, *sum);
    }
  }

  if (len) {
    fwrite(inbuf, 1, len, ofp);
    if (sum) {
      *sum = update_sum32(inbuf, len, *sum);
    }
  }

  return 0;
}

int
file_hexdecode(FILE * ifp, FILE * ofp, struct hex_key * hkey, uint32_t * sum)
{
  return __file_hexcode(0, ifp, ofp, hkey, sum);
}

int
file_hexencode(FILE * ifp, FILE * ofp, struct hex_key * hkey, uint32_t * sum)
{
  return __file_hexcode(1, ifp, ofp, hkey, sum);
}

int
memmatch(void * p1, int s1, void * p2, int s2)
{
  char * ptr = p1;
  int i;

  for (i = 0; i < (s1 - s2); i++) {
    if (!memcmp(ptr + i, p2, s2)) {
      return i;
    }
  }

  return -1;
}

int
find_key(long pos, long * o_pos, void * buf, int len, void * key, int klen,
	 const char * str)
{
  char * ptr = buf;
  int offs = 0;

  while (offs >= 0) {
    offs = memmatch(ptr, len, key, klen);

    if (offs >= 0) {
      if (*o_pos != 0 && (*o_pos != (pos + offs))) {
	fprintf(stderr, "Duplicate match for %s at %08lx and %08lx\n",
		str, *o_pos, pos + offs);
	return -1;
      }

      *o_pos = pos + offs;
    }

    ptr += offs + klen;
    len -= offs + klen;
  }

  return 0;
}

int
bl_patch(FILE * ifp, FILE * ofp, struct dsa_key * dkey)
{
  long pos, p_pos, q_pos, g_pos, y_pos;
  struct dsa_key * key;
  size_t len;
  int i;

  p_pos = q_pos = g_pos = y_pos = 0;

  printv(V_ALL, "Scanning the bootloader for existing DSA keys\n");

  for (i = 0, key = &dsa_keytable[0]; i < NUM_DSA_KEYS; i++, key++) {
    printv(V_LOW, "Searching for '%s' DSA keys ... ", key->name);

    pos = 0;

    do {
      fseek(ifp, pos, SEEK_SET);

      len = fread(inbuf, 1, BUFSIZE, ifp);

      if (find_key(pos, &p_pos, inbuf, len, key->p, sizeof(key->p), "p")) {
	break;
      }
      if (find_key(pos, &q_pos, inbuf, len, key->q, sizeof(key->q), "q")) {
	break;
      }
      if (find_key(pos, &g_pos, inbuf, len, key->g, sizeof(key->g), "g")) {
	break;
      }
      if (find_key(pos, &y_pos, inbuf, len, key->y, sizeof(key->y), "y")) {
	break;
      }

      pos += BUFSIZE - 128;

    } while (len == BUFSIZE);

    if (p_pos && q_pos && g_pos && y_pos) {
      printv(V_LOW, "found!\n");
      printv(V_ALL, "Found DSA key '%s', ", key->name);
      printv(V_ALL, "p at %lx, q at %lx, g at %lx, y at %lx\n",
	     p_pos, q_pos, g_pos, y_pos);
      printv(V_ALL,
	     "*******************************************************\n"
	     "**    Do NOT try to use the patched bootloader if    **\n"
	     "**    you think the values above are not correct!    **\n"
	     "**                                                   **\n"
	     "**    I TAKE ABSOLUTELY NO RESPONSIBILITY IF YOU     **\n"
	     "**    BREAK YOUR PLAYER BEYOND REPAIR WITH THIS      **\n"
	     "**    TOOL! IF YOU FEEL UNSURE, JUST DON'T DO IT!    **\n"
	     "*******************************************************\n");

      fseek(ifp, 0, SEEK_SET);

      file_copy(ifp, ofp, -1, NULL);

      fseek(ofp, p_pos, SEEK_SET);
      fwrite(dkey->p, 1, sizeof(dkey->p), ofp);

      fseek(ofp, q_pos, SEEK_SET);
      fwrite(dkey->q, 1, sizeof(dkey->q), ofp);

      fseek(ofp, g_pos, SEEK_SET);
      fwrite(dkey->g, 1, sizeof(dkey->g), ofp);

      fseek(ofp, y_pos, SEEK_SET);
      fwrite(dkey->y, 1, sizeof(dkey->y), ofp);

      printv(V_ALL, "Bootloader patched with key '%s'!\n", dkey->name);

      return 0;
    } else {
      printv(V_LOW, "not found\n");
    }
  }

  fprintf(stderr,
	  "No suitable DSA keys found! Unable to patch the bootloader!\n");

  return 1;
}

int
myopt(int argc, char * argv[], const char * optstr, const char ** optarg)
{
  static char * opt;
  static int optind;
  const char * c;

  if (optind >= argc) {
    return -optind;
  }

  if (!opt) {
    opt = argv[optind];

    if (!strcmp(opt, "--")) {
      optind++;
      return -optind;
    }

    if (opt[0] != '-') {
      return -optind;
    }

    opt++;

    if (*opt == 0) {
      return -optind;
    }
  }

  c = strchr(optstr, *opt);

  if (!c) {
    fprintf(stderr, "Unknown option '%c'!\n", *opt);
    opt++;
    if (*opt == 0) {
      optind++;
      opt = NULL;
    }
    return '?';
  }

  if (c[1] == ':') {
    optind++;
    if (c[2] == ':') {
      *optarg = opt[1] ? opt + 1 : NULL;
      opt = NULL;
    } else {
      if (opt[1] != 0) {
	*optarg = opt + 1;
      } else if (optind < argc) {
	*optarg = argv[optind++];
      } else {
	fprintf(stderr, "Option '%c' requires an argument!\n", *c);
	exit(1);
      }
      opt = NULL;
    }
  } else {
    opt++;
    if (*opt == 0) {
      optind++;
      opt = NULL;
    }
  }

  return *c;
}

int
parse_tea_key(int argc, char * argv[], uint32_t * key)
{
  char * p;
  int i;

  if (argc == 1) {
    for (i = 0; i < NUM_TEA_KEYS; i++) {
      if (!strcmp(tea_keytable[i].name, argv[0])) {
	memcpy(key, tea_keytable[i].key, 4 * sizeof(uint32_t));
	return 1;
      }
    }
    fprintf(stderr, "Unknown key '%s'!\n", argv[0]);
    return 0;
  } else if (argc == 4) {
    for (i = 0; i < 4; i++) {
      key[i] = strtoul(argv[i], &p, 16);
      if (*p != 0) {
	fprintf(stderr, "Invalid key '%s' (must be a hexadecimal number)!\n",
		argv[i]);
	return 0;
      }
    }
    return 1;
  }

  fprintf(stderr, "Invalid number of arguments!\n");

  return 0;
}

void
print_tea_keys()
{
  int i;

  printf("\nKnown keys: %s", tea_keytable[0].name);

  for (i = 1; i < NUM_TEA_KEYS; i++) {
    printf(", %s", tea_keytable[i].name);
  }

  printf("\n\n");
}

void
help_decrypt()
{
  printf("\n"
	 "Usage:\tmi4code decrypt [options] <infile> <outfile> [keyid]\n"
	 "\n"
	 "\tmi4code decrypt [options] <infile> <outfile> [k1] [k2] [k3] [k4]\n"
	 "\n"
	 "options:\n"
	 "\t-l\tlist known keys\n"
	 "\t-s\tstrip mi4 header\n"
	 "\t-v\tincrease verbosity\n"
	 "\t-q\tdecrease verbosity\n"
	 "\t-h\tthis help\n"
	 "\n");

  exit(1);
}

int
cmd_decrypt(int argc, char * argv[])
{
  FILE * ifp, * ofp;
  struct tea_key * kp;
  uint32_t key[4];
  long ipos, opos;
  int strip_header = 0;
  int i;
  int c;
  
  while ((c = myopt(argc, argv, "lsvqh", NULL)) > 0) {
    switch (c) {
    case 'l':
      print_tea_keys();
      return 0;
    case 's':
      strip_header = 1;
      break;
    case 'v':
      verbose++;
      break;
    case 'q':
      verbose--;
      break;
    case 'h':
      help_decrypt();
      break;
    }
  }

  argc += c;
  argv -= c;

  if (argc < 2) {
    fprintf(stderr, "Invalid number of arguments!\n");
    help_decrypt();
  }

  ifp = fopen(argv[0], "rb");

  if (!ifp) {
    perror(argv[0]);
    return 1;
  }

  ofp = fopen(argv[1], "wb+");

  if (!ofp) {
    perror(argv[1]);
    return 1;
  }

  argc -= 2;
  argv += 2;

  if (!mi4hdr_read(ifp)) {
    return 1;
  }
  
  if (!strip_header) {
    printv(V_LOW, "Copying the mi4 header\n");
    mi4hdr_write(ofp);
  } else {
    printv(V_LOW, "Skipping the mi4 header\n");
  }

  if (mi4hdr.plainlen) {
    printv(V_LOW, "Copying the plaintext part (%d bytes)\n", mi4hdr.plainlen);
    file_copy(ifp, ofp, mi4hdr.plainlen, NULL);
  } else {
    printv(V_LOW, "File has no plaintext part\n");
  }

  if (argc > 0) {
    if (!parse_tea_key(argc, argv, key)) {
      return 1;
    }
    printv(V_ALL, "Decrypting with key %08x %08x %08x %08x\n",
	   key[0], key[1], key[2], key[3]);
    file_decrypt(ifp, ofp, key, NULL);
    if (decode_ok(ofp, !strip_header)) {
      printv(V_ALL, "Verify ok!\n");
      return 0;
    } else {
      printv(V_ALL, "Verification failed (maybe wrong key?)!\n");
      return 1;
    }
  }

  ipos = ftell(ifp);
  opos = ftell(ofp);

  for (i = 0; i < NUM_TEA_KEYS; i++) {
    fseek(ifp, ipos, SEEK_SET);
    fseek(ofp, opos, SEEK_SET);

    kp = &tea_keytable[i];

    printv(V_LOW, "Trying key '%s'\t(%08x %08x %08x %08x) ... ",
	   kp->name, kp->key[0], kp->key[1], kp->key[2], kp->key[3]);

    file_decrypt(ifp, ofp, kp->key, NULL);

    if (decode_ok(ofp, !strip_header)) {
      printv(V_LOW, "ok!\n");
      printv(V_ALL, "Decrypted ok with key '%s' (%08x %08x %08x %08x)\n",
	     kp->name, kp->key[0], kp->key[1], kp->key[2], kp->key[3]);
      return 0;
    } else {
      printv(V_LOW, "fail\n");
    }
  }

  printv(V_ALL, "Unable to decrypt the file (maybe the key is not known)!\n");

  return 1;
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
parse_plen(const char * s, uint32_t * v)
{
  if (!strcmp(s, "all")) {
    *v = ~7;
    return 1;
  }

  if (!parse_uint32(s, v)) {
    return 0;
  }

  if (*v & 7) {
    fprintf(stderr, "Lenght must be a multiple of 8!\n");
    return 0;
  }

  return 1;
}

void
help_encrypt()
{
  printf("\n"
	 "Usage:\tmi4code encrypt [options] <infile> <outfile> <keyid>\n"
	 "\n"
	 "\tmi4code encrypt [options] <infile> <outfile> <k1> <k2> <k3> <k4>\n"
	 "\n"
	 "options:\n"
	 "\t-l\t\tlist known keys\n"
	 "\t-n\t\tdon't correct the checksum\n"
	 "\t-p <num|all>\tplaintext length\n"
	 "\t-v\t\tincrease verbosity\n"
	 "\t-q\t\tdecrease verbosity\n"
	 "\t-h\t\tthis help\n"
	 "\n");

  exit(1);
}

int
cmd_encrypt(int argc, char * argv[])
{
  FILE * ifp, * ofp;
  const char * opt;
  uint32_t key[4];
  uint32_t crc = 0;
  uint32_t plen;
  int fix_crc = 1;
  int set_plen = 0;
  int c;
  
  while ((c = myopt(argc, argv, "lnp:vqh", &opt)) > 0) {
    switch (c) {
    case 'l':
      print_tea_keys();
      return 0;
    case 'n':
      fix_crc = 0;
      break;
    case 'p':
      if (!parse_plen(opt, &plen)) {
	return 1;
      }
      set_plen = 1;
      break;
    case 'v':
      verbose++;
      break;
    case 'q':
      verbose--;
      break;
    case 'h':
      help_encrypt();
      break;
    }
  }

  argc += c;
  argv -= c;

  if (argc < 2) {
    fprintf(stderr, "Invalid number of arguments!\n");
    help_encrypt();
  }


  ifp = fopen(argv[0], "rb");
  
  if (!ifp) {
    perror(argv[0]);
    return 1;
  }

  ofp = fopen(argv[1], "wb");
  
  if (!ofp) {
    perror(argv[1]);
    return 1;
  }

  argc -= 2;
  argv += 2;

  if (!parse_tea_key(argc, argv, key)) {
    return 1;
  }

  if (!mi4hdr_read(ifp)) {
    return 1;
  }

  if (set_plen) {
    mi4hdr.plainlen = MIN(plen, mi4hdr.totallen - sizeof(struct mi4hdr));
  }

  mi4hdr_write(ofp);

  if (mi4hdr.plainlen) {
    printv(V_LOW, "Copying the plaintext part (%d bytes)\n", mi4hdr.plainlen);
    file_copy(ifp, ofp, mi4hdr.plainlen, &crc);
  } else {
    printv(V_LOW, "File has no plaintext part\n");
  }

  printv(V_ALL, "Encrypting with key %08x %08x %08x %08x\n",
	 key[0], key[1], key[2], key[3]);

  file_encrypt(ifp, ofp, key, &crc);

  if (crc != mi4hdr.crc32) {
    if (fix_crc) {
      printv(V_ALL, "CRC32 corrected (%08x -> %08x)\n", mi4hdr.crc32, crc);
      mi4hdr.crc32 = crc;
      mi4hdr_write(ofp);
    } else {
      printv(V_ALL, "CRC32 mismatch detected (%08x <-> %08x)\n",
	     mi4hdr.crc32, crc);
    }
  }

  return 0;
}

int
parse_leo(const char * s, uint32_t * v)
{
  int i;

  for (i = 0; i < NUM_LE_OFFS; i++) {
    if (!strcmp(le_offs_table[i].name, s)) {
      *v = le_offs_table[i].offs;
      printv(V_LOW, "Using label end offset '%s' (0x%x)\n", s, *v);
      return 1;
    }
  }

  if (!parse_uint32(s, v)) {
    return 0;
  }

  printv(V_LOW, "Using label end offset 0x%x\n", *v);

  return 1;
}

void
print_leos()
{
  int i;

  printf("\nKnown offs_ids: %s (0x%x)",
	 le_offs_table[0].name, le_offs_table[0].offs);

  for (i = 1; i < NUM_LE_OFFS; i++) {
    printf(", %s (0x%x)", le_offs_table[i].name, le_offs_table[i].offs);
  }

  printf("\n\n");
}

void
help_build()
{
  printf("\n"
	 "Usage:\tmi4code build [options] <infile> <outfile> [offs]\n"
	 "\n"
	 "\tmi4code build [options] <infile> <outfile> [offs_id]\n"
	 "\n"
	 "options:\n"
	 "\t-l\t\tlist known offs_ids\n"
	 "\t-2\t\tmake 010201 header\n"
	 "\t-3\t\tmake 010301 header (default)\n"
	 "\t-p <num|all>\tplaintext length (default 0x200)\n"
	 "\t-v\t\tincrease verbosity\n"
	 "\t-q\t\tdecrease verbosity\n"
	 "\t-h\t\tthis help\n"
	 "\n");

  exit(1);  
}

int
roundup(int val, int to)
{
  val += to - 1;
  val &= ~(to - 1);

  return val;
}

int
cmd_build(int argc, char * argv[])
{
  FILE * ifp, * ofp;
  const char * opt;
  uint32_t ver = 0x10301;
  uint32_t plen = 0x200;
  uint32_t x, leo;
  int todo;
  int c;
  
  while ((c = myopt(argc, argv, "l23p:vqh", &opt)) > 0) {
    switch (c) {
    case 'l':
      print_leos();
      return 0;
    case '2':
      ver = 0x10201;
      break;
    case '3':
      ver = 0x10301;
      break;
    case 'p':
      if (!parse_plen(opt, &plen)) {
	return 1;
      }
      break;
    case 'v':
      verbose++;
      break;
    case 'q':
      verbose--;
      break;
    case 'h':
      help_build();
      break;
    }
  }

  argc += c;
  argv -= c;

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Invalid number of arguments!\n");
    help_build();
  }

  ifp = fopen(argv[0], "rb");
  
  if (!ifp) {
    perror(argv[0]);
    return 1;
  }

  ofp = fopen(argv[1], "wb");
  
  if (!ofp) {
    perror(argv[1]);
    return 1;
  }

  argc -= 2;
  argv += 2;

  if (argc > 0) {
    if (!parse_leo(argv[0], &leo)) {
      return 1;
    }

    argc -= 1;
    argv += 1;
  } else {
    parse_leo("default", &leo);
  }

  printv(V_ALL,
	 "*************************************************************\n"
	 "**        The data in input file offsets 0xe0-0xeb         **\n"
	 "**        will be overwritten. You should not have         **\n"
	 "**                your own code/data there!                **\n"
	 "*************************************************************\n");

  memset(&mi4hdr, 0, sizeof(struct mi4hdr));

  mi4hdr_write(ofp);

  file_copy(ifp, ofp, -1, NULL);

  if (ftell(ofp) < (leo + sizeof(struct mi4hdr))) {
    x = MAX(leo, 0xec);
    fprintf(stderr,
	    "File too small, at least %u (0x%x) bytes needed!\n", x, x);
    return 1;
  }

  put_le32(&x, VERIFY_MAGIC);
  fwrite(&x, 1, sizeof(x), ofp);

  todo = roundup(ftell(ofp), 1024) - ftell(ofp);

  mi4hdr.magic    = MI4MAGIC;
  mi4hdr.version  = ver;
  mi4hdr.datalen  = ftell(ofp) - sizeof(struct mi4hdr);
  mi4hdr.crypto   = 2;
  mi4hdr.totallen = roundup(ftell(ofp), 1024);
  mi4hdr.plainlen = MIN(plen, mi4hdr.totallen - sizeof(struct mi4hdr));

  while (todo--) {
    fputc(0, ofp);
  }

  mi4hdr_write(ofp);

  fseek(ofp, sizeof(struct mi4hdr) + 0xe0, SEEK_SET);

  put_le32(&x, LABEL_MAGIC);
  fwrite(&x, 1, sizeof(x), ofp);

  put_le32(&x, leo);
  fwrite(&x, 1, sizeof(x), ofp);

  put_le32(&x, mi4hdr.datalen);
  fwrite(&x, 1, sizeof(x), ofp);

  printv(V_ALL, "Image created ok!\n");

  return 0;
}

void
print_dsa_keys()
{
  int i;

  printf("\nKnown DSA keys: dummy*");

  for (i = 0; i < NUM_DSA_KEYS; i++) {
    printf(", %s%s", dsa_keytable[i].name, dsa_keytable[i].known ? "*" : "");
  }

  printf("\n"
	 "\n"
	 "'dummy' is not an actual key but a special signature\n"
	 "which tries to exploit a quirk in the verification code.\n"
	 "\n"
	 "A star after the name means the private key is known and\n"
	 "can be used for signing.\n"
	 "\n");
}

struct dsa_key *
parse_dsa_key(int argc, char * argv[])
{
  int i;

  if (argc < 1) {
    return NULL;
  }

  for (i = 0; i < NUM_DSA_KEYS; i++) {
    if (!strcmp(argv[0], dsa_keytable[i].name)) {
      return &dsa_keytable[i];
    }
  }

  fprintf(stderr, "Unknown key '%s'!\n", argv[0]);

  return NULL;
}

void
help_sign()
{
  printf("\n"
	 "Usage:\tmi4code sign [options] <infile> <outfile> [keyid]\n"
	 "\n"
	 "\tSign the file with DSA private key. If key not\n"
	 "\tgiven the dummy signature will be used.\n"
	 "\n"
	 "options:\n"
	 "\t-l\tlist available keys\n"
	 "\t-v\tincrease verbosity\n"
	 "\t-q\tdecrease verbosity\n"
	 "\t-h\tthis help\n"
	 "\n");

  exit(1);
}

const char dummy_r[20] =
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
const char dummy_s[20] =
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int
cmd_sign(int argc, char * argv[])
{
  FILE * ifp, * ofp;
  struct dsa_key * dkey;
  int c;
  
  while ((c = myopt(argc, argv, "lvqh", NULL)) > 0) {
    switch (c) {
    case 'l':
      print_dsa_keys();
      return 0;
    case 'v':
      verbose++;
      break;
    case 'q':
      verbose--;
      break;
    case 'h':
      help_sign();
      break;
    }
  }

  argc += c;
  argv -= c;

  if (argc < 2) {
    fprintf(stderr, "Invalid number of arguments!\n");
    help_sign();
  }

  ifp = fopen(argv[0], "rb");
  
  if (!ifp) {
    perror(argv[0]);
    return 1;
  }

  ofp = fopen(argv[1], "wb");
  
  if (!ofp) {
    perror(argv[1]);
    return 1;
  }

  argc -= 2;
  argv += 2;

  if (!mi4hdr_read(ifp)) {
    return 1;
  }

  if (!mi4hdr_dsa_check()) {
    return 1;
  }

  if (argc < 1 || !strcmp(argv[0], "dummy")) {
    memcpy(mi4hdr.dsa_r, dummy_r, sizeof(mi4hdr.dsa_r));
    memcpy(mi4hdr.dsa_s, dummy_s, sizeof(mi4hdr.dsa_s));

    printv(V_ALL,
	   "*************************************************************\n"
	   "**         Image signed with 'dummy' DSA signature.        **\n"
	   "**   This works with certain versions of the bootloader    **\n"
	   "**     and completely bypasses the DSA check! It works     **\n"
	   "**      only because the DSA verification code in the      **\n"
	   "**     bootloader has a certain quirk and accepts this     **\n"
	   "**    signature regardless of the contents of the file.    **\n"
	   "**    This trick may not work on all (future) versions     **\n"
	   "**                of the bootloader.                       **\n"
	   "*************************************************************\n");
  } else {
    init_dsa();

    dkey = parse_dsa_key(argc, argv);

    if (!dkey) {
      return 1;
    }

    if (!dkey->known) {
      fprintf(stderr,
	      "The private key for '%s' is not known! Cannot sign with it!\n",
	      dkey->name);
    }

    sign_image(ifp, dkey->p, dkey->q, dkey->g, dkey->y,
	       dkey->x, mi4hdr.dsa_r, mi4hdr.dsa_s);

    printv(V_ALL,
	   "*************************************************************\n"
	   "**              Image signed with DSA key.                 **\n"
	   "**          A bootloader must have the same key            **\n"
	   "**              to boot the signed image!                  **\n"
	   "*************************************************************\n");
  }

  mi4hdr_write(ofp);

  file_copy(ifp, ofp, -1, NULL);

  return 0;
}

void
help_verify()
{
  printf("\n"
	 "Usage:\tmi4code verify [options] <file> [keyid]\n"
	 "\n"
	 "options:\n"
	 "\t-l\tlist available keys\n"
	 "\t-v\tincrease verbosity\n"
	 "\t-q\tdecrease verbosity\n"
	 "\t-h\tthis help\n"
	 "\n");

  exit(1);
}

int
cmd_verify(int argc, char * argv[])
{
  struct dsa_key * dkey;
  FILE * ifp, * bfp;
  int i;
  int c;
  
  bfp = NULL; /* !!! */

  while ((c = myopt(argc, argv, "lvqh", NULL)) > 0) {
    switch (c) {
    case 'l':
      print_dsa_keys();
      return 0;
    case 'v':
      verbose++;
      break;
    case 'q':
      verbose--;
      break;
    case 'h':
      help_verify();
      break;
    }
  }

  argc += c;
  argv -= c;

  if (argc < 1) {
    fprintf(stderr, "Invalid number of arguments!\n");
    help_verify();
  }

  ifp = fopen(argv[0], "rb");

  if (!ifp) {
    perror(argv[0]);
    return 1;
  }

  if (!mi4hdr_read(ifp)) {
    return 1;
  }

  if (!mi4hdr_dsa_check()) {
    return 1;
  }

  if (!memcmp(mi4hdr.dsa_r, dummy_r, sizeof(mi4hdr.dsa_r)) &&
      !memcmp(mi4hdr.dsa_s, dummy_s, sizeof(mi4hdr.dsa_s))) {
    fprintf(stderr,
	    "The mi4 is signed with 'dummy' signature. "
	    "It is not possible to verify it.\n");
    return 0;
  }

  init_dsa();

  for (i = 0; i < NUM_DSA_KEYS; i++) {
    dkey = &dsa_keytable[i];

    if (verify_image(ifp, dkey->p, dkey->q, dkey->g, dkey->y,
		     mi4hdr.dsa_r, mi4hdr.dsa_s)) {
      printv(V_ALL, "Image verified ok with DSA key '%s'\n", dkey->name);
      return 0;
    }
  }

  printv(V_ALL, "No suitable DSA key found (or signature is incorrect)!\n");

  return 1;
}

void
help_keyscan()
{
  printf("\n"
	 "Usage:\tmi4code keyscan [options] <infile> <keyfile>\n"
	 "\n"
	 "\tScan for potential keys for <infile> in <keyfile>\n"
	 "\n"
	 "options:\n"
	 "\t-v\tincrease verbosity\n"
	 "\t-q\tdecrease verbosity\n"
	 "\t-h\tthis help\n"
	 "\n");

  exit(1);
}

int
cmd_keyscan(int argc, char * argv[])
{
  FILE * ifp, * kfp;
  int complete = 0;
  int c;
  
  while ((c = myopt(argc, argv, "vqh", NULL)) > 0) {
    switch (c) {
    case 'v':
      verbose++;
      break;
    case 'q':
      verbose--;
      break;
    case 'h':
      help_keyscan();
      break;
    }
  }

  argc += c;
  argv -= c;

  if (argc != 2) {
    fprintf(stderr, "Invalid number of arguments!\n");
    help_keyscan();
  }

  ifp = fopen(argv[0], "rb");
  
  if (!ifp) {
    perror(argv[0]);
    return 1;
  }

  if (!strcmp(argv[1], "-")) {
    kfp = stdin;
    complete = 1;
  } else {
    kfp = fopen(argv[1], "rb");
  
    if (!kfp) {
      perror(argv[1]);
      return 1;
    }
  }

  argc -= 2;
  argv += 2;

  if (!mi4hdr_read(ifp)) {
    return 1;
  }

  key_scan(ifp, kfp, complete);

  return 0;
}

void
help_blpatch()
{
  printf("\n"
	 "Usage:\tmi4code blpatch [options] <infile> <outfile> [keyid]\n"
	 "\n"
	 "\tPatch the bootloader file with DSA public key. If key not\n"
	 "\tgiven the mi4code key will be used.\n"
	 "\n"
	 "options:\n"
	 "\t-l\tlist available keys\n"
	 "\t-v\tincrease verbosity\n"
	 "\t-q\tdecrease verbosity\n"
	 "\t-h\tthis help\n"
	 "\n");

  exit(1);
}

int
cmd_blpatch(int argc, char * argv[])
{
  FILE * ifp, * ofp;
  struct dsa_key * dkey;
  int c;
  
  while ((c = myopt(argc, argv, "lvqh", NULL)) > 0) {
    switch (c) {
    case 'l':
      print_dsa_keys();
      return 0;
    case 'v':
      verbose++;
      break;
    case 'q':
      verbose--;
      break;
    case 'h':
      help_blpatch();
      break;
    }
  }

  argc += c;
  argv -= c;

  if (argc < 2) {
    fprintf(stderr, "Invalid number of arguments!\n");
    return 1;
  }

  ifp = fopen(argv[0], "rb");
  
  if (!ifp) {
    perror(argv[0]);
    return 1;
  }

  ofp = fopen(argv[1], "wb");
  
  if (!ofp) {
    perror(argv[1]);
    return 1;
  }

  argc -= 2;
  argv += 2;

  if (argc < 1) {
    dkey = &dsa_keytable[0];
  } else {
    dkey = parse_dsa_key(argc, argv);
  }

  if (!dkey) {
    return 1;
  }

  return bl_patch(ifp, ofp, dkey);
}

void
print_hex_keys()
{
  int i;

  printf("\nKnown HEX keys: %s", hex_keytable[0].name);

  for (i = 1; i < NUM_HEX_KEYS; i++) {
    printf(", %s", hex_keytable[i].name);
  }

  printf("\n\n");
}

struct hex_key *
parse_hex_key(int argc, char * argv[])
{
  int i;

  if (argc < 1) {
    fprintf(stderr, "Invalid number of arguments!\n");
    return NULL;
  }

  for (i = 0; i < NUM_HEX_KEYS; i++) {
    if (!strcmp(argv[0], hex_keytable[i].name)) {
      return &hex_keytable[i];
    }
  }

  fprintf(stderr, "Unknown key '%s'!\n", argv[0]);

  return NULL;
}

void
help_hexdec()
{
  printf("\n"
	 "Usage:\tmi4code hexdec [options] <infile> <outfile> <keyid>\n"
	 "\n"
	 "options:\n"
	 "\t-l\tlist available keys\n"
	 "\t-s\tstrip hex header\n"
	 "\t-v\tincrease verbosity\n"
	 "\t-q\tdecrease verbosity\n"
	 "\t-h\tthis help\n"
	 "\n");

  exit(1);
}

int
cmd_hexdec(int argc, char * argv[])
{
  struct hex_key * hkey;
  FILE * ifp, * ofp;
  int strip_header = 0;
  int c;

  while ((c = myopt(argc, argv, "lsvqh", NULL)) > 0) {
    switch (c) {
    case 'l':
      print_hex_keys();
      return 0;
    case 's':
      strip_header = 1;
      break;
    case 'v':
      verbose++;
      break;
    case 'q':
      verbose--;
      break;
    case 'h':
      help_hexdec();
      break;
    }
  }

  argc += c;
  argv -= c;

  if (argc < 3) {
    fprintf(stderr, "Invalid number of arguments!\n");
    help_hexdec();
  }

  ifp = fopen(argv[0], "rb");
  
  if (!ifp) {
    perror(argv[0]);
    return 1;
  }

  ofp = fopen(argv[1], "wb");
  
  if (!ofp) {
    perror(argv[1]);
    return 1;
  }

  argc -= 2;
  argv += 2;

  hkey = parse_hex_key(argc, argv);

  if (!hkey) {
    return 1;
  }

  if (!hexhdr_read(ifp)) {
    return 1;
  }

  if (!strip_header) {
    printv(V_LOW, "Copying the hex header\n");
    hexhdr_write(ofp);
  } else {
    printv(V_LOW, "Skipping the hex header\n");
  }

  printv(V_ALL, "Decoding with key '%s'\n", hkey->name);

  file_hexdecode(ifp, ofp, hkey, NULL);

  printv(V_ALL,
	 "*************************************************************\n"
	 "**      This tool currently has no way of knowing if       **\n"
	 "**      the correct key was used. It's up to the user      **\n"
	 "**            to verify the results somehow.               **\n"
	 "*************************************************************\n");

  return 0;
}

void
help_hexenc()
{
  printf("\n"
	 "Usage:\tmi4code hexenc [options] <infile> <outfile> <keyid>\n"
	 "\n"
	 "options:\n"
	 "\t-l\tlist available keys\n"
         "\t-n\tdon't correct the checksum\n"
	 "\t-v\tincrease verbosity\n"
	 "\t-q\tdecrease verbosity\n"
	 "\t-h\tthis help\n"
	 "\n");

  exit(1);
}

int
cmd_hexenc(int argc, char * argv[])
{
  struct hex_key * hkey;
  FILE * ifp, * ofp;
  uint32_t sum;
  int fix_csum = 1;
  int c;

  while ((c = myopt(argc, argv, "lnvqh", NULL)) > 0) {
    switch (c) {
    case 'l':
      print_hex_keys();
      return 0;
    case 'n':
      fix_csum = 0;
      break;
    case 'v':
      verbose++;
      break;
    case 'q':
      verbose--;
      break;
    case 'h':
      help_hexenc();
      break;
    }
  }

  argc += c;
  argv -= c;

  if (argc < 3) {
    fprintf(stderr, "Invalid number of arguments!\n");
    help_hexenc();
  }

  ifp = fopen(argv[0], "rb");
  
  if (!ifp) {
    perror(argv[0]);
    return 1;
  }

  ofp = fopen(argv[1], "wb");
  
  if (!ofp) {
    perror(argv[1]);
    return 1;
  }

  argc -= 2;
  argv += 2;

  hkey = parse_hex_key(argc, argv);

  if (!hkey) {
    return 1;
  }

  if (!hexhdr_read(ifp)) {
    return 1;
  }

  hexhdr_write(ofp);

  sum = hexhdr.sum1;

  printv(V_ALL, "Encoding with key '%s'\n", hkey->name);

  file_hexencode(ifp, ofp, hkey, &sum);

  if (sum != hexhdr.sum2) {
    if (fix_csum) {
      printv(V_ALL, "Checksum corrected (%08x -> %08x)\n", hexhdr.sum2, sum);
      hexhdr.sum2 = sum;
      hexhdr_write(ofp);
    } else {
      printv(V_ALL, "Checksum mismatch detected (%08x <-> %08x)\n",
	     hexhdr.sum2, sum);
    }
  }

  printv(V_ALL,
	 "*************************************************************\n"
	 "**      This tool currently has no way of knowing if       **\n"
	 "**      the correct key was used or if the bootloader      **\n"
	 "**    was broken in some other way. It is even possible    **\n"
	 "**    that this tool does not encode the file in a right   **\n"
	 "**     way (it has not been tested on actual hardware).    **\n"
	 "**    Trying to flash a broken bootloader may break your   **\n"
	 "**     player beyond repair. IF YOU FEEL UNSURE, ABORT     **\n"
	 "**            WHATEVER YOU ARE TRYING TO DO NOW!           **\n"
	 "*************************************************************\n");

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

#ifdef SUPPORT_DSA
#define VERSION_EXT "-DSA"
#else
#define VERSION_EXT ""
#endif

int
main(int argc, char * argv[])
{
  cmd_func func;

  printf("mi4code " VERSION VERSION_EXT " (c) by MrH 2006, 2007\n");

  if (argc < 2) {
    help();
  }

  func = get_cmd(argv[1]);

  if (!func) {
    fprintf(stderr, "Unknown command '%s'!\n", argv[1]);
    help();
  }

  return func(argc - 2, argv + 2);
}
