
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* how to use this:
 *
 * gcc -o mkinitrd mkinitrd.c && ./mkinitrd -f <file1> "/sbin/init" -f <file2> "/bin/cat"
 *
 * Maximum depth for directories is limited to 2 (f.ex. /usr/bin/...) because initial
 * ramdisk should only contain the necessary files for booting. Thus supporting complex
 * directory hiearchies is unnecessary (for now at least)
 *
 * Example of initrd with following files:
 *      - /sbin/init
 *      - /sbin/dsh
 *      - /bin/echo
 *      - /bin/cat
 *      - /test
 *
 *  /         [dir,  2 items]
 *    sbin/   [dir,  2 items]
 *      init  [file, N bytes]
 *      dsh   [file, N bytes]
 *    bin/    [dir,  2 items]
 *      echo  [file, N bytes]
 *      cat   [file, N bytes]
 *
 *
 * Right now this program is NOT very flexible so give the parameters in correct order. So for example
 * if you wanted to create folder structure defined above (and you have directory called 'programs')
 * you'd call mkinitrd like this:
 *
 * ./mkinitrd -f programs/file1.bin "/sbin/init" -f programs/file2.bin "/sbin/sh"
 *            -f programs/file3.bin "/bin/echo"  -f programs/file4.bin "/bin/cat"
 *
 *  Remember that all binary files must be compiled with a cross-compiler and linked against
 *  libc/libk.a instead of the default libc provided by the compiler */

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

uint32_t alloc_inode(void)
{
    static int inode = 1;
    return inode++;
}

void parse_path(char *path, char *dir, char **fname)
{
    *fname = strdup(strrchr(path, '/') + 1);

    char *path_ptr = path + 1;
    char *dir_ptr  = dir;

    while (*path_ptr != '/')
        *dir_ptr++ = *path_ptr++;
    *dir_ptr = '\0';

#ifdef __DEBUG__
    printf("bin name: %s\n", *fname);
    printf("dir name: %s\n", dir);
#endif
}

