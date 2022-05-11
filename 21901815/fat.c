#include "fat.h"

int fd;
unsigned char sector[SECTOR_SIZE];
unsigned char cluster[CLUSTER_SIZE];

int main(int argc, char** argv) {
	params p;
	p.diskimage = malloc(100 * sizeof(char));
	p.mode = malloc(2 * sizeof(char));
	//unsigned char sector[SECTOR_SIZE];
	//unsigned char cluster[CLUSTER_SIZE];
	str_trimcopy(p.diskimage, argv[1], strlen(argv[1]));
	str_trimcopy(p.mode, argv[2], strlen(argv[2]));
	fd = open(p.diskimage, O_SYNC | O_RDONLY); // disk fd
	
	if (strcmp(p.mode, "-v") == 0) {
		print_image_info();
	}
	else if (strcmp(p.mode, "-s") == 0) {
		print_sector(atoi(argv[3]));
	}
	else if (strcmp(p.mode, "-c") == 0) {
		print_cluster(atoi(argv[3]), 0);
	}
	else if (strcmp(p.mode, "-t") == 0) {
		readsector(fd, sector, 0);
		struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
		traverse(bp->fat32.root_cluster, "/", 1);
	}
	else if(strcmp(p.mode, "-a") == 0) {
		print_file_bytes(argv[3]);
	}
	
	// always check the return values
	//int i;
	
	free(p.diskimage);
	free(p.mode);
	close(fd);
	return 0;
}

// fat DISKIMAGE -v
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
	
	unsigned int cluster_count = (bp->total_sect - bp->fats * bp->fat32.length - bp->reserved) / bp->sec_per_clus;
	printf("Number of clusters: %u\n", cluster_count);
	
	printf("Data region starts at sector: %u\n", bp->reserved + bp->fats * bp->fat32.length);
	
	printf("Root directory starts at sector: %u\n", bp->reserved + bp->fats * bp->fat32.length + bp->sec_per_clus * (bp->fat32.root_cluster - 2));
	
	printf("Root directory starts at cluster: %u\n", bp->fat32.root_cluster);
	
	printf("Disk size in bytes: %u bytes\n", ((unsigned short int *) bp->sector_size)[0] * bp->total_sect);
	
	printf("Disk size in megabytes: %u MB\n", (((unsigned short int *) bp->sector_size)[0] * bp->total_sect) >> 20);
	
	readsector(fd, sector, 1);
	struct fat_boot_fsinfo * ip = (struct fat_boot_fsinfo *) sector;
	printf("Number of used clusters: %u\n", cluster_count - ip->free_clusters);
	
	printf("Number of free clusters: %u\n", ip->free_clusters); 
	
}

// fat DISKIMAGE -s SECTORNUM
void print_sector(unsigned int sector_no) { 
	readsector(fd, sector, sector_no);
	
	unsigned int offset;
	char buf[16];
	for (offset = 0; offset < SECTOR_SIZE; offset += 16) {
		printf("%08x: ", offset);
		int i;
		for (i = 0; i < 16; i++) {
			printf("%02x ", sector[offset + i]);
			buf[i] = sector[offset + i];
		}
		printf("\t");
		for (i = 0; i < 16; i++) {
			if (isprint(buf[i])) {
				printf("%c", buf[i]);
			}
			else {
				printf(".");
			}
		}
		printf("\n");
	}
}

// fat DISKIMAGE -c CLUSTERNUM
void print_cluster(unsigned int cluster_no, unsigned int global_offset) {
	readcluster(fd, cluster, cluster_no);
	
	unsigned int offset;
	char buf[16];
	for (offset = 0; offset < CLUSTER_SIZE; offset += 16) {
		printf("%08x: ", offset + global_offset);
		int i;
		for (i = 0; i < 16; i++) {
			printf("%02x ", cluster[offset + i]);
			buf[i] = cluster[offset + i];
		}
		printf("\t");
		for (i = 0; i < 16; i++) {
			if (isprint(buf[i])) {
				printf("%c", buf[i]);
			}
			else {
				printf(".");
			}
		}
		printf("\n");
	}
}

// fat DISKIMAGE -t
typedef struct subdirectory {
	char name[9];
	unsigned int namelen;
	unsigned int cluster;
} subdirectory;

