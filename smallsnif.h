/*
 * Small tool for inspect USB trafic from usbmon
 .
 *
 * Copyright (C) 2013 Marco Bueno <bueno.marco@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <sys/ioctl.h>
#include <stdint.h>

#define SETUP_LEN 8
 
typedef uint16_t u16;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;


const char *PIPE[] = {"ISO", "INTR", "CTRL","BULK"};

// usbmon_packet ( as described in kernel/Documentation/usb/usbmon.txt)
struct usbmon_packet {
    u64 id;         /*  0: URB ID - from submission to callback */
    unsigned char type; /*  8: Same as text; extensible. */
    unsigned char xfer_type; /*    ISO (0), Intr, Control, Bulk (3) */
    unsigned char epnum;    /*     Endpoint number and transfer direction */
    unsigned char devnum;   /*     Device address */
    u16 busnum;     /* 12: Bus number */
    char flag_setup;    /* 14: Same as text */
    char flag_data;     /* 15: Same as text; Binary zero is OK. */
    s64 ts_sec;     /* 16: gettimeofday */
    s32 ts_usec;        /* 24: gettimeofday */
    int status;     /* 28: */
    unsigned int length;    /* 32: Length of data (submitted or actual) */
    unsigned int len_cap;   /* 36: Delivered length */
    union {         /* 40: */
        unsigned char setup[SETUP_LEN]; /* Only for Control S-type */
        struct iso_rec {        /* Only for ISO */
            int error_count;
            int numdesc;
        } iso;
    } s;
    int interval;       /* 48: Only for Interrupt and ISO */
    int start_frame;    /* 52: For ISO */
    unsigned int xfer_flags; /* 56: copy of URB's transfer_flags */
    unsigned int ndesc; /* 60: Actual number of ISO descriptors */
};              /* 64 total length */

struct mon_mfetch_arg {
    uint32_t *offvec;   /* Vector of events fetched */
    uint32_t nfetch;    /* Number of events to fetch (out: fetched) */
    uint32_t nflush;    /* Number of events to flush */
};

struct mon_bin_get {
        struct usbmon_packet  *hdr; /* Can be 48 bytes or 64. */
        void  *data;
        size_t alloc;           /* Length of data (can be zero) */
};


#define MON_IOC_MAGIC 0x92

#define MON_IOCX_MFETCH  _IOWR(MON_IOC_MAGIC, 7, struct mon_mfetch_arg)
#define MON_IOCX_GET   _IOW(MON_IOC_MAGIC, 6, struct mon_bin_get)
#define MON_IOCQ_URB_LEN _IO(MON_IOC_MAGIC, 1)