/*
 * uhidinfo.c
 *   display uhid device information
 *    - vender id
 *    - product id
 *    - interface number
 *    - report id
 *   and atlernative name from device list
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

struct devinfo {
	int vid;	/* vender id */
	int pid;	/* product id */
	int ino;	/* interface number */
	int rid;	/* report id */
};


#define ALTNAMEMAX 256
struct altinfo {
	struct devinfo dev;
	char altdev[ALTNAMEMAX];
};

#define ALTMAX 256
static struct altinfo devlist[ALTMAX];

static int devlistnum;
static unsigned int quiet = 0;

#ifndef ALTER_DEVLIST
#  define ALTER_DEVLIST "/usr/local/etc/uhidlist"
#endif

int loadDevlist() {
	int count = 0;
	FILE *fp;
	size_t linesize = 0;
	ssize_t linelen;
	char *line = NULL;
	if ((fp = fopen(ALTER_DEVLIST,"r")) == NULL) {
		if (!quiet) perror("fopen");
		exit(1);
	}
	while ((linelen = getline(&line, &linesize, fp)) != -1) {
		int vid=-1,pid=-1,ino=-1, rid=-1;
		int n;
		char altname[256]="";
		n=sscanf(line, "%x %x %d %d %255s%*[^\n]",&vid,&pid,&ino,&rid,altname);
		if ( n == 5 ) {
			devlist[count].dev.vid=vid;
			devlist[count].dev.pid=pid;
			devlist[count].dev.ino=ino;
			devlist[count].dev.rid=rid;
			strlcpy(devlist[count].altdev,altname,ALTNAMEMAX);
			count++;
			devlistnum=count;
			if (count >= ALTMAX)
				break;
		}
	}
	fclose(fp);
	return 0;
}

int searchDevice(struct devinfo *search) {
	int i;
	for (i=0;i< devlistnum;i++)
		if (search->pid == devlist[i].dev.pid &&
		    search->vid == devlist[i].dev.vid &&
		    search->rid == devlist[i].dev.rid && 
		    ((search->ino >= 0)?(search->ino == devlist[i].dev.ino):1))
			return i;
	return -1;
}

void printInfo(char *dev, struct devinfo *hiddevinfo){
	char *format;
	if ( hiddevinfo->ino == -1 ) {
		if ( quiet ) 
			format = "%s %4.4X %4.4X ? %d\n";
		else
			format = "%s: Vendor=%4.4X Product=%4.4X Interface=? Report=%d\n";
		printf(format,
		       dev,
		       hiddevinfo->vid,
		       hiddevinfo->pid,
		       hiddevinfo->rid);
	} else {
		if ( quiet ) 
			format = "%s %4.4X %4.4X %d %d\n";
		else
			format = "%s: Vendor=%4.4X Product=%4.4X Interface=%d Report=%d\n";
		printf(format,
		       dev,
		       hiddevinfo->vid,	
		       hiddevinfo->pid,	
		       hiddevinfo->ino,
		       hiddevinfo->rid);
	}
}

void usage() {
	printf("Usage:\n");
	printf("  uhidinfo [-sq] {device ...}\n");
	printf("\n");
	printf("    Options:\n");
	printf("      -s  search and report alternative name (one device only)\n");
	printf("      -q  quiet mode\n");
	exit(1);
}


int getUhidInfo(char *dev, struct devinfo *hiddevinfo){
	struct usb_device_info udev_info;
	int hidfd;
	int report_id;
	usb_interface_descriptor_t idesc;

	if ((hidfd = open(dev, O_RDWR,O_NONBLOCK)) < 0) {
		if (!quiet) perror(dev);
		return -1;
	}
	if (ioctl(hidfd, USB_GET_DEVICEINFO, &udev_info)) {
		if (!quiet) perror("USB_GET_DEVICEINFO");
		return -1;
	}
	hiddevinfo->vid=udev_info.udi_vendorNo;
	hiddevinfo->pid=udev_info.udi_productNo;

	if (ioctl(hidfd, USB_GET_REPORT_ID, &report_id)) {
		if (!quiet) perror("USB_GET_REPORT_ID");
		return -1;
	}

	hiddevinfo->rid=report_id;

	if (ioctl(hidfd, USB_GET_INTERFACE_DESC, &idesc)) {
		if (errno == EINVAL) 
			hiddevinfo->ino=-1;
		else {
			if (!quiet) perror("USB_GET_INTERFACE_DESC");
			return -1;
		}
	} else 
		hiddevinfo->ino=idesc.bInterfaceNumber;
	return 0;
}

int main(int argc, char **argv) {

	int flags,info=0,altname=0;
	int d;
	char *dev;
	extern int optind;

	struct devinfo hiddevinfo;
	struct usb_device_info *udev_info;

	if (argc < 2) {
		usage();
	}

	while ((flags = getopt(argc, argv, "sq")) != -1){
		switch(flags){
		case 's':
			altname=1;
			break;
		case 'q':
			quiet = 1;
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if ( altname ) {
		loadDevlist();
		if ( argc != 1 )
			usage();
		dev = *argv;
		if(!getUhidInfo(dev, &hiddevinfo)) {
			d = searchDevice(&hiddevinfo);
			if (d >= 0) {
				if (!quiet)
					printf("%s: ", dev);
				printf("%s\n",devlist[d].altdev);
			} else 
				exit(1);
		}
	} else while (argc-- >0) {
		dev = *argv++;
		if (!getUhidInfo(dev, &hiddevinfo))
			printInfo(dev, &hiddevinfo);
	}
}
