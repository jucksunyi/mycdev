/********************
mycdev.h
********************/

#ifndef _MYCDEV_H_
#define _MYCDEV_H_

#ifndef MYCDEV_MAJOR
#define MYCDEV_MAJOR 266
#endif

#ifndef MYCDEV_NR_DEVS
#define MYCDEV_NR_DEVS 2
#endif

#ifndef MYCDEV_SIZE
#define MYCDEV_SIZE 4096
#endif

struct mycdev
{
	char *data;
	unsigned long size;
};

#endif
