/*
 * dfu-programmer
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#ifdef HAVE_LIBUSB_1_0
#include <libusb.h>
#else
#include <usb.h>
#endif

#include "config.h"
#include "dfu-device.h"
#include "dfu.h"
#include "atmel.h"
#include "arguments.h"
#include "commands.h"
#include "main.h"
#include <stdlib.h> //for malloc

int debug;
#ifdef HAVE_LIBUSB_1_0
libusb_context *usbcontext;
#endif

int dfuprog_virtual_cmd(char* commandLine){
    //Initialise variables for calculating argc.
    int argc = 1;
    char *temp = commandLine;
    const char delimiter[2] = " ";

    //Keep reading until a null terminator to find argc
    while(*temp){
        argc+=(*temp==delimiter[0]);
        temp++;
    }
    fprintf(stderr, "argc = %d\n", argc);


    //Setup variables for argv.  We need to make a copt of the commandLine string since strtok will try to edit it (and we may not have permission!)
    char *commandLine_copy = (char *)malloc(strlen(commandLine));
    strcpy(commandLine_copy, commandLine);
    char *argv[argc];

    //Extract the arguments;
    int i;
    argv[0] = strtok(commandLine_copy, delimiter);
    fprintf(stderr, "argv[0] = %s\n", argv[0]);
    for(i=1; i<argc; i++){
        argv[i] = strtok(NULL, delimiter);
        fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);
    }

        fprintf(stderr, "attempting to call main()\n");
    return dfuprog_virtual_main(argc, argv);
}

int dfuprog_virtual_main(int argc, char **argv){
    static const char *progname = PACKAGE;
    int retval = SUCCESS;
    int status;
    dfu_device_t dfu_device;
    struct programmer_arguments args;
#ifdef HAVE_LIBUSB_1_0
    struct libusb_device *device = NULL;
#else
    struct usb_device *device = NULL;
#endif

    memset( &args, 0, sizeof(args) );
    memset( &dfu_device, 0, sizeof(dfu_device) );

    status = parse_arguments(&args, argc, argv);
    if( status < 0 ) {
        /* Exit with an error. */
        return ARGUMENT_ERROR;
    } else if (status > 0) {
        /* It was handled by parse_arguments. */
        return SUCCESS;
    }

#ifdef HAVE_LIBUSB_1_0
    if (libusb_init(&usbcontext)) {
        fprintf( stderr, "%s: can't init libusb.\n", progname );
        return DEVICE_ACCESS_ERROR;
    }
#else
    usb_init();
#endif

    if( debug >= 200 ) {
#ifdef HAVE_LIBUSB_1_0
        libusb_set_debug(usbcontext, debug );
#else
        usb_set_debug( debug );
#endif
    }

    if( !(args.command == com_bin2hex || args.command == com_hex2bin) ) {
        device = dfu_device_init( args.vendor_id, args.chip_id,
                                  args.bus_id, args.device_address,
                                  &dfu_device,
                                  args.initial_abort,
                                  args.honor_interfaceclass );

        if( NULL == device ) {
            fprintf( stderr, "%s: no device present.\n", progname );
            retval = DEVICE_ACCESS_ERROR;
            goto error;
        }
    }

    if( 0 != (retval = execute_command(&dfu_device, &args)) ) {
        /* command issued a specific diagnostic already */
        goto error;
    }

