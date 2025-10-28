#include "tar.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>
#include "framebuffer.h"

// A pointer to the in-memory initrd.tar file
static void* initrd_ptr = NULL;

// The USTAR/tar header structure (512 bytes)
typedef struct {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size_str[12];
    char mtime[12];
    char checksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
} tar_header_t;

/**
 * @brief Helper function to convert an octal string to an unsigned integer.
 */
static uint64_t octal_to_uint(const char* str, size_t size) {
    uint64_t n = 0;
    const char* c = str;
    while (size > 0 && *c >= '0' && *c <= '7') {
        n = n * 8 + (*c - '0');
        c++;
        size--;
    }
    return n;
}

/**
 * @brief Initializes the tar file system.
 */
void tar_init(void* address) {
    initrd_ptr = address;
}

/**
 * @brief Looks up a file by name in the initrd.
 */
void* tar_lookup(const char* filename) {
    if (initrd_ptr == NULL) {
        return NULL;
    }

    tar_header_t* header = (tar_header_t*)initrd_ptr;
    // Loop through all file headers in the tar archive
    while (header->filename[0] != '\0') {
        char* name = header->filename;
        if (name[0] == '.' && name[1] == '/') {
            name += 2; // Skip leading "./"
        }
        if (strcmp(name, filename) == 0) {
            // Found it! The file data is right after the 512-byte header.
            return (void*)(header + 1);
        }

        // Not a match, so find the next header.
        // 1. Get the file size from the octal string.
        uint64_t size = octal_to_uint(header->size_str, 11);

        // 2. Round the size up to the nearest 512-byte block.
        uint64_t blocks = (size + 511) / 512;

        // 3. Jump forward by one header (512 bytes) + all the data blocks.
        header = (tar_header_t*)((char*)header + 512 + (blocks * 512));
    }

    return NULL; // File not found
}

/**
 * @brief Lists all files in the tar archive (for debugging purposes).
 */
void tar_list_files(void) {
    if (initrd_ptr == NULL) {
        fb_print("tar: Not initialized.\n");
        return;
    }

    tar_header_t* header = (tar_header_t*)initrd_ptr;
    fb_print("Files in initrd:\n");

    // Loop through all file headers in the tar archive
    while (header->filename[0] != '\0') {
        // Print the filename
        fb_print("  - ");
        fb_print(header->filename);
        fb_print("\n");

        // Get the file size from the octal string.
        uint64_t size = octal_to_uint(header->size_str, 11);

        // Round the size up to the nearest 512-byte block.
        uint64_t blocks = (size + 511) / 512;

        // Jump forward by one header (512 bytes) + all the data blocks.
        header = (tar_header_t*)((char*)header + 512 + (blocks * 512));
    }
}

