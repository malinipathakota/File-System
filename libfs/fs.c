#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "disk.h"
#include "fs.h"

#define BLOCK_SIZE 4096
#define FAT_EOC 0xFFFF

struct  __attribute__((__packed__)) superblock
{
	char signature[8];
	uint16_t block_amount_virtual_disk;
	uint16_t root_directory_block_index;
	uint16_t data_block_start_index;
	uint16_t amount_data_blocks;
	uint8_t fat_blocks;
	uint8_t padding[BLOCK_SIZE];
};

struct  __attribute__((__packed__)) root_entry
{
	char filename[16];
	uint32_t size_of_file;
	uint16_t index_first_data_block;
	uint8_t padding[10];
};

struct __attribute__((__packed__)) file_descriptor
{
	char filename[16];
	int fd;
	uint16_t offset;
};

typedef struct superblock superblock_t;
typedef struct root_entry root_t;
typedef struct file_descriptor fd_t; 
root_t root_array[FS_FILE_MAX_COUNT];
fd_t filed[FS_OPEN_MAX_COUNT];
uint16_t *fat_array;
typedef uint8_t *blocks;
superblock_t super_block;
bool opened = false;

//-----------START OF FUNCTIONS------------------------------------------
/**
 * fs_mount - Mount a file system
 * Open the virtual disk file @diskname and mount the file system that it
 * contains. 
 */
int fs_mount(const char *diskname)
{
	int open = block_disk_open(diskname);
	if(open == -1)
	{
		return -1;
	}
	opened = true;
	//populates super_block with contents 
	int read = block_read(0, (void*)&super_block);
	if(read == -1)
	{
		return -1;
	}
	
	int number_of_disk_blocks = block_disk_count();
	if(super_block.block_amount_virtual_disk != number_of_disk_blocks)
	{
		return -1;
	}
	//error case for signature
	int signature_failure = memcmp(super_block.signature, "ECS150FS", 8);
	if(signature_failure != 0)
	{
		return -1;
	}

	int fatblocks = super_block.fat_blocks;
	fat_array = malloc(fatblocks*BLOCK_SIZE*sizeof(uint16_t));
	//populates fat_array
	{	for(int i = 0; i < fatblocks; i++)
		{
			if(block_read(i + 1, &fat_array[i*(BLOCK_SIZE/2)]) == -1)
			{
				return -1;
			}
		}
		
	}
	int root = block_read(super_block.root_directory_block_index, &root_array);
	if(root == -1)
	{
		return -1;
	}
	return 0;
}

/**
 * fs_umount - Unmount file system
 * Unmount the currently mounted file system and close the underlying virtual
 * disk file.
 */
int fs_umount(void)
{	
	//writing to virtual disk
	if(opened == false)
	{
		return -1;
	}
	int super_write_fail = block_write(0, (void*)&super_block);
	if(super_write_fail == -1)
	{
		return -1;
	}

	int root_failure = block_write(super_block.root_directory_block_index, &root_array);
	if(root_failure == -1)
	{
		return -1;
	}
	//writing to virtual disk
	for(int i = 0; i < super_block.fat_blocks; i++)
	{	
		int fat_failure = block_write(i + 1, &fat_array[i*(BLOCK_SIZE/2)]);
		if(fat_failure == -1){
			return -1;
		}
	}
	//finished writing everything to disk, clear superblock
	memset(super_block.signature, '\0', 8);
	super_block.block_amount_virtual_disk = 0;
	super_block.root_directory_block_index = 0;
	super_block.data_block_start_index = 0;
	super_block.amount_data_blocks = 0;
	super_block.fat_blocks = 0;
	
	free(fat_array);
	//clearing the root_array
	for (int i = 0; i < 128; i++) 
	{
		memset(root_array[i].filename,0,strlen(root_array[i].filename));
		root_array[i].size_of_file = 0;
		root_array[i].index_first_data_block = 0;
	} 

	
	int block_close = block_disk_close();
	if(block_close == -1)
	{
		return -1;
	}
	return 0;
}