error:
    if( NULL != dfu_device.handle ) {
        int rv;

#ifdef HAVE_LIBUSB_1_0
        rv = libusb_release_interface( dfu_device.handle, dfu_device.interface );
#else
        rv = usb_release_interface( dfu_device.handle, dfu_device.interface );
#endif
        /* The RESET command sometimes causes the usb_release_interface command to fail.
           It is not obvious why this happens but it may be a glitch due to the hardware
           reset in the attached device. In any event, since reset causes a USB detach
           this should not matter, so there is no point in raising an alarm.
        */
        if( 0 != rv && !(com_launch == args.command &&
                args.com_launch_config.noreset == 0) ) {
            fprintf( stderr, "%s: failed to release interface %d.\n",
                             progname, dfu_device.interface );
            retval = DEVICE_ACCESS_ERROR;
        }
    }

    if( NULL != dfu_device.handle ) {
#ifdef HAVE_LIBUSB_1_0
        libusb_close(dfu_device.handle);
#else
        if( 0 != usb_close(dfu_device.handle) ) {
            fprintf( stderr, "%s: failed to close the handle.\n", progname );
            retval = DEVICE_ACCESS_ERROR;
        }
#endif
    }

#ifdef HAVE_LIBUSB_1_0
    libusb_exit(usbcontext);
#endif

    return retval;
}

int main( int argc, char **argv )
{
    static const char *progname = PACKAGE;
    int retval = SUCCESS;
    int status;
    dfu_device_t dfu_device;
    struct programmer_arguments args;
#ifdef HAVE_LIBUSB_1_0
    struct libusb_device *device = NULL;
#else
    struct usb_device *device = NULL;
#endif

    memset( &args, 0, sizeof(args) );
    memset( &dfu_device, 0, sizeof(dfu_device) );

    status = parse_arguments(&args, argc, argv);
    if( status < 0 ) {
        /* Exit with an error. */
        return ARGUMENT_ERROR;
    } else if (status > 0) {
        /* It was handled by parse_arguments. */
        return SUCCESS;
    }

#ifdef HAVE_LIBUSB_1_0
    if (libusb_init(&usbcontext)) {
        fprintf( stderr, "%s: can't init libusb.\n", progname );
        return DEVICE_ACCESS_ERROR;
    }
#else
    usb_init();
#endif

    if( debug >= 200 ) {
#ifdef HAVE_LIBUSB_1_0
        libusb_set_debug(usbcontext, debug );
#else
        usb_set_debug( debug );
#endif
    }

    if( !(args.command == com_bin2hex || args.command == com_hex2bin) ) {
        device = dfu_device_init( args.vendor_id, args.chip_id,
                                  args.bus_id, args.device_address,
                                  &dfu_device,
                                  args.initial_abort,
                                  args.honor_interfaceclass );

        if( NULL == device ) {
            fprintf( stderr, "%s: no device present.\n", progname );
            retval = DEVICE_ACCESS_ERROR;
            goto error;
        }
    }

    if( 0 != (retval = execute_command(&dfu_device, &args)) ) {
        /* command issued a specific diagnostic already */
        goto error;
    }

error:
    if( NULL != dfu_device.handle ) {
        int rv;

#ifdef HAVE_LIBUSB_1_0
        rv = libusb_release_interface( dfu_device.handle, dfu_device.interface );
#else
        rv = usb_release_interface( dfu_device.handle, dfu_device.interface );
#endif
        /* The RESET command sometimes causes the usb_release_interface command to fail.
           It is not obvious why this happens but it may be a glitch due to the hardware
           reset in the attached device. In any event, since reset causes a USB detach
           this should not matter, so there is no point in raising an alarm.
        */
        if( 0 != rv && !(com_launch == args.command &&
                args.com_launch_config.noreset == 0) ) {
            fprintf( stderr, "%s: failed to release interface %d.\n",
                             progname, dfu_device.interface );
            retval = DEVICE_ACCESS_ERROR;
        }
    }

    if( NULL != dfu_device.handle ) {
#ifdef HAVE_LIBUSB_1_0
        libusb_close(dfu_device.handle);
#else
        if( 0 != usb_close(dfu_device.handle) ) {
            fprintf( stderr, "%s: failed to close the handle.\n", progname );
            retval = DEVICE_ACCESS_ERROR;
        }
#endif
    }

#ifdef HAVE_LIBUSB_1_0
    libusb_exit(usbcontext);
#endif

    return retval;
}