void traverse(unsigned int dir_cluster, char path[120], unsigned int pathlen) {
	subdirectory subdirs[25];
	unsigned int subdir_count = 0;
	
	readcluster(fd, cluster, dir_cluster);
	
	struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
	
	int i;
	for (i = 0; i < 32; i++) {
		if ((unsigned char) dp->name[0] != (unsigned char) 0x00 && (unsigned char) dp->name[0] != (unsigned char) 0xe5) { // dir entry is used (not uninitialized or deleted)
			char name[9];
			name[8] = '\0';
			int namelen = str_trimcopy(name, (char *) &(dp->name[0]), 8);;

			if ( dp->attr != 0x10 && dp->attr != 0x08) { // file (not root or subdirectory)
				char ext[4];	
				ext[3] = '\0';
				str_trimcopy(ext, (char *) &(dp->name[8]), 3);		
				printf("(f) %s%s.%s\n", path, name, ext);
			}
			else if (dp->attr == (unsigned char) 0x10) { // subdirectory
				// save directory cluster to recursion queue
				unsigned short int tmp[2];
				tmp[1] = dp->starthi;
				tmp[0] = dp->start;
				subdirs[subdir_count].cluster = *((unsigned int *)(tmp));
				subdirs[subdir_count].namelen = str_trimcopy(subdirs[subdir_count].name, name, namelen);
				subdir_count++;
			}
		}
		dp++;
	}

	char nextpath[120];
	str_trimcopy(nextpath, path, pathlen);
	for (i = 0; i < subdir_count; i++) {
		unsigned int nextlen = str_concat(nextpath, pathlen, subdirs[i].name, subdirs[i].namelen);
		nextlen = str_concat(nextpath, nextlen, "/", 1); 
		printf("(d) %s\n", nextpath);
		if (subdirs[i].name[0] != '.') {
			traverse(subdirs[i].cluster, nextpath, nextlen);
		}
	}
}

// ./fat DISKIMAGE -a PATH
void read_fat(unsigned int *arr, unsigned int fat_sectors, unsigned int entries_per_sector) {
	int i;
	int count = 0;
	for (i = 0; i < fat_sectors; i++) {
		readsector(fd, sector, 32 + i);
		unsigned int *cur_sector = (unsigned int *) sector;
		int j;
		for (j = 0; j < entries_per_sector; j++) {
			arr[count++] = cur_sector[j];
		}
	}
}

void print_file_bytes(char* path) {
	char str[120];
	unsigned int fullpathlen = str_trimcopy(str, path, strlen(path)); 
	char **names = malloc(11 * sizeof(char *)); // max directory tree depth is 10 (+ 1 for good measure)
	
	unsigned int namecount = path_split(names, str, "/");
	//printf("namecount: %d\n", namecount);
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
	unsigned int cluster_no = bp->fat32.root_cluster;
	
	// get to the directory containing file
	int i, j, cur = 0;
	char name[13];
	for (i = 0; i < namecount - 1; i++) {
		readcluster(fd, cluster, cluster_no);
		//printf("read %u\n", cluster_no);
		struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
		for (j = 0; j < 32; j++) {
			str_trimcopy(name, dp->name, 8);
			if (strcmp(name, names[cur]) == 0) { // found directory entry with next directory
				//printf("found %s\n", name);
				unsigned short int tmp[2];
				tmp[1] = dp->starthi;
				tmp[0] = dp->start;
				cluster_no = *((unsigned int *)(tmp));
				cur++;
				//printf("next to find: %s\n", names[cur]);
				break;
			}
			dp++;
		}
	}
	 
	// get to the first cluster of the file
	readcluster(fd, cluster, cluster_no);
	//printf("===\n\n");
	struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
	char ext[4];
	for (j = 0; j < 32; j++) {
		str_trimcopy(name, dp->name, 8);
		str_trimcopy(ext, &(dp->name[8]), 3);
		//printf("ext %s\n", ext);
		str_concat(name, strlen(name), ".", 1);
		str_concat(name, strlen(name), ext, strlen(ext));
		//printf("name %s\n", name);
		if (strcmp(name, names[cur]) == 0) { // found directory entry with next directory
			//printf("found %s\n", name);
			unsigned short int tmp[2];
			tmp[1] = dp->starthi;
			tmp[0] = dp->start;
			cluster_no = *((unsigned int *)(tmp));
			cur++;
			break;
		}
		dp++;
	}
	//printf("file first cluster %u\n", cluster_no);
	// at the start of file
	unsigned int *fat;
	readsector(fd, sector, 0);
	bp = (struct fat_boot_sector *) sector;
	unsigned int fat_size = bp->fat32.length * ((unsigned short int *) bp->sector_size)[0];
	fat = malloc(fat_size);
	read_fat(fat, bp->fat32.length, ((unsigned short int *) bp->sector_size)[0] / 4);
	
	unsigned int global_offset = 0;
	while (cluster_no < (unsigned int) 0x0ffffff7) {
		print_cluster(cluster_no, global_offset);
		cluster_no = fat[cluster_no];
		//printf("cluster %d\n", cluster_no);
		global_offset += CLUSTER_SIZE;
	} 
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