/**
 * fs_info - Display information about file system
 * Display some information about the currently mounted file system.
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
int fs_info(void)
{
	if(opened == false)
	{
		return -1;
	}

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", super_block.block_amount_virtual_disk);
	printf("fat_blk_count=%d\n", super_block.fat_blocks);
	printf("rdir_blk=%d\n", super_block.root_directory_block_index);
	printf("data_blk=%d\n", super_block.data_block_start_index);
	printf("data_blk_count=%d\n", super_block.amount_data_blocks);

	int fat_counter = 0;
	for(int i = 0; i < super_block.amount_data_blocks; i++)
	{
		if(fat_array[i] == 0)
		{
			fat_counter = fat_counter + 1;
		}
	}

  	printf("fat_free_ratio=%d/%d\n", fat_counter, super_block.amount_data_blocks);
  
	int root_counter = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) 
	{
		if(root_array[i].size_of_file == 0)
		{
			root_counter = root_counter + 1;
		}
	} 
	printf("rdir_free_ratio=%d/%d\n", root_counter, FS_FILE_MAX_COUNT);
	return 0;
}

/**
 * fs_create - Create a new file
 * Create a new and empty file named @filename in the root directory of the
 * mounted file system. 
 */
int fs_create(const char *filename)
{
	if(filename == NULL || filename[strlen(filename)] != '\0' || strlen(filename) > FS_FILENAME_LEN) {
		return -1;
	}

	int num = 0;
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if(strlen(root_array[i].filename) != 0) {
			if(strcmp(root_array[i].filename, filename) == 0) {
				return -1;
			}
			else {
				num = num + 1;
			}
		}
		
		if(num == FS_FILE_MAX_COUNT) {
			return -1;
		}
	}

	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if(root_array[i].filename[0] == '\0') {
			strcpy(root_array[i].filename, filename);
			root_array[i].index_first_data_block = FAT_EOC;
			root_array[i].size_of_file = 0;
			block_write(super_block.root_directory_block_index,&root_array);
			return 0;
		}
	}
	return 0;
}

/**
 * fs_delete - Delete a file
 * Delete the file named @filename from the root directory of the mounted file
 * system.
 */
int fs_delete(const char *filename)
{
	int length = strlen(filename);
	if(filename == NULL || filename[strlen(filename)] != '\0' || strlen(filename) > FS_FILENAME_LEN) {
		return -1;
	}
	//checks if there is no file named @filename in root directory to delete
	bool found = false;
	int first_data_block = 0;
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if(strcmp(root_array[i].filename, filename) == 0)
		{
			found = true;
			first_data_block = root_array[i].index_first_data_block;
		}
	}
	if(found == false)
	{
		return -1;
	}
	//checks if file @filename is currently open
	for(int i = 0; i < FS_OPEN_MAX_COUNT; i++)
	{
		if(strcmp(filed[i].filename, filename) == 0)
		{
			if(filed[i].fd != -1)
			{
				return -1;
			}
		}
	}
	//clears file from fat array
	int next_data_block;
	int next = first_data_block;
	while(next_data_block != FAT_EOC)
	{
		next_data_block = fat_array[next];
		fat_array[next] = 0;
		next = next_data_block;
	}
	fat_array[next_data_block] = 0;

	//clears root array
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if(strcmp(root_array[i].filename, filename) == 0) {
			memset(root_array[i].filename, '\0', length);
			root_array[i].size_of_file = 0;
			root_array[i].index_first_data_block = 0;
			return 0;
		}
	}
	return -1;	
}

/**
 * fs_ls - List files on file system
 * List information about the files located in the root directory.
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
int fs_ls(void)
{
	if(opened == false) {
		return -1;
	}
	printf("FS Ls:");
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (root_array[i].filename[0] != '\0') {
			printf("\nfile: %s, size: %d, ", root_array[i].filename, root_array[i].size_of_file);
			printf("data_blk: %d", root_array[i].index_first_data_block);
		}
	}
	printf("\n");
	return 0;
}

/**
 * fs_open - Open a file
 * Open file named @filename for reading and writing, and return the
 * corresponding file descriptor. 
 */
int fs_open(const char *filename)
{
	//checks if there is no file, invalid file length, or all files are currently open
	if(filename == NULL || filename[strlen(filename)] != '\0' || strlen(filename) > FS_FILENAME_LEN || filed[31].fd == 31)
	{
		return -1;
	}
	bool found = false;
	//checks if file exists in root directory
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if(strcmp(root_array[i].filename, filename) == 0)
		{
			found = true;
		}
	}
	if(found == false)
	{
		return -1;
	}

	for(int i = 0; i < FS_OPEN_MAX_COUNT; i++)
	{
		if(strlen(filed[i].filename) == 0)
		{	
			strcpy(filed[i].filename, filename);
			filed[i].fd = i;
			filed[i].offset = 0; 
			return filed[i].fd;
		}
	}
	return -1;
}

