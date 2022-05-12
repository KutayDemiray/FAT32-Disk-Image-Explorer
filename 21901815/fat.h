#include <stdio.h>
#include <string.h> 
#include <fcntl.h>
#include <unistd.h>
#include <linux/msdos_fs.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#define CLUSTER_SIZE 1024 // cluster size in bytes
#define SECTOR_SIZE 512 // sector size in bytes
#define FAT_LENGTH_IN_SECTORS 1016
#define DATA_START_SECTOR 32 + 2 * FAT_LENGTH_IN_SECTORS

#define VOLUME_LABEL_OFFSET 71
#define VOLUME_LABEL_SIZE 11
#define FS_TYPE_OFFSET 82
#define FS_TYPE_SIZE 8
#define NO_OF_SECTORS_IN_DISK_OFFSET 32
#define NO_OF_SECTORS_IN_DISK_SIZE 4
#define SECTOR_SIZE_IN_BYTES_OFFSET 11
#define SECTOR_SIZE_IN_BYTES_SIZE 2
#define RESERVED_SECTORS_OFFSET 14
#define RESERVED_SECTORS_SIZE 2
#define SECTORS_PER_FAT_OFFSET 36
#define SECTORS_PER_FAT_SIZE 4


#define MODE_PRINT_VOLUME_INFO 1
#define MODE_PRINT_SECTOR 2
#define MODE_PRINT_CLUSTER 3
#define MODE_TRAVERSE_DIRECTORY 4
#define MODE_PRINT_FILE_ASCII 5
#define MODE_PRINT_FILE_BYTES 6
#define MODE_LIST_DIRECTORY 7
#define MODE_LIST_FILE_CLUSTER_NOS 8
#define MODE_DIRECTORY_ENTRY 9
#define MODE_PRINT_FAT 10
#define MODE_READ_FILE 11
#define MODE_PRINT_VOLUME_MAP 12
#define MODE_PRINT_HELP 13

typedef struct params {
	char* mode;
	char* diskimage;
	int num1;
	int num2;
	// TODO	
	
} params;

typedef struct subdirectory {
	char name[9];
	unsigned int namelen;
	unsigned int cluster;
} subdirectory;

typedef struct datetime {
	unsigned char second;
	unsigned char minute;
	unsigned char hour;
	unsigned char day;
	unsigned char month;
	unsigned short int year;
} datetime;

void process_args(int argc, char** argv, params* p);

void print_image_info();
void print_sector(unsigned int sector_no);
void print_cluster(unsigned int cluster_no, unsigned int global_offset);
void traverse(unsigned int dir_cluster, char path[120], unsigned int pathlen);
void print_file_content(char *path);
void print_file_bytes(char *path);
void list_dir(char *path);
void read_volume_map(unsigned int dir_cluster, char path[120], unsigned int pathlen, char **map, unsigned int *fat);


unsigned int find_start(unsigned short int start, unsigned short int starthi);

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

// our own implementation of some string operations (which we preferred not to use)

// copies str2 into str while trimming trailing spaces
// returns new length after trim
// str should have at least str2len + 1 char capacity (including null character)
int str_trimcopy(char *str, char* str2, unsigned int str2len) {
	int whitespace = 1;
	int strlen;
	int j;
	for (j = str2len - 1; j >= 0; j--) {
		if (whitespace && !isspace(str2[j])) {
			str[j] = str2[j];
			strlen = j + 1;
			str[strlen] = '\0';
			whitespace = 0;
		}
		else if (!whitespace) {
			str[j] = str2[j];
		}
	}
	//printf("strlen %d\n", strlen);
	return strlen;
}

int str_concat(char *str, unsigned int strlen, char *str2, unsigned int str2len) {
	int i;
	for (i = 0; i < str2len; i++) {
		str[strlen + i] = str2[i];
	}
	
	str[strlen + str2len] = '\0';
	return strlen + str2len;
}

// other utility functions
int path_split(char **tokens, char *path, char *delimiter) {
	int count = 0;
	char *tmp = strtok(path, delimiter);
	while (tmp != NULL) {
		tokens[count++] = tmp;
		tmp = strtok(NULL, delimiter);
	}
	
	return count;
}

void read_fat(unsigned int *arr, unsigned int fat_sectors, unsigned int entries_per_sector); // implemented in fat.c

datetime read_datetime(unsigned short int date, unsigned short int time) {
	datetime dt;
	dt.second = 2 * (char)(time & 0x1F); // 0000 0000 0001 1111
	dt.minute = (char)((time & 0x7E0) >> 5); // 0000 0111 1110 0000
	dt.hour = (char)((time & 0xF800) >> 11); // 1111 1000 0000 0000
	dt.day = (char)(date & 0x1F); // 0000 0000 0001 1111
	dt.month = (char)((date & 0x1E0) >> 5); // 0000 0001 1110 0000
	dt.year = 1980 + ((date & 0xFE00) >> 9); // 1111 1110 0000 0000
	return dt;
}

void traverse_in_fat(char *path, unsigned int first, char **map, unsigned int *fat);
