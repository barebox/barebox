/*
 * (C) Copyright 2001
 * Denis Peter, MPL AG Switzerland
 *
 * Part of this source has been derived from the Linux USB
 * project.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */
#include <common.h>
#include <devices.h>

#ifdef CONFIG_USB_KEYBOARD

#include <usb.h>

#undef USB_KBD_DEBUG
/*
 * if overwrite_console returns 1, the stdin, stderr and stdout
 * are switched to the serial port, else the settings in the
 * environment are used
 */
#ifdef CFG_CONSOLE_OVERWRITE_ROUTINE
extern int overwrite_console (void);
#else
int overwrite_console (void)
{
	return (0);
}
#endif

#ifdef	USB_KBD_DEBUG
#define	USB_KBD_PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define USB_KBD_PRINTF(fmt,args...)
#endif


#define REPEAT_RATE  40/4 /* 40msec -> 25cps */
#define REPEAT_DELAY 10 /* 10 x REAPEAT_RATE = 400msec */

#define NUM_LOCK	0x53
#define CAPS_LOCK 0x39
#define SCROLL_LOCK 0x47


/* Modifier bits */
#define LEFT_CNTR		0
#define LEFT_SHIFT	1
#define LEFT_ALT		2
#define LEFT_GUI		3
#define RIGHT_CNTR	4
#define RIGHT_SHIFT	5
#define RIGHT_ALT		6
#define RIGHT_GUI		7

#define USB_KBD_BUFFER_LEN 0x20  /* size of the keyboardbuffer */

static volatile char usb_kbd_buffer[USB_KBD_BUFFER_LEN];
static volatile int usb_in_pointer = 0;
static volatile int usb_out_pointer = 0;

unsigned char new[8];
unsigned char old[8];
int repeat_delay;
#define DEVNAME "usbkbd"
static unsigned char num_lock = 0;
static unsigned char caps_lock = 0;
static unsigned char scroll_lock = 0;

static unsigned char leds __attribute__ ((aligned (0x4)));

static unsigned char usb_kbd_numkey[] = {
	 '1', '2', '3', '4', '5', '6', '7', '8', '9', '0','\r',0x1b,'\b','\t',' ', '-',
	 '=', '[', ']','\\', '#', ';', '\'', '`', ',', '.', '/'
};
static unsigned char usb_kbd_numkey_shifted[] = {
	 '!', '@', '#', '$', '%', '^', '&', '*', '(', ')','\r',0x1b,'\b','\t',' ', '_',
	 '+', '{', '}', '|', '~', ':', '"', '~', '<', '>', '?'
};

/******************************************************************
 * Queue handling
 ******************************************************************/
/* puts character in the queue and sets up the in and out pointer */
static void usb_kbd_put_queue(char data)
{
	if((usb_in_pointer+1)==USB_KBD_BUFFER_LEN) {
		if(usb_out_pointer==0) {
			return; /* buffer full */
		} else{
			usb_in_pointer=0;
		}
	} else {
		if((usb_in_pointer+1)==usb_out_pointer)
			return; /* buffer full */
		usb_in_pointer++;
	}
	usb_kbd_buffer[usb_in_pointer]=data;
	return;
}

/* test if a character is in the queue */
static int usb_kbd_testc(void)
{
	if(usb_in_pointer==usb_out_pointer)
		return(0); /* no data */
	else
		return(1);
}
/* gets the character from the queue */
static int usb_kbd_getc(void)
{
	char c;
	while(usb_in_pointer==usb_out_pointer);
	if((usb_out_pointer+1)==USB_KBD_BUFFER_LEN)
		usb_out_pointer=0;
	else
		usb_out_pointer++;
	c=usb_kbd_buffer[usb_out_pointer];
	return (int)c;

}

/* forward decleration */
static int usb_kbd_probe(struct usb_device *dev, unsigned int ifnum);

