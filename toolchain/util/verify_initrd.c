#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* how to use this:
 *
 * gcc -o verify verify_mkinitrd.c && ./verify <file1> <file2> ... <file4> 
 *
 * This program shall verify that the generated initrd is valid.
 * initrd verification is used only for debugging */

#define HEADER_MAGIC  0x13371338
#define FILE_MAGIC    0xdeadbeef
#define DIR_MAGIC     0xcafebabe

#define NAME_MAXLEN   64
#define MAX_FILES     5
#define MAX_DIRS      5

typedef struct disk_header {
    /* how much is the total size of initrd */
    uint32_t size;

    /* directory magic number (for verification) */
    uint32_t magic;

    /* how many directories initrd has */
    uint32_t num_dir;

    /* directory offsets (relative to disk's address) */
    uint32_t dir_offsets[MAX_DIRS];
} disk_header_t;

typedef struct dir_header {
    char name[NAME_MAXLEN];

    /* how many bytes directory takes in total:
     *  sizeof(dir_header_t) + num_files * sizeof(file_header_t) + file sizes */
    uint32_t size;

    /* how many files the directory has (1 <= num_files <= 5) */
    uint32_t num_files;

    uint32_t inode;
    uint32_t magic;

    uint32_t file_offsets[MAX_FILES];
} dir_header_t;

typedef struct file_header {
    char name[NAME_MAXLEN];

    /* file size in bytes (header size excluded) */
    uint32_t size;

    uint32_t inode;
    uint32_t magic;
} file_header_t;

uint32_t get_file_size(FILE *fp)
{
    uint32_t fsize = 0;

    fseek(fp, 0L, SEEK_END);
    fsize = (uint32_t)ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    return fsize;
}

int main(int argc, char **argv)
{
    FILE *disk_fp = fopen("initrd.bin", "r"), *tmp;

    disk_header_t disk_header;
    dir_header_t  root_header;

    uint8_t  file_count;
    uint32_t disk_size;
    uint32_t magic;

    fread(&disk_header, sizeof(disk_header_t), 1, disk_fp);

    printf("disk_size:  %u\n"
           "num_dir:    %u\n"
           "magic:      0x%x\n", disk_header.size,
           disk_header.num_dir, disk_header.magic);

    puts("\n------------");

    fread(&root_header, sizeof(dir_header_t), 1, disk_fp);

    printf("name:       %s\n"
           "item count: %u\n"
           "num bytes:  %u\n"
           "inode:      %u\n\n",
           root_header.name, root_header.num_files,
           root_header.size, root_header.inode);

    printf("file/dir offsets:\n");
    for (int i = 0; i < root_header.num_files; ++i) {
        printf("\t%d. item offset: %u\n", i + 1, root_header.file_offsets[i]);
    }

    printf("\ndirectory contents:\n");

    dir_header_t dir;
    file_header_t file;

    for (int dir_iter = 0; dir_iter < root_header.num_files; ++dir_iter) {
        fread(&dir, sizeof(dir_header_t), 1, disk_fp);

        printf("\n------------\n"
               "name:       %s\n"
               "item count: %u\n"
               "num bytes:  %u\n"
               "inode:      %u\n",
               dir.name, dir.num_files,
               dir.size, dir.inode);

        printf("\nfile/dir offsets:\n");
        for (int i = 0; i < root_header.num_files; ++i) {
            printf("\t%d. item offset: %u\n", i + 1, dir.file_offsets[i]);
        }

        printf("\ndirectory contents:\n");

        for (int file_iter = 0; file_iter < dir.num_files; ++file_iter) {
            fread(&file, sizeof(file_header_t), 1, disk_fp);
            printf("\tname:  %s\n"
                   "\tsize:  %u\n"
                   "\tinode: %u\n\n",
                file.name, file.size,
                file.inode, file.magic);
            fseek(disk_fp, file.size, SEEK_CUR);
        }
    }

    return 0;
}
