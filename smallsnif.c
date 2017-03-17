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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h> 
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "smallsnif.h"

#define BUFF_LEN 1024 // 1k default buffer

void pTm( struct usbmon_packet *st ) 
{
    struct tm *tm;
    
    tm = localtime((const time_t *)&st->ts_sec);
    printf("%02d:%02d:%02d.%03ld ",
           tm->tm_hour, 
           tm->tm_min, 
           tm->tm_sec,
           st->ts_usec / 1000 );
}

void SnifLoop( int fd, int device  )
{
    fd_set  rfds;
    struct usbmon_packet hdr;
    struct mon_bin_get ev;
    
    memset( &ev,0,sizeof( ev ) );
    ev.hdr = &hdr; 
    ev.alloc = BUFF_LEN;
    ev.data = malloc( BUFF_LEN );
    if( NULL == ev.data)
    {
      perror("\nmalloc");
      return;
    }
    
    while( 1 )
    {
       FD_ZERO( &rfds );
       FD_SET( fd, &rfds );
       if( -1 == select( fd+1, &rfds, NULL, NULL, NULL ) )
           break;
       
        int size = ioctl( fd, MON_IOCQ_URB_LEN, 0 );
        if( size > 0 )
        {
           void *aux = realloc( ev.data, size );
           if( NULL != aux )
           {
               ev.data  = aux;
               ev.alloc = size; 
           }
        }
        hdr.len_cap = 0;
        if( !ioctl( fd, MON_IOCX_GET, &ev ) && hdr.len_cap )
        {
            int i;
            char bHasPrint = 0;
            
            if( -1 != device )
            {
                if( hdr.devnum != device )
                {
                    continue;
                }
            }
            
            pTm(&hdr);
            if( 'C' == hdr.type ) 
            {
                printf( "<-Rcv(%s:%02x,Bus:%d,Dev:%02x)Len:%d: ", 
                        PIPE[hdr.xfer_type], 
                        hdr.epnum, 
                        hdr.busnum, 
                        hdr.devnum,
                        hdr.len_cap );
            } 
            else if( 'S' == hdr.type ) 
            {
                printf( "->Snd(%s:%02x,Bus:%d,Dev:%02x)Len:%d: ",
                        PIPE[hdr.xfer_type], 
                        hdr.epnum, 
                        hdr.busnum, 
                        hdr.devnum,
                        hdr.len_cap);
            } 
            else 
                printf("Ev.Type=%c: ", hdr.type);

            for( i=0; i<hdr.len_cap; i++ ) 
            {
                printf("%02X ", ((unsigned char *)ev.data)[i]);
                if( isprint(((unsigned char *)ev.data)[i]))
                   bHasPrint = 1;
            }
            printf("\n");
            if( bHasPrint )
            {
                printf("[");
                for( i=0; i<hdr.len_cap; i++ ) 
                    if( isprint(((unsigned char *)ev.data)[i]))
                        printf("%c", ((unsigned char *)ev.data)[i]);
                    else
                        printf(".");
                printf("]\n");
            }
            fflush( stdout );
        }
    }

    perror("\nselect()");

    free( ev.data );
}

void menu( char *pProcess )
{
    fprintf(stderr, "Usage: %s [options] /dev/usbmon[n]\n", pProcess);
    fprintf(stderr, "[options] \n");
    fprintf(stderr, "  -tN triger only device number N. \n   Obs: The device number is specified by \"Dev:xx\" in each sniffer line.\n");
    fprintf(stderr, " Eg: \n");
    fprintf(stderr, "   1. %s -t03 /dev/usbmon0\n", pProcess);
    fprintf(stderr, "   2. %s /dev/usbmon0\n\n", pProcess);
    fprintf(stderr, "Note: usbmon driver must be loaded before you try %s\n", pProcess);
    
}

int main( int argc, char ** argv ) 
{
    int fd;
    char device[3];
    
    if( ( argc != 2 ) && ( argc != 3) || 
	  ( (argc > 2) && ( ( strlen(argv[1] )!=4 ) || strncmp( argv[1],"-t",2 ) ) ) )
    {
        menu(argv[0]);
        return 1;
    }
    
    if( argc == 3 )
        strcpy( device, &argv[1][2] );
    else
        strcpy( device, "-1" );
    
    fd = open( argv[argc-1], O_RDONLY );
    if( fd < 0 ) 
    {
        perror( argv[argc-1] );
        return 1;
    }

    SnifLoop( fd, atoi( device) );
    
    close( fd );
    
    return 0;
}