/* search for keyboard and register it if found */
int drv_usb_kbd_init(void)
{
	int error,i,index;
	device_t usb_kbd_dev,*old_dev;
	struct usb_device *dev;
	char *stdinname  = getenv ("stdin");

	usb_in_pointer=0;
	usb_out_pointer=0;
	/* scan all USB Devices */
	for(i=0;i<USB_MAX_DEVICE;i++) {
		dev=usb_get_dev_index(i); /* get device */
		if(dev->devnum!=-1) {
			if(usb_kbd_probe(dev,0)==1) { /* Ok, we found a keyboard */
				/* check, if it is already registered */
				USB_KBD_PRINTF("USB KBD found set up device.\n");
				for (index=1; index<=ListNumItems(devlist); index++) {
					old_dev = ListGetPtrToItem(devlist, index);
					if(strcmp(old_dev->name,DEVNAME)==0) {
						/* ok, already registered, just return ok */
						USB_KBD_PRINTF("USB KBD is already registered.\n");
						return 1;
					}
				}
				/* register the keyboard */
				USB_KBD_PRINTF("USB KBD register.\n");
				memset (&usb_kbd_dev, 0, sizeof(device_t));
				strcpy(usb_kbd_dev.name, DEVNAME);
				usb_kbd_dev.flags =  DEV_FLAGS_INPUT | DEV_FLAGS_SYSTEM;
				usb_kbd_dev.putc = NULL;
				usb_kbd_dev.puts = NULL;
				usb_kbd_dev.getc = usb_kbd_getc;
				usb_kbd_dev.tstc = usb_kbd_testc;
				error = device_register (&usb_kbd_dev);
				if(error==0) {
					/* check if this is the standard input device */
					if(strcmp(stdinname,DEVNAME)==0) {
						/* reassign the console */
						if(overwrite_console()) {
							return 1;
						}
						error=console_assign(stdin,DEVNAME);
						if(error==0)
							return 1;
						else
							return error;
					}
					return 1;
				}
				return error;
			}
		}
	}
	/* no USB Keyboard found */
	return -1;
}


/* deregistering the keyboard */
int usb_kbd_deregister(void)
{
	return device_deregister(DEVNAME);
}

/**************************************************************************
 * Low Level drivers
 */

/* set the LEDs. Since this is used in the irq routine, the control job
   is issued with a timeout of 0. This means, that the job is queued without
   waiting for job completion */

static void usb_kbd_setled(struct usb_device *dev)
{
	struct usb_interface_descriptor *iface;
	iface = &dev->config.if_desc[0];
	leds=0;
	if(scroll_lock!=0)
		leds|=1;
	leds<<=1;
	if(caps_lock!=0)
		leds|=1;
	leds<<=1;
	if(num_lock!=0)
		leds|=1;
	usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_SET_REPORT, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
 		0x200, iface->bInterfaceNumber,(void *)&leds, 1, 0);

}


