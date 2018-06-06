#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define FILE	"/dev/mycdev"		
#define SIZE_BUF  33

int main(void)
{
	int fd = -1, i;
	char *buf ;
	
    unsigned long addr_phyim = 0;
    unsigned long addr_lcnt = 0;
	unsigned long addr_lcnt2 = 0;
	addr_phyim    = 4272554372;	//0xFEAA0184;
	addr_lcnt = 4272554336;  //0xFEAA0160;	
	addr_lcnt2 = 0xFEAA0164;
	
	fd = open(FILE, O_RDWR);
	if (fd < 0)
	{
		printf("open %s error.\n", FILE);
		return -1;
	}
	buf = (char *)malloc(SIZE_BUF * sizeof(char));
	*(buf + SIZE_BUF) = '\0';
 	memset(buf, 0 , strlen(buf));
   	sprintf(buf, "%lu", addr_lcnt);

	printf("the user buf is :%s\n", buf);

	
	write(fd, buf, strlen(buf));
	memset(buf, 0 , strlen(buf));
	read(fd, buf, 32);

	printf("the user buf is :%d\n", atoi(buf));
	
	close(fd);
	
	return 0;
}
