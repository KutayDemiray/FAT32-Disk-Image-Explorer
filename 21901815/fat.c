#include <stdioh.>

void process_args(int argc, char** argv, int* num, char** path);


typedef struct {
	int mode;
	char* diskimage;
	int num1;
	int num2;
	
	
} params;
int main(int argc, char** argv) {
	int mode;
	int num;
	char* diskpath;
	char* path;	
	process_args(argc, argv, &num, &path);
	
	return 0;
}

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
void process_args(int argc, char** argv, char** diskpath, int* num, char** path) {
	*diskpath = argv[1];
	
}

