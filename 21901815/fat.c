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

// 4. fat DISKIMAGE -t
void print_name_recursive(int fd, int cnum, unsigned char* buf){
	struct msdos_dir_entry* cluster;
	readcluster(fd, buf, cnum);
	cluster = (msdos_dir_entry*) buf;
	
	int i;
	for (i = 0; i <32; i++ ){
		if (cluster->attr == 0x08 || cluster->attr == 0x10) { // the allocation belongs to a directory
			printf("(d)\t%s\n", cluster->name);
			
			// next cluster information retrieval
			unsigned short int start = (unsigned short int) cluster->start;
			unsigned short int starthi = (unsigned short int) cluster->starthi;
			unsigned int start_address = find_start(start, starthi);
			print_name_recursive(fd, start_address, buf);
		}
		else if (cluster->attr <= 0x07){ // the allocation belongs to a file
			printf("(f)\t%s\n", cluster->name);
		}
		
		cluster++;	
	}
}

// 5. fat DISKIMAGE -a PATH
void print_content_recursive(int fd, int cnum, unsigned char* buf, char* path){
	struct msdos_dir_entry* cluster;
	readcluster(fd, buf, cnum);
	cluster = (msdos_dir_entry*) buf;
	int i;
	for (i = 0; i<32; i++){
		if (cluster-> name != path){
			if (cluster->attr == 0x08 || cluster->attr == 0x10){
				unsigned short int start = (unsigned short int) cluster->start;
				unsigned short int starthi = (unsigned short int) cluster->starthi;
				unsigned int start_address = find_start(start, starthi);
				print_content_recursive(fd, start_address, buf, path);
			}
		}
		else {
			
		}
		
	}
}

// helper for 4 and 5
unsigned int find_start(unsigned short int start, unsigned short int starthi){
	unsigned int start_address;
	unsigned short int tmp = start;
	unsigned short int tmp2 = tmp & 0x00FF;
	tmp2 = tmp2 << 8;
	tmp = tmp & 0xFF00;
	tmp = tmp >> 8; 
	unsigned short int lsp = tmp | tmp2;
	tmp = starthi;
	tmp2 = tmp & 0x00FF;
	tmp2 = tmp2 << 8;
	tmp = tmp & 0xFF00;
	tmp = tmp >> 8; 
	unsigned short int msp = tmp |tmp2;
	unsigned int tmp3 = (unsigned int)msp;
	tmp3 = tmp3 << 16;
	start_address = (unsigned int) lsp;
	start_address = start_address | tmp3;
	return start_address;
}


