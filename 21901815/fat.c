#include <stdio.h>
#include <string.h> 
#include <fcntl.h>
#include <unistd.h>
#include <linux/msdos_fs.h>
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
	  
	//readsector(fd, sector, 0); // read sector #0
	readcluster(fd, cluster, 3); // read cluster #3 
	
	// always check the return values
	int i;
	/*
	for (i = 0; i < SECTOR_SIZE; i++) {
		//printf("%02X  ", cluster[i]);
		printf("%02X ", sector[i]);  
		if (i % 16 == 15) {
			printf("\n");
		} 
	}
	*/
	
	/*
	for (i = 0; i < CLUSTER_SIZE; i++) {
		printf("%02X  ", cluster[i]); 
		if (i % 16 == 15) {
			printf("\n");
		} 
	}

	*/
	print_image_info();
	
	close(fd);
	return 0;
}

void process_args(int argc, char** argv, params* p) {
	p->diskimage = argv[1];
}

// 4. fat DISKIMAGE -t
void print_name_recursive(int fd, int cnum, unsigned char* buf){
	struct msdos_dir_entry* cluster;
	readcluster(fd, buf, cnum);
	cluster = (struct msdos_dir_entry*) buf;
	
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
	cluster = (struct msdos_dir_entry*) buf;
	int i;
	for (i = 0; i<32; i++){
		if (strcmp((char *) (cluster->name), path) == 0){
			if (cluster->attr == 0x08 || cluster->attr == 0x10){
				unsigned short int start = (unsigned short int) cluster->start;
				unsigned short int starthi = (unsigned short int) cluster->starthi;
				unsigned int start_address = find_start(start, starthi);
				print_content_recursive(fd, start_address, buf, path); 
			}
		}
		else {
			// The path is found. Now we need to traverse the file content.
			unsigned int bit_size = (cluster->size)*8; 
			
			// find the data start
			unsigned short int start = (unsigned short int) cluster->start;
			unsigned short int starthi = (unsigned short int) cluster->starthi;
			unsigned int start_address = find_start(start, starthi);
			readcluster(fd, buf, start_address);
			cluster = (struct msdos_dir_entry*) buf;
			
			// print the file content
			int j;
			for (j = 0; j < bit_size; j++){
				printf("%c", cluster[j]);
			}
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

void print_image_info() {
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
	
	char fs_type[FS_TYPE_SIZE + 1]; 
	int i;
	for (i = 0; i < FS_TYPE_SIZE; i++) {
		fs_type[i] = bp->fat32.fs_type[i];
	}
	fs_type[FS_TYPE_SIZE] = '\0';
	printf("File system type: %s\n", fs_type);
	
	char vol_label[VOLUME_LABEL_SIZE + 1];
	for (i = 0; i < VOLUME_LABEL_SIZE; i++) {
		vol_label[i] = bp->fat32.vol_label[i];
	}
	vol_label[VOLUME_LABEL_SIZE] = '\0';
	printf("Volume label: %s\n", vol_label);
	
	printf("Number of sectors in disk: %u\n", bp->total_sect);  
	
	printf("Sector size in bytes: %u\n", ((unsigned short int *) bp->sector_size)[0]);
	
	printf("Number of reserved sectors: %u\n", bp->reserved);
	
	printf("Number of sectors per FAT table: %u\n", bp->fat32.length);
	
	printf("Number of FAT tables: %u\n", bp->fats);
	
	printf("Number of sectors per cluster: %u\n", bp->sec_per_clus);
	
	printf("Number of clusters: %u\n", bp->total_sect / bp->sec_per_clus);
	
	printf("Data region starts at sector: %u\n", bp->reserved + bp->fats * bp->fat32.length);
	
	printf("Root directory starts at sector: %u\n", bp->reserved + bp->fats * bp->fat32.length + bp->sec_per_clus * (bp->fat32.root_cluster - 2));
	
	printf("Root directory starts at cluster: %u\n", bp->fat32.root_cluster);
	
	printf("Disk size in bytes: %u bytes\n", ((unsigned short int *) bp->sector_size)[0] * bp->total_sect);
	
	printf("Disk size in megabytes: %u MB\n", (((unsigned short int *) bp->sector_size)[0] * bp->total_sect) >> 20);
	
	printf("Number of used clusters: %u\n", 0); // TODO =======================================================================
	
	printf("Number of free clusters: ");
	
	/*
	char fs_type[FS_TYPE_SIZE + 1];
	int i;
	for (i = 0; i < FS_TYPE_SIZE; i++) {
		fs_type[i] = sector[FS_TYPE_OFFSET + i];
	}
	fs_type[FS_TYPE_SIZE] = '\0';
	printf("File system type: %s\n", fs_type);
	
	char volume_label[VOLUME_LABEL_SIZE + 1];
	for (i = 0; i < VOLUME_LABEL_SIZE; i++) {
		volume_label[i] = sector[VOLUME_LABEL_OFFSET + i];
	}
	volume_label[VOLUME_LABEL_SIZE] = '\0';
	printf("Volume label: %s\n", volume_label);
	
	unsigned int sectors_in_disk;
	char buf[4]; // NO_OF_SECTORS_IN_DISK_SIZE = 4, but this buffer will be used for other similarly sized values as well
	for (i = 0; i < NO_OF_SECTORS_IN_DISK_SIZE; i++) {
		buf[i] = sector[NO_OF_SECTORS_IN_DISK_OFFSET + i];
	}
	sectors_in_disk = ((unsigned int *) buf)[0];
	printf("Number of sectors in disk: %u\n", sectors_in_disk);
	
	unsigned int sector_size_in_bytes;
	for (i = 0; i < SECTOR_SIZE_IN_BYTES_SIZE; i++) {
		buf[i] = sector[SECTOR_SIZE_IN_BYTES_OFFSET + i];
	}
	sector_size_in_bytes = ((unsigned short int *) buf)[0];
	printf("Sector size in bytes: %u\n", sector_size_in_bytes);
	
	unsigned int reserved_sectors;
	for (i = 0; i < RESERVED_SECTORS_SIZE; i++) {
		buf[i] = sector[RESERVED_SECTORS_OFFSET + i];
	}
	reserved_sectors = ((unsigned short int *) buf)[0];
	printf("Number of reserved sectors: %u\n", reserved_sectors);
	
	unsigned int sectors_per_fat;
	for (i = 0; i < SECTORS_PER_FAT_SIZE; i++) {
		buf[i] = sector[SECTORS_PER_FAT_OFFSET + i];
	}
	sectors_per_fat = ((unsigned int *) buf)[0];
	printf("Number of sectors per FAT table: %u\n", sectors_per_fat);
	*/
	
}
