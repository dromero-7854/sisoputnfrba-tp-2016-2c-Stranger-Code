/*
 * pokedex-server-osada.h
 *
 *  Created on: 18/9/2016
 *      Author: utnso
 */
#include <stdint.h>

#ifndef __OSADA_H__
#define __OSADA_H__

#define	OSADA_ENOENT		 		1 // no such file or directory
#define	OSADA_ISREG		 			2 // is a regular file
#define	OSADA_ISDIR		 			3 // is a directory
#define	OSADA_ENOTDIR				4 // not a directory
#define	OSADA_ENOSPC				5 // no space left on device
#define	OSADA_EMPTYDIR 				6 // empty directory
#define	OSADA_NOTEMPTYDIR 			7 // no empty directory
#define	OSADA_SEXE		 			8 // successful execution
#define	OSADA_ENAMETOOLONG		 	9 // name too long

#define OSADA_BLOCK_SIZE 64
#define OSADA_FILE_BLOCK_SIZE 32
#define OSADA_MAPP_SIZE 4
#define OSADA_FILENAME_LENGTH 17
#define FILE_BLOCKS_MOUNT 2048
#define FILE_TABLE_SIZE 1024

typedef unsigned char osada_block[OSADA_BLOCK_SIZE];
typedef uint32_t osada_block_pointer;

#pragma pack(push, 1)

typedef struct {
	unsigned char magic_number[7]; 		// OSADAFS
	uint8_t version;
	uint32_t fs_blocks; 				// total amount of blocks
	uint32_t bitmap_blocks; 			// bitmap size in blocks
	uint32_t allocations_table_offset; 	// allocations table's first block number
	uint32_t data_blocks; 				// amount of data blocks
	unsigned char padding[40]; 			// useless bytes just to complete the block size
} osada_header;

typedef enum __attribute__((packed)) {
	DELETED = '\0',
	REGULAR = '\1',
	DIRECTORY = '\2',
} osada_file_state;

typedef struct {
	osada_file_state state;
	unsigned char fname[OSADA_FILENAME_LENGTH];
	uint16_t parent_directory;
	uint32_t file_size;
	uint32_t lastmod;
	osada_block_pointer first_block;
} osada_file;

#pragma pack(pop)

#endif
