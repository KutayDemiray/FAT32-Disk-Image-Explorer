#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "fat.h"

int readsector (int fd, unsigned char *buf, unsigned int snum) {
	off_t offset;
	int n;
	offset = snum * SECTOR_SIZE;
	lseek (fd, offset, SEEK_SET);
	n = read (fd, buf, SECTOR_SIZE);
	if (n == SECTOR_SIZE)
		return (0);
	else
		return(-1);
}

int writesector (int fd, unsigned char *buf, unsigned int snum) {
	off_t offset;
	int n;
	offset = snum * SECTOR_SIZE;
	lseek (fd, offset, SEEK_SET);
	n = write (fd, buf, SECTOR_SIZE);
	fsync (fd); // write without delayed-writing
	if (n == SECTOR_SIZE)
		return (0); // success
	else
		return(-1);
}

int readcluster (int fd, unsigned char *buf, unsigned int cnum) {
	off_t offset;
	int n;
	unsigned int snum; // sector number
	snum = DATA_START_SECTOR + (cnum - 2) * (CLUSTER_SIZE / SECTOR_SIZE);
	offset = snum * SECTOR_SIZE;
	lseek (fd, offset, SEEK_SET);
	n = read (fd, buf, CLUSTER_SIZE);
	if (n == CLUSTER_SIZE)
		return (0); // success
	else
		return (-1);
}

int fd;
unsigned char sector[SECTOR_SIZE];
unsigned char cluster[CLUSTER_SIZE];

int main(int argc, char** argv) {
	params p;
	process_args(argc, argv, &p);
	
	//unsigned char sector[SECTOR_SIZE];
	//unsigned char cluster[CLUSTER_SIZE];
	fd = open(p.diskimage, O_SYNC | O_RDONLY); // disk fd
	readsector(fd, sector, 0); // read sector #0
	readcluster(fd, cluster, 3); // read cluster #3
	// always check the return values
	int i;
	for (i = 0; i < CLUSTER_SIZE; i++) {
		printf("%02X  ", cluster[i]);
		//printf("%c ", sector[i]); 
		if (i % 16 == 15) {
			printf("\n");
		} 
	}
	return 0;
}

void process_args(int argc, char** argv, params* p) {
	p->diskimage = argv[1];
}

void print_image_info(int fd);