int main(int argc, char **argv)
{
    if (argc == 1)
        return -1;

    FILE *disk_fp = fopen("initrd.bin", "w"), *tmp;
    char *fname;
    int c, dir_count = 0, dir_ptr = 0;
    uint32_t cur_offset = 0;

    disk_header_t disk_header = {
        .size    = 0,
        .num_dir = 0,
        .magic   = HEADER_MAGIC
    };

    memset(disk_header.dir_offsets, 0, sizeof(disk_header.dir_offsets));

    dir_header_t root = {
        .name      = "/",
        .num_files = 0,
        .size      = 0,
        .inode     = alloc_inode(),
        .magic     = DIR_MAGIC
    };

    memset(root.file_offsets, 0, sizeof(root.file_offsets));

    char prev_dir[NAME_MAXLEN], cur_dir[NAME_MAXLEN];

    memset(prev_dir, 0, NAME_MAXLEN);
    memset(cur_dir,  0, NAME_MAXLEN);

    /* MAX_FILES and MAX_DIRS can be adjusted according to
     * current needs. If you decide to change them, remember
     * to update them in kernel/fs/initrd.c too */
    struct {
        dir_header_t dir;

        size_t file_ptr;
        size_t file_count;

        struct {
            file_header_t file;
            const char *file_path;
        } files[MAX_FILES];
    } data[MAX_DIRS];

    memset(&data, 0, sizeof(data));

    while ((c = getopt(argc, argv, "f:")) != -1) {
        if (c == 'f') {
            parse_path(argv[optind], cur_dir, &fname);

            if (strcmp(cur_dir, prev_dir)) {

                /* check if dir already exists */
                for (int i = 0; i < MAX_DIRS; ++i) {
                    if (!strcmp(cur_dir, data[i].dir.name)) {
#ifdef __DEBUG__
                        printf("dir already exists\n");
#endif
                        dir_ptr = i;
                        goto save_dir_data;
                    }
                }

                if (dir_count == MAX_DIRS) {
                    printf("maximum number of directories reached!\n");
                    printf("aborting...");
                    return -1;
                }

#ifdef __DEBUG__
                printf("dir %s does not exist!\n", cur_dir);
#endif

                dir_ptr = dir_count++;
                data[dir_ptr].dir.inode = alloc_inode();
                data[dir_ptr].dir.magic = DIR_MAGIC;
                data[dir_ptr].dir.num_files = 0;
                strncpy(data[dir_ptr].dir.name, cur_dir, NAME_MAXLEN);
            }

save_dir_data:
            tmp = fopen(argv[optind - 1], "r");
            uint32_t nwritten = get_file_size(tmp);
            fclose(tmp);

#ifdef __DEBUG__
            printf("save file %s (%u) to dir %s %s\n", fname, nwritten,
                    data[dir_ptr].dir.name, argv[optind - 1]);
#endif

            data[dir_ptr].dir.size += nwritten + sizeof(file_header_t);
            data[dir_ptr].dir.num_files++;

            size_t fcount = data[dir_ptr].file_count,
                   fptr   = data[dir_ptr].file_ptr;

            for (int i = 0; i < MAX_FILES; ++i) {
                if (strncmp(fname, data[dir_ptr].files[i].file.name, NAME_MAXLEN) == 0) {
                    printf("%s already exists in dir %s\n", fname, data[dir_ptr].dir.name);
                    goto end;
                }
            }

            if (fcount == MAX_FILES) {
                printf("maximum number of files per dir reached!");
                goto end;
            }

            fptr = fcount++;
            data[dir_ptr].files[fptr].file_path  = argv[optind - 1];
            data[dir_ptr].files[fptr].file.size  = nwritten;
            data[dir_ptr].files[fptr].file.inode = alloc_inode();
            data[dir_ptr].files[fptr].file.magic = FILE_MAGIC;

            if (fptr == 0)
                cur_offset = sizeof(dir_header_t);
            else
                cur_offset += sizeof(file_header_t)
                           +  data[dir_ptr].files[fptr - 1].file.size;

            /* for the first file the offset is sizeof(dir_header_t)
             *
             * for all subsequent files it's sizeof(file_header_t) +
             * previous file's offset and its size */
            strncpy(data[dir_ptr].files[fptr].file.name, fname, NAME_MAXLEN);
            data[dir_ptr].dir.file_offsets[fptr] = 0;
            data[dir_ptr].dir.file_offsets[fptr] = cur_offset;

            data[dir_ptr].file_count++;
            data[dir_ptr].file_ptr++;
end:
            strncpy(prev_dir, cur_dir, NAME_MAXLEN);
        }
    }

    /* header must be written after files because
     * its info depends on file data
     *
     * skip sizeof(disk_header_t) bytes from
     * the beginning of disk to save space for header */
    fseek(disk_fp, sizeof(disk_header_t), SEEK_SET);

    /* skip root directory also because we don't know at this point
     * how many directories/files it shall contain.
     *
     * This info is filled out later */
    fseek(disk_fp, sizeof(dir_header_t), SEEK_CUR);

    for (int dir_iter = 0; dir_iter < dir_count; ++dir_iter) {
#ifdef __DEBUG__
        puts("--------");

        for (int i = 0; i < data[dir_iter].dir.num_files; ++i)
            printf("%d. file's offset: %d\n", i, data[dir_iter].dir.file_offsets[i]);

        printf("disk: %zu\n", sizeof(disk_header_t));
        printf("dir:  %zu\n", sizeof(dir_header_t));
        printf("file: %zu\n", sizeof(file_header_t));

        printf("name:       %s\n"
               "item count: %u\n"
               "num bytes:  %u\n"
               "inode:      %u\n"
               "\ndirectory contents:\n",
               data[dir_iter].dir.name, data[dir_iter].dir.num_files,
               data[dir_iter].dir.size, data[dir_iter].dir.inode);

#endif
        fwrite(&data[dir_iter].dir, sizeof(dir_header_t), 1, disk_fp);
        disk_header.size += data[dir_iter].dir.size;

        for (int file_iter = 0; file_iter < data[dir_iter].dir.num_files; ++file_iter) {

            /* disk_header.file_count++; */
            fwrite(&data[dir_iter].files[file_iter].file, sizeof(file_header_t), 1, disk_fp);
            tmp = fopen(data[dir_iter].files[file_iter].file_path, "r");

            while ((c = fgetc(tmp)) != EOF)
                putc(c, disk_fp);

            fclose(tmp);

#if __DEBUG__
            printf("\tpath:   %s\n"
                   "\tname:   %s\n"
                   "\tsize:   %u\n"
                   "\tinode:  %u\n\n",
                data[dir_iter].files[file_iter].file_path,
                data[dir_iter].files[file_iter].file.name,
                data[dir_iter].files[file_iter].file.size,
                data[dir_iter].files[file_iter].file.inode,
                data[dir_iter].files[file_iter].file.magic);
#endif
        }
    }

    root.num_files = dir_count;
    root.size      = sizeof(dir_header_t) * dir_count;

    root.file_offsets[0] = sizeof(dir_header_t);

    for (size_t i = 1; i < dir_count; ++i) {
        root.file_offsets[i] =
            root.file_offsets[i - 1]
            + sizeof(dir_header_t)
            + data[dir_ptr - 1].dir.size;
    }

#if __DEBUG__
    printf("root dir offsets:\n");
    for (size_t i = 0; i < dir_count; ++i)
        printf("\t%d. dir offset: %u\n", i + 1, root.file_offsets[i]);
#endif

    fseek(disk_fp, 0L, SEEK_SET);
    fwrite(&disk_header, sizeof(disk_header_t), 1, disk_fp);
    fwrite(&root,        sizeof(dir_header_t),  1, disk_fp);

    fclose(disk_fp);
    return 0;
}