/**
 * fs_close - Close a file
 * Close file descriptor @fd.
 */
int fs_close(int fd)
{
	if(fd > 31 || fd < 0 || filed[fd].fd == -1)
	{
		return -1;
	}
	strcpy(filed[fd].filename, "");
	filed[fd].fd = -1;
	filed[fd].offset = 0; 	
	return  0;
}

/**
 * fs_stat - Get file status
 * @fd: File descriptor
 * Get the current size of the file pointed by file descriptor @fd.
 */
int fs_stat(int fd)
{
	if(fd > 31 || fd < 0 || filed[fd].fd == -1)
	{
		return -1;
	}
	char *file = filed[fd].filename;

	for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if(strcmp(root_array[i].filename, file) == 0)
		{
			return root_array[i].size_of_file;
		}
	}
	return -1;
}

/**
 * fs_lseek - Set file offset
 * Set the file offset (used for read and write operations) associated with file
 * descriptor @fd to the argument @offset. 
 */
int fs_lseek(int fd, size_t offset)
{
	if(fd > 31 || fd < 0 || offset > root_array[fd].size_of_file || offset < 0 || filed[fd].fd == -1)
	{
		return -1;
	}
	filed[fd].offset = offset; 
	return 0;
}

//---------HELPER FUNCTIONS--------------
uint16_t find_data_block_index(int fd, uint16_t offset)
{
	uint16_t root_index;
	char* file= filed[fd].filename;
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if(strcmp(root_array[i].filename, file) == 0)
		{
			root_index = root_array[i].index_first_data_block;
			break;
		}
	}
	uint16_t value = root_index + offset;
	return value;
}
//---------END OF HELPER FUNCTIONS-------

int fs_write(int fd, void *buf, size_t count)
{
	size_t dec_bytes = count;
	if(count > BLOCK_SIZE) {
		dec_bytes = BLOCK_SIZE;
	}
	int inc_bytes = 0;
	int num_blocks = 0;
	fd_t file_descrip = filed[fd];
	root_t *write_block = &root_array[0];
	blocks bounce[count];

	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if(strcmp(root_array[i].filename, file_descrip.filename) == 0) {
			write_block = &root_array[i];
		}
	}

	if(write_block->size_of_file == 0) {	
		int find_index = 0;
		// finds the index of the first block that's empty
		for(int i = 0; i < super_block.amount_data_blocks; i++) {
			if(fat_array[i] == 0) {
				find_index = i;
				break;
			}
		}
		write_block->index_first_data_block = find_index;
	}

	int index = write_block->index_first_data_block;
	int total_index = index + super_block.data_block_start_index;
	int dec_count = count;
	
	for(int i = ((BLOCK_SIZE + count - 1) / BLOCK_SIZE); i >= 1; i--) {
		int find_index = 0;
		// finds the index of the first block that's empty
		for(int i = 0; i < super_block.amount_data_blocks; i++) {
			if(fat_array[i] == 0) {
				find_index = i;
				break;
			}
		}

		// sets values in the fat_array and writes to disk
		fat_array[index] = find_index;
		index = fat_array[index];
		fat_array[index] = FAT_EOC;
		int *buff = (void*)bounce + (BLOCK_SIZE * (num_blocks++));
		block_write(total_index, buff); 	
		                   
		dec_count = dec_count - dec_bytes;
		size_t size = dec_count;
		if(dec_count > BLOCK_SIZE) {
			size = BLOCK_SIZE;
		}

	 	// calculates amount of bytes written and amount of bytes left
		inc_bytes = inc_bytes + dec_bytes;
		write_block->size_of_file = write_block->size_of_file + dec_bytes; 
		dec_bytes = size;
	}
	return inc_bytes;
}

/**
 * fs_read - Read from a file
 * Attempt to read @count bytes of data from the file referenced by file
 * descriptor @fd into buffer pointer by @buf. It is assumed that @buf is large
 * enough to hold at least @count bytes.
 */
int fs_read(int fd, void *buf, size_t count)
{
	uint32_t size;
	//get the file
	char* file_to_read = filed[fd].filename;
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if(strcmp(root_array[i].filename, file_to_read) == 0)
		{
			size = root_array[i].size_of_file;
			break;
		}
	}
	return size;
}
