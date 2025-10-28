#ifndef __TAR_H__
#define __TAR_H__

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initializes the tar file system with the initrd's memory address.
 * @param address The pointer to the start of the initrd.tar file.
 */
void tar_init(void* address);

/**
 * @brief Looks up a file by name in the initrd.
 * @param filename The name of the file to find (e.g., "hello.txt").
 * @return A pointer to the file's *content* in memory, or NULL if not found.
 */
void* tar_lookup(const char* filename);

/**
 * @brief Lists all files in the tar archive (for debugging purposes).
 */
void tar_list_files(void);

#endif // __TAR_H__