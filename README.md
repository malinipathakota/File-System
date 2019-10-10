## Report
Malini Pathakota <br />
Karishma Tangella <br />

## Introduction
In this project we implemented a FAT-based filesystem software stack called 
ECS150-FS. This included implementing a variety of functions which worked 
with files. 

## Design
This file system is based on a File Allocation Table and supports up to 128 
files in a single root directory. It is implemented on top of a virtual disk,
which is split into blocks. We were provided a Block API that was essential
for our implementation. 

## Implementation

***Creating the filesystem layer***
The first step in creating our filesystem was to create the file-system layer. 
This consists of creating a superblock, a root directory, and our FAT. We used 
structs for superblock, file descriptors, and files or entries. We created an 
array for the FAT, and an array of structs for the root directory which 
consists of 128 entries. Our Block size was 4096 bytes.

Our functions consist of:

1. Mount<br/>
Our Mount function allows the file system to be ready for use by mounting the 
file system. This function opens the virtual disk using the Block API given.It
also initializes our superblock, root directory and our FAT blocks. It loads 
the block information from the virtual disk by using a function block_read(); 
which is provided in the Block API.


2. Unmount<br/>
Unmount unmounts the file system. It makes sure the file system is properly 
closed and writes all the datat to disk and then cleans the internal data 
structures of this sytem, which as mentioned above include the superblock,
root directory and FAT. 

3. Info<br/>
This function displays information about the file system including the 
datablocks, root entries, fat blocks etc. 

4. Create<br/>
This function creates a new and empty file which is then placed in the root 
directory. We find an empty entry in the root directory, and then add the 
file including its information, such as size, and the index of the first 
data block that the file will start from.   

5. Delete<br/>
Our Delete function deletes the file from the root directory of the mounted
file. It will delete the file, only if the file is not open, the filename
is not null, invalid or out of bounds. After deleting the file from the
root directory, we clear the file from the FAT also.
6. Ls<br/>
Ls lists all the files on the file system, along with the information about
 the file such as the size the data block it starts from.

7. Open<br/>
This function opens a file for reading and writing, and then returns its file
descriptor. We implemented this function by adding files to an a array of file
descriptors we created. This array holds a maximum of 32 file descriptors. 
We populate the array by adding a file only if the file descriptor is empty.
Each file descriptor holds the name of the file, the offset along with the
file descriptor number.

8. Close<br/>
This function closes the file descriptor only if the file descriptor number 
is not invalid or out of bounds. It resets the file descriptor for the given
file. It resets is file descriptor number to -1, and the offset to 0.

9. Stat<br/>
This function gets the size of the given file and returns it.
10. Lseek<br/>
This function gets the offset of a given file and returns it.

11. Write<br/>
This function writes the bytes of data into the file referenced by the file
descriptor. First we utilized a loop in order to find the appropriate file
referenced by the file descriptor and then used another loop in order to 
find the index of the first empty block. Then calculated how many bytes we 
had left to write and used this value for a for loop where we set values in 
the fat array and recalculated how many bytes we have already written and 
have left to write on every iteration.

12. Read<br/>
This function is supposed to read a file into a given data buffer. The number 
of bytes read can be smaller or equal to the count, which is the number of 
bytes that should be read. We were not able to finish this function on time, 
but our idea was to use a separate buffer to read the data from one block at
a time, and copy the data into our buffer based on the bytes that are 
allowed. We would return the size of the read bytes as well.

## Testing
We tested using test_fs.c and the testing script provided. To test our
functions, we tested manually using ./fs_make to create virtual disks
of many sizes. We then called out the different functions to see if
they worked. This included calling out all the functions in different 
orders to see if they affected each other, or if the functions are not
using the global variables properly. We found that some functions
affected each other negatively, but after testing manually like 
this, we were able to fix any problems that arised. 

