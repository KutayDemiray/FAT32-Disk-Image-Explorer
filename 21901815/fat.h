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
	int mode;
	char* diskimage;
	int num1;
	int num2;
	// TODO	
	
} params;

void process_args(int argc, char** argv, params* p);

void print_image_info(); 

unsigned int find_start(unsigned short int start, unsigned short int starthi);