#define CAPITAL_MASK 0x20
/* Translate the scancode in ASCII */
static int usb_kbd_translate(unsigned char scancode,unsigned char modifier,int pressed)
{
	unsigned char keycode;

	if(pressed==0) {
		/* key released */
 		repeat_delay=0;
		return 0;
	}
	if(pressed==2) {
		repeat_delay++;
		if(repeat_delay<REPEAT_DELAY)
			return 0;
		repeat_delay=REPEAT_DELAY;
	}
	keycode=0;
	if((scancode>3) && (scancode<0x1d)) { /* alpha numeric values */
		keycode=scancode-4 + 0x61;
		if(caps_lock)
			keycode&=~CAPITAL_MASK; /* switch to capital Letters */
		if(((modifier&(1<<LEFT_SHIFT))!=0)||((modifier&(1<<RIGHT_SHIFT))!=0)) {
			if(keycode & CAPITAL_MASK)
				keycode&=~CAPITAL_MASK; /* switch to capital Letters */
			else
				keycode|=CAPITAL_MASK; /* switch to non capital Letters */
		}
	}
	if((scancode>0x1d) && (scancode<0x3A)) {
		if(((modifier&(1<<LEFT_SHIFT))!=0)||((modifier&(1<<RIGHT_SHIFT))!=0))  /* shifted */
			keycode=usb_kbd_numkey_shifted[scancode-0x1e];
		else /* non shifted */
			keycode=usb_kbd_numkey[scancode-0x1e];
	}
	if(pressed==1) {
		if(scancode==NUM_LOCK) {
			num_lock=~num_lock;
			return 1;
		}
		if(scancode==CAPS_LOCK) {
			caps_lock=~caps_lock;
			return 1;
		}
		if(scancode==SCROLL_LOCK) {
			scroll_lock=~scroll_lock;
			return 1;
		}
	}
	if(keycode!=0) {
		USB_KBD_PRINTF("%c",keycode);
		usb_kbd_put_queue(keycode);
	}
	return 0;
}

/* Interrupt service routine */
static int usb_kbd_irq(struct usb_device *dev)
{
	int i,res;

	if((dev->irq_status!=0)||(dev->irq_act_len!=8))
	{
		USB_KBD_PRINTF("usb_keyboard Error %lX, len %d\n",dev->irq_status,dev->irq_act_len);
		return 1;
	}
	res=0;
	for (i = 2; i < 8; i++) {
		if (old[i] > 3 && memscan(&new[2], old[i], 6) == &new[8]) {
			res|=usb_kbd_translate(old[i],new[0],0);
		}
		if (new[i] > 3 && memscan(&old[2], new[i], 6) == &old[8]) {
			res|=usb_kbd_translate(new[i],new[0],1);
		}
	}
	if((new[2]>3) && (old[2]==new[2])) /* still pressed */
		res|=usb_kbd_translate(new[2],new[0],2);
	if(res==1)
		usb_kbd_setled(dev);
	memcpy(&old[0],&new[0], 8);
	return 1; /* install IRQ Handler again */
}

/* probes the USB device dev for keyboard type */
static int usb_kbd_probe(struct usb_device *dev, unsigned int ifnum)
{
	struct usb_interface_descriptor *iface;
	struct usb_endpoint_descriptor *ep;
	int pipe,maxp;

	if (dev->descriptor.bNumConfigurations != 1) return 0;
	iface = &dev->config.if_desc[ifnum];

	if (iface->bInterfaceClass != 3) return 0;
	if (iface->bInterfaceSubClass != 1) return 0;
	if (iface->bInterfaceProtocol != 1) return 0;
	if (iface->bNumEndpoints != 1) return 0;

	ep = &iface->ep_desc[0];

	if (!(ep->bEndpointAddress & 0x80)) return 0;
	if ((ep->bmAttributes & 3) != 3) return 0;
	USB_KBD_PRINTF("USB KBD found set protocol...\n");
	/* ok, we found a USB Keyboard, install it */
	/* usb_kbd_get_hid_desc(dev); */
	usb_set_protocol(dev, iface->bInterfaceNumber, 0);
	USB_KBD_PRINTF("USB KBD found set idle...\n");
	usb_set_idle(dev, iface->bInterfaceNumber, REPEAT_RATE, 0);
	memset(&new[0], 0, 8);
	memset(&old[0], 0, 8);
	repeat_delay=0;
	pipe = usb_rcvintpipe(dev, ep->bEndpointAddress);
	maxp = usb_maxpacket(dev, pipe);
	dev->irq_handle=usb_kbd_irq;
	USB_KBD_PRINTF("USB KBD enable interrupt pipe...\n");
	usb_submit_int_msg(dev,pipe,&new[0], maxp > 8 ? 8 : maxp,ep->bInterval);
	return 1;
}



#endif /* CONFIG_USB_KEYBOARD */
