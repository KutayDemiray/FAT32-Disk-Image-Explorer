#include "fat.h"

int fd;
unsigned char sector[SECTOR_SIZE];
unsigned char cluster[CLUSTER_SIZE];

int main(int argc, char** argv) {
	params p;
	p.diskimage = malloc(100 * sizeof(char));
	p.mode = malloc(3 * sizeof(char));
	
	//unsigned char sector[SECTOR_SIZE];
	//unsigned char cluster[CLUSTER_SIZE];
	if (strcmp(argv[1], "-h") == 0) {
		print_help();
		free(p.diskimage);
		free(p.mode);
		return 0;
	}
	str_trimcopy(p.diskimage, argv[1], strlen(argv[1]));
	str_trimcopy(p.mode, argv[2], 2);
	
	fd = open(p.diskimage, O_SYNC | O_RDONLY); // disk fd
	
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
	fat_length_in_sectors = bp->fat32.length;
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
	else if (strcmp(p.mode, "-a") == 0) {
		char path[120];
		int i;
		for (i = 0; i < strlen(argv[3]); i++) {
			path[i] = toupper(argv[3][i]);
		}
		path[++i] = '\0';
		print_file_content(path);
	}
	else if (strcmp(p.mode, "-b") == 0) {
		char path[120];
		int i;
		for (i = 0; i < strlen(argv[3]); i++) {
			path[i] = toupper(argv[3][i]);
		}
		path[++i] = '\0';
		print_file_bytes(path);
	}
	else if (strcmp(p.mode, "-l") == 0) {
		char path[120];
		int i;
		for (i = 0; i < strlen(argv[3]); i++) {
			path[i] = toupper(argv[3][i]);
		}
		path[++i] = '\0';
		list_dir(path); 
	}
	else if (strcmp(p.mode, "-n") == 0) {
		char path[120];
		int i;
		for (i = 0; i < strlen(argv[3]); i++) {
			path[i] = toupper(argv[3][i]);
		}
		path[++i] = '\0';
		print_cluster_indices(path);
	}
	else if (strcmp(p.mode, "-d") == 0){
		char path[120];
		int i;
		for (i = 0; i < strlen(argv[3]); i++) {
			path[i] = toupper(argv[3][i]);
		}
		path[++i] = '\0';
		print_dentry_info(path);
	}
	else if (strcmp(p.mode, "-r") == 0) {
		char path[120];
		int i;
		for (i = 0; i < strlen(argv[3]); i++) {
			path[i] = toupper(argv[3][i]);
		}
		path[++i] = '\0';
		print_file_from_offset(path, atoi(argv[4]), atoi(argv[5]));
	}
	// ...
	else if (strcmp(p.mode, "-m") == 0) {
		readsector(fd, sector, 0);
		struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
		unsigned int fat_size = bp->fat32.length * ((unsigned short int *) bp->sector_size)[0];
		char **map = calloc(fat_size / 4, sizeof(char *));
		unsigned int *fat = malloc(fat_size);
		read_fat(fat, bp->fat32.length, ((unsigned short int *) bp->sector_size)[0] / 4);
		readsector(fd, sector, 0);
		traverse_in_fat("/", bp->fat32.root_cluster, map, fat);
		read_volume_map(bp->fat32.root_cluster, "/", 1, map, fat);
		unsigned int i;
		unsigned int last_entry;
		if (strcmp(argv[3], "-1") == 0) {
			last_entry = fat_size / 4;
		}
		else {
			//printf("num : %d\n", atoi(argv[3]));
			last_entry = atoi(argv[3]);
		}
		
		for (i = 0; i < last_entry; i++) {
			if (i < 2 || i > 0x0ffffff7) {
				printf("%08d: --EOF--\n", i);
			}
			else if (map[i] == 0) {
				printf("%08d: --FREE--\n", i);
			}
			else {
				printf("%08d: %s\n", i, map[i]);
			}
		}
		
		free(fat);
		free(map);
	}
	else if (strcmp(p.mode, "-f")==0){
		print_fat(atoi(argv[3]));
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
	
	char fs_type[9]; 

	str_trimcopy(fs_type, (char *)bp->fat32.fs_type, 8);
	printf("File system type: %s\n", fs_type);
	
	char vol_label[12];
	str_trimcopy(vol_label, (char *)bp->fat32.vol_label, 11);
	printf("Volume label: %s\n", vol_label);
	
	printf("Number of sectors in disk: %u\n", bp->total_sect);  
	
	printf("Sector size in bytes: %u\n", ((unsigned short int *) bp->sector_size)[0]);
	
	printf("Number of reserved sectors: %u\n", bp->reserved);
	
	printf("Number of sectors per FAT table: %u\n", bp->fat32.length);
	
	printf("Number of FAT tables: %u\n", bp->fats);
	
	printf("Number of sectors per cluster: %u\n", bp->sec_per_clus);
	
	unsigned int cluster_count = (bp->fat32.length * SECTOR_SIZE) / 4;
	printf("Number of clusters: %u\n", cluster_count);
	
	printf("Data region starts at sector: %u\n", bp->reserved + bp->fats * bp->fat32.length);
	
	printf("Root directory starts at sector: %u\n", bp->reserved + bp->fats * bp->fat32.length + bp->sec_per_clus * (bp->fat32.root_cluster - 2));
	
	printf("Root directory starts at cluster: %u\n", bp->fat32.root_cluster);
	
	printf("Disk size in bytes: %u bytes\n", ((unsigned short int *) bp->sector_size)[0] * bp->total_sect);
	
	printf("Disk size in megabytes: %u MB\n", (((unsigned short int *) bp->sector_size)[0] * bp->total_sect) >> 20);
	
	unsigned int *fat = malloc(bp->fat32.length * SECTOR_SIZE);
	readsector(fd, sector, 0);
	read_fat(fat, bp->fat32.length, SECTOR_SIZE / 4);
	
	unsigned int i;
	unsigned int used_count = 0;
	
	
	for (i = 0; i < cluster_count; i++) {
		//printf("%u\n", fat[i]);
		if (fat[i] != 0) {
			used_count++;
		}
	}
	free(fat);
	 
	printf("Number of used clusters: %u\n", used_count);
	
	printf("Number of free clusters: %u\n", cluster_count - used_count); 
	
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
void traverse(unsigned int dir_cluster, char path[120], unsigned int pathlen) {
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector; 
	unsigned int dentry_per_clus = ((unsigned short int *) bp->sector_size)[0] * bp->sec_per_clus / 32;
	
	readcluster(fd, cluster, dir_cluster);
	
	struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
	
	unsigned int i;
	for (i = 0; i < dentry_per_clus; i++) {
		if ((unsigned char) dp->name[0] != (unsigned char) 0x00 && (unsigned char) dp->name[0] != (unsigned char) 0xe5) { // dir entry is used (not uninitialized or deleted)
			char name[9];
			name[8] = '\0';
			int namelen = str_trimcopy(name, (char *) &(dp->name[0]), 8);

			if ( dp->attr != 0x10 && dp->attr != 0x08) { // file (not root or subdirectory)
				char ext[4];	
				ext[3] = '\0';
				str_trimcopy(ext, (char *) &(dp->name[8]), 3);		
				printf("(f) %s%s.%s\n", path, name, ext);
			}
			else if ((char) dp->name[0] == '.') {// dentry to self or parent, just print (don't traverse)
				printf("(d) %s%s\n", path, name);
			}
			else if (dp->attr == (unsigned char) 0x10) { // subdirectory
				// save directory cluster to recursion queue
				unsigned short int tmp[2];
				tmp[1] = dp->starthi;
				tmp[0] = dp->start;
				unsigned int nextclus = *((unsigned int *)(tmp));
				char nextpath[120];
				str_trimcopy(nextpath, path, pathlen);
				unsigned int nextlen = str_concat(nextpath, pathlen, name, namelen);
				nextlen = str_concat(nextpath, nextlen, "/", 1);
				printf("(d) %s\n", nextpath);
				traverse(nextclus, nextpath, nextlen);
				readcluster(fd, cluster, dir_cluster);
			}
		}
		dp++;
	}

}

void read_fat(unsigned int *arr, unsigned int fat_sectors, unsigned int entries_per_sector) {
	int i;
	int count = 0;
	for (i = 0; i < fat_sectors; i++) {
		readsector(fd, sector, 32 + i); // fat starts at sector 32
		unsigned int *cur_sector = (unsigned int *) sector;
		int j;
		for (j = 0; j < entries_per_sector; j++) {
			arr[count++] = cur_sector[j];
		}
	}
}

// ./fat DISKIMAGE -a PATH
void print_file_content(char *path) {
	char str[120];
	unsigned int pathlen = strlen(path);
	str_trimcopy(str, path, pathlen); 
	char **names = malloc(11 * sizeof(char *)); // max directory tree depth is 10 (+ 1 for good measure)
	
	unsigned int namecount = path_split(names, str, "/");
	//printf("namecount: %d\n", namecount);
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
	unsigned int cluster_no = bp->fat32.root_cluster;
	unsigned int dentry_per_clus = ((unsigned short int *) bp->sector_size)[0]  * bp->sec_per_clus / 32;
	
	// get to the directory containing file
	int i, j, cur = 0;
	char name[13];
	for (i = 0; i < namecount - 1; i++) {
		readcluster(fd, cluster, cluster_no);
		//printf("read %u\n", cluster_no);
		struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
		for (j = 0; j < dentry_per_clus; j++) {
			str_trimcopy(name, (char *) dp->name, 8);
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
	// printf("===\n\n");
	struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
	char ext[4];
	for (j = 0; j < dentry_per_clus; j++) {
		unsigned int namelen = str_trimcopy(name, (char *)dp->name, 8);
		unsigned int extlen = str_trimcopy(ext, (char *) (&(dp->name[8])), 3);
		//printf("ext %s\n", ext);
		namelen = str_concat(name, namelen, ".", 1);
		namelen = str_concat(name, strlen(name), ext, strlen(ext));
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
	
	while (cluster_no < (unsigned int) 0x0ffffff7) { // indices smaller than 0x0ffffff7 are valid (not EOF or reserved)
		readcluster(fd, cluster, cluster_no);
		for (i = 0; i < CLUSTER_SIZE; i++) {
			printf("%c", cluster[i]);
		}
		cluster_no = fat[cluster_no];
		//printf("cluster %d\n", cluster_no);
	}
	
	free(names);
	free(fat);
}

// ./fat DISKIMAGE -b PATH
void print_file_bytes(char* path) {
	char str[120];
	str_trimcopy(str, path, strlen(path)); 
	char **names = malloc(11 * sizeof(char *)); // max directory tree depth is 10 (+ 1 for good measure)
	
	unsigned int namecount = path_split(names, str, "/");
	//printf("namecount: %d\n", namecount);
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
	unsigned int cluster_no = bp->fat32.root_cluster;
	unsigned int dentry_per_clus = ((unsigned short int *) bp->sector_size)[0]  * bp->sec_per_clus / 32;
	
	// get to the directory containing file
	int i, j, cur = 0;
	char name[13];
	for (i = 0; i < namecount - 1; i++) {
		readcluster(fd, cluster, cluster_no);
		//printf("read %u\n", cluster_no);
		struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
		for (j = 0; j < dentry_per_clus; j++) {
			str_trimcopy(name, (char *)dp->name, 8);
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
	for (j = 0; j < dentry_per_clus; j++) {
		str_trimcopy(name, (char *) dp->name, 8);
		str_trimcopy(ext, (char *) &(dp->name[8]), 3);
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
	free(names);
	free(fat);
}

// ./fat DISKIMAGE -l PATH
void list_dir(char *path) {
	char str[120];
	str_trimcopy(str, path, strlen(path)); 
	char **names = malloc(11 * sizeof(char *)); // max directory tree depth is 10 (+ 1 for good measure)
	
	unsigned int namecount = path_split(names, str, "/");
	//printf("namecount: %d\n", namecount);
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
	unsigned int cluster_no = bp->fat32.root_cluster;
	unsigned int dentry_per_clus = ((unsigned short int *) bp->sector_size)[0]  * bp->sec_per_clus / 32;
	//printf("%u\n", dentry_per_clus);
	
	// get to the directory containing file
	int i, j, cur = 0;
	char name[12];
	for (i = 0; i < namecount; i++) {
		readcluster(fd, cluster, cluster_no);
		//printf("read %u\n", cluster_no);
		struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
		for (j = 0; j < dentry_per_clus; j++) {
			str_trimcopy(name, (char *)dp->name, 8);
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
	
	// now in target directory, print contents
	readcluster(fd, cluster, cluster_no);
	//print_cluster(cluster_no, 0);
	struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
	for (j = 0; j < dentry_per_clus; j++) {
		//printf("%d\n", j);
		if ((unsigned char) dp->name[0] != (unsigned char) 0x00 && (unsigned char) dp->name[0] != (unsigned char) 0xe5) { // dir entry is 	used (not uninitialized or deleted)
			int namelen = str_trimcopy(name, (char *) &(dp->name[0]), 8);;
			unsigned short int tmp[2];
			tmp[1] = dp->starthi;
			tmp[0] = dp->start;
			unsigned int fcn = *((unsigned int *)(tmp));
			datetime dt = read_datetime(dp->date, dp->time);
			if ( dp->attr != 0x10 && dp->attr != 0x08) { // file (not root or subdirectory)
				char ext[4];	
				int extlen = str_trimcopy(ext, (char *) &(dp->name[8]), 3);
				namelen = str_concat(name, namelen, ".", 1);
				namelen = str_concat(name, namelen, ext, extlen);
				printf("(f) name=%-15sfcn=%-10usize(bytes)=%-10udate=%u-%u-%u %u:%u:%u\n", name, fcn, dp->size, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
			}
			else if (dp->attr == (unsigned char) 0x10) { // subdirectory
				printf("(d) name=%-15sfcn=%-10usize(bytes)=%-10udate=%u-%u-%u %u:%u:%u\n", name, fcn, dp->size, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
			}
		}
		dp++;
	}
	free(names);
}

// ./fat DISKIMAGE -n PATH
void print_cluster_indices(char *path) {
	char str[120];
	str_trimcopy(str, path, strlen(path)); 
	char **names = malloc(11 * sizeof(char *)); // max directory tree depth is 10 (+ 1 for good measure)
	
	unsigned int namecount = path_split(names, str, "/");
	//printf("namecount: %d\n", namecount);
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
	unsigned int cluster_no = bp->fat32.root_cluster;
	unsigned int dentry_per_clus = ((unsigned short int *) bp->sector_size)[0]  * bp->sec_per_clus / 32;
	unsigned int fat_size = bp->fat32.length * ((unsigned short int *) bp->sector_size)[0];
	unsigned int *fat = malloc(fat_size);
	read_fat(fat, bp->fat32.length, ((unsigned short int *) bp->sector_size)[0] / 4);
	
	if (strcmp(path, "/") == 0) {
		unsigned int cindex = 0;
		
		while (cluster_no != 0 && cluster_no < 0x0ffffff7) {
			printf("cindex=%-10u clusternum=%u\n", cindex++, cluster_no);
			cluster_no = fat[cluster_no];
		}
	
		free(fat);
		return;
	}
	
	// get to the directory containing file
	int i, j, cur = 0;
	char name[13];
	for (i = 0; i < namecount - 1; i++) {
		readcluster(fd, cluster, cluster_no);
		//printf("read %u\n", cluster_no);
		struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
		for (j = 0; j < dentry_per_clus; j++) {
			str_trimcopy(name, (char *)dp->name, 8);
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
	char dirname[9];
	for (j = 0; j < dentry_per_clus; j++) {
		str_trimcopy(name, (char *) dp->name, 8);
		str_trimcopy(dirname, (char *) dp->name, 8);
		str_trimcopy(ext, (char *) &(dp->name[8]), 3);
		//printf("ext %s\n", ext);
		str_concat(name, strlen(name), ".", 1);
		str_concat(name, strlen(name), ext, strlen(ext));
		//printf("name %s\n", name);

		if ((dp->attr == (unsigned char) 0x10 && strcmp(dirname, names[cur]) == 0) || (dp->attr != (unsigned char) 0x10 && strcmp(name, names[cur]) == 0)) { // found directory entry
			//printf("found %s\n", name);
			unsigned short int tmp[2];
			tmp[1] = dp->starthi;
			tmp[0] = dp->start;
			cluster_no = *((unsigned int *)(tmp));
			//printf("set cno=%u\n", cluster_no);
			cur++;
			break;
		}
		dp++;
		if (j == 31) {
			cluster_no = 0;
		}
	}
	
	
	readsector(fd, sector, 0);
	//printf("cno: %u, root: %u\n", cluster_no, (unsigned short int) bp->fat32.root_cluster);
	if (cluster_no != 0 && cluster_no != (unsigned short int) bp->fat32.root_cluster) { // root should be taken care of already
		unsigned int cindex = 0;
		
		while (cluster_no != 0 && cluster_no < 0x0ffffff7) {
			printf("cindex=%-10u clusternum=%u\n", cindex++, cluster_no);
			cluster_no = fat[cluster_no];
		}
	}
	free(fat);
	free(names);
}

// 9 10 11



// 13) fat -h
void print_help(){
	printf("Commands:\n");
	printf("./fat <diskimage> -v: Prints volume info\n");
	printf("./fat <diskimage> -s <sectorno>: Prints given sector\n");
	printf("./fat <diskimage> -c <clusterno>: Prints given cluster\n");
	printf("./fat <diskimage> -t: Starting from root, traverses the directory tree (in DFT order)\n");
	printf("./fat <diskimage> -a <path>: Prints contents of given ASCII text file\n");
	printf("./fat <diskimage> -b <path>: Prints byte sequence of given file\n");
	printf("./fat <diskimage> -l <path>: Prints the names of files and subdirectories in given directory\n");
	printf("./fat <diskimage> -n <path>: Prints the cluster numbers storing the contents of given file or directory\n");
	printf("./fat <diskimage> -d <path>: Prints the directory entry information (name, type etc.) of the given file\n");
	printf("./fat <diskimage> -f <count>: Prints the contents of FAT table, starting from the beginning up to the given number of entries\n");
	printf("./fat <diskimage> -r <path> <offset> <count>: Starting from given offset, prints specified amount of bytes in given file\n");
	printf("./fat <diskimage> -m <count>: Prints specified number of rows of a map of the volume\n");
	printf("./fat -h: Prints this help information\n");
	printf("Examples:\n");
	printf("./fat disk1 -v\n");
	printf("./fat disk1 -s 32\n");
	printf("./fat disk1 -c 2\n");
	printf("./fat disk1 -r /DIR2/F1.TXT 100 64\n");
	printf("./fat disk1 -b /DIR2/F1.TXT\n");
	printf("./fat disk1 -a /DIR2/F1.TXT\n");
	printf("./fat disk1 -n /DIR1/AFILE1.BIN\n");
	printf("./fat disk1 -m 100\n");
	printf("./fat disk1 -f 50\n");
	printf("./fat disk1 -l /\n");
	printf("./fat disk1 -l /DIR2\n");
	printf("./fat disk1 -h\n");
}

// 9) ./fat DISKIMAGE -d PATH
void print_dentry_info(char* path) {
	char str[120];
	//printf("strlen(path): %u\n", strlen(path));
	str_trimcopy(str, path, strlen(path)); 
	char **tokens = malloc(11 * sizeof(char *)); // max directory tree depth is 10 (+ 1 for good measure)
	
	unsigned int token_cnt = path_split(tokens, str, "/");
	
	// fetch the directery entry count
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
	unsigned int cluster_no = bp->fat32.root_cluster;
	char name[12];
	unsigned short int tmp[2];
	struct msdos_dir_entry *dp;
	unsigned int dentry_per_clus = ((unsigned short int *) bp->sector_size)[0]  * bp->sec_per_clus / 32;
	int current_loc = 0;
	char ext[4];
	int i;
	readcluster(fd, cluster, cluster_no);
	dp = (struct msdos_dir_entry *) cluster;
	while (current_loc < token_cnt){
		int i;
		readcluster(fd, cluster, cluster_no);
		dp = (struct msdos_dir_entry *) cluster;
		for (i = 0; i<dentry_per_clus; i++){
			//name[8] = '\0';
			str_trimcopy(name, (char *) &(dp->name[0]), 8);
			
			/*
			if (strcmp(ext, "")!=0){
				str_concat(name, strlen(name), ".", 1);
				str_concat(name, strlen(name), ext, strlen(ext));
			}
			*/
			if (dp->attr != 0x08 && dp->attr != 0x10) {
				str_trimcopy(ext, (char*)&(dp->name[8]), 3);
				str_concat(name, strlen(name), ".", 1);
				str_concat(name, strlen(name), ext, strlen(ext));
			}
			//printf("name %s tok %s\n", name, tokens[current_loc]);
			if (strcmp(tokens[current_loc], name)==0){
				current_loc++;
				tmp[1] = dp->starthi;
				tmp[0] = dp->start;
				cluster_no = *((unsigned int *)(tmp));
				break;
			}
			dp++;
		}
		//if (current_loc++ < token_cnt) {
			
		//}
		
	}
	
	if (current_loc == 0){
		str_trimcopy(name, (char *) &(dp->name[0]), 8);
	}
	printf("name         = %s\n", name);
	//printf("attr         = %u\n", dp->attr);
	
	if (dp-> attr == 0x08 || dp->attr == 0x10)
		printf("type         = DIRECTORY\n");
	else if ((dp->attr >> 3)<<7 != 0x01 )
		printf("type         = FILE\n");
	
	printf("firstcluster = %u\n", cluster_no);
	unsigned int ccount = 0;
	/*
	if (dp->size %1024 == 0)
		ccount = dp->size /1024;
	else 
		ccount = dp->size/1024 + 1;
	*/
	readsector(fd, sector, 0);
	unsigned int *fat = malloc(bp->fat32.length * SECTOR_SIZE);
	read_fat(fat, bp->fat32.length, ((unsigned short int *) bp->sector_size)[0] / 4);
	readsector(fd, sector, 0);
	while (cluster_no != 0 && cluster_no < 0x0ffffff7) {
		//printf("%u %u\n", cluster_no, fat[cluster_no]);
		ccount++;
		cluster_no = fat[cluster_no];
	} 	
	
	
	printf("clustercount = %u\n", ccount);
	printf("size(bytes)  = %u\n", dp->size);
	datetime date = read_datetime(dp->date, dp->time);
	printf("date         = %02u-%02u-%u\n", date.day, date.month, date.year);
	printf("time         = %02u:%02u:%02u\n",date.hour ,date.minute, date.second );
	free(fat);
	free(tokens);
}

// 10) ./fat DISKIMAGE -f COUNT
void print_fat(int count){
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector; 
	unsigned int sector_entries = ((unsigned short int *) bp->sector_size)[0] / 4;
	int sector_count;
	if (count == -1){
		count = bp->fat32.length * sector_entries;
		sector_count = bp->fat32.length;
	}	
	else if (count % 32 == 0)
		sector_count = (count/32);
	else 
		sector_count = (count/32)+1;
	
	// read fat and then print
	unsigned int* arr = malloc (sector_count * ((unsigned short int *) bp->sector_size)[0]);
	read_fat(arr, sector_count, sector_entries);
	//print the data
	int i;
	for (i = 0; i<count; i++){
		if (arr[i] <(unsigned int) 0x0ffffff7 )
			printf("%07u: %u\n", i, arr[i]);
		else
			printf("%07u: EOF\n",i);
	}
	free(arr);
}

// ./fat DISKIMAGE -r PATH OFFSET COUNT
void print_file_from_offset(char *path, int offset, int count) {
	char str[120];
	str_trimcopy(str, path, strlen(path)); 
	char **names = malloc(11 * sizeof(char *)); // max directory tree depth is 10 (+ 1 for good measure)
	
	unsigned int namecount = path_split(names, str, "/");
	//printf("namecount: %d\n", namecount);
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector;
	unsigned int cluster_no = bp->fat32.root_cluster;
	unsigned int dentry_per_clus = ((unsigned short int *) bp->sector_size)[0]  * bp->sec_per_clus / 32;
	
	// get to the directory containing file
	int i, j, cur = 0;
	char name[13];
	for (i = 0; i < namecount - 1; i++) {
		readcluster(fd, cluster, cluster_no);
		//printf("read %u\n", cluster_no);
		struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
		for (j = 0; j < dentry_per_clus; j++) {
			str_trimcopy(name, (char *)dp->name, 8);
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
	for (j = 0; j < dentry_per_clus; j++) {
		str_trimcopy(name, (char *) dp->name, 8);
		str_trimcopy(ext, (char *) &(dp->name[8]), 3);
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
	
	readcluster(fd, cluster, cluster_no);
	//printf("offset: %d, count: %d\n", offset, count);
	char buf[16];
	readsector(fd, sector, 0);
	unsigned int cluster_size = bp->sec_per_clus * *((unsigned int *) bp->sector_size);
	int row_index = 0;
	int row_len = ((count - i) < 16) ? count - i : 16;
	for (i = 0; i < count; i++) {
		if (i % 16 == 0) {
			printf("%08x: ", i + offset);
			row_len = ((count - i) < 16) ? count - i : 16;
			//printf("row len: %d :  ", row_len);
		}
		//printf("%: %d\n", (i + offset) % cluster_size );
		if ((i + offset) % cluster_size == 0) {
			cluster_no = fat[cluster_no];
			readcluster(fd, cluster, cluster_no);
		}
		
		buf[row_index] = cluster[(i + offset) % CLUSTER_SIZE]; 
		printf("%02x ", buf[row_index++]);
		
		if (row_index == row_len) {
			for (; row_index < 16; row_index++) {
				printf("   ");
			}
		}
		
		if (row_index == 16) {
			for (j = 0; j < row_len; j++) {
				if (isprint(buf[j])) {
					printf("%c", buf[j]);
				}
				else {
					printf(".");
				}
			}
			row_index = 0;
			printf("\n");
		}
	}

	free(names);
	free(fat);
}

// ./fat DISKIMAGE -m COUNT
void traverse_in_fat(char *path, unsigned int first, char **map, unsigned int *fat) {
	//printf("traverse_in_fat() before %s\n", path);
	unsigned int clus = first;
	while (clus != 0 && clus < 0x0ffffff7) {
		map[clus] = malloc(sizeof(char) * 120);
		str_trimcopy(map[clus], path, strlen(path));
		//printf("traverse_in_fat() %s in %d: %s\n", path, clus, map[clus]);
		clus = fat[clus];
		
	}
}


void read_volume_map(unsigned int dir_cluster, char path[120], unsigned int pathlen, char **map, unsigned int *fat) {
	//printf("read_volume_map()\n");
	readsector(fd, sector, 0);
	struct fat_boot_sector *bp = (struct fat_boot_sector *) sector; 
	unsigned int dentry_per_clus = ((unsigned short int *) bp->sector_size)[0] * bp->sec_per_clus / 32;

	readcluster(fd, cluster, dir_cluster);
	struct msdos_dir_entry *dp = (struct msdos_dir_entry *) cluster;
	unsigned int i;
	for (i = 0; i < dentry_per_clus; i++) {
		
		if ((unsigned char) dp->name[0] != (unsigned char) 0x00 && (unsigned char) dp->name[0] != (unsigned char) 0xe5) { // dir entry is used (not uninitialized or deleted)
			char name[9];
			int namelen = str_trimcopy(name, (char *) &(dp->name[0]), 8);;
			if ( dp->attr != 0x10 && dp->attr != 0x08) { // file (not root or subdirectory)
				char ext[4];	
				str_trimcopy(ext, (char *) &(dp->name[8]), 3);		
				//printf("(f) %s%s.%s\n", path, name, ext);
				char fpath[120];
				str_trimcopy(fpath, path, pathlen);
				unsigned int nextlen = str_concat(fpath, pathlen, name, strlen(name));
				nextlen = str_concat(fpath, nextlen, ".", 1);
				nextlen = str_concat(fpath, nextlen, ext, strlen(ext));
				unsigned short int tmp[2];
				tmp[1] = dp->starthi;
				tmp[0] = dp->start;
				unsigned int nextclus = *((unsigned int *)(tmp));
				traverse_in_fat(fpath, nextclus, map, fat);
			}
			else if ((char) dp->name[0] == '.') {// dentry to self or parent, just print (don't traverse)
				//printf("(d) %s%s\n", path, name);
			}
			else if (dp->attr == (unsigned char) 0x10) { // subdirectory
				// save directory cluster to recursion queue
				unsigned short int tmp[2];
				tmp[1] = dp->starthi;
				tmp[0] = dp->start;
				unsigned int nextclus = *((unsigned int *)(tmp));
				char nextpath[120];
				str_trimcopy(nextpath, path, pathlen);
				unsigned int nextlen = str_concat(nextpath, pathlen, name, namelen);
				nextlen = str_concat(nextpath, nextlen, "/", 1);
				//printf("(d) %s\n", nextpath);
				traverse_in_fat(nextpath, nextclus, map, fat);
				read_volume_map(nextclus, nextpath, nextlen, map, fat);
				readcluster(fd, cluster, dir_cluster);
			}
		}
		dp++;
	}
	

}

/*
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
*/
