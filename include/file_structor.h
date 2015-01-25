/*
 * Generic tools for extracting information from a file,
 * and putting it into a struct in memory.
 * The file contents are assumed to be aligned like the struct,
 * although endianness may vary.
 */
#ifndef FILE_STRUCTOR_H
#define FILE_STRUCTOR_H

#include <logger.h>
#include <debug_assert.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>

/* wrapper around the file from which to map the data chunks */
struct file_structor {
	/* the descriptor of the source file */
	int fd;
	/* the size of the source file */
	off_t size;
};

/*
 * Try to initialize a "struct file_structor",
 * given the path of the source file.
 * to_open:	the source wrapper to initialize
 * path:	the path of the source file
 * returns	0 on success,
 *		-1 if opening the file or finding its size failed,
 *		   with errno set by the failing function: "open" or "fstat"
 */
int open_file_structor(struct file_structor *to_open, const char *path);
/*
 * Try to close the source file,
 * and set its descriptor to indicate that it is invalid.
 * to_close:	the wrapper whose source file descriptor to close
 * returns	0 on success or if
 *		  the file descriptor does not need to be closed;
 *		-1 if the file must be closed, but closing failed,
 *		   with errno set by the failing function: "close"
 */
int close_file_structor(struct file_structor *to_close);

/*
 * contains pointer to a data struct chunk in the file
 */
struct file_struct {
	/* the source file */
	struct file_structor *src_file;
	/* the number of bytes in the struct, ie. its size */
	size_t size;
	/* the location of the struct chunk in the file */
	off_t start_in_file;

	/* the raw data of the struct chunk in the file */
	void *data;
	/*
	 * If the "data" field was directly mapped from "src_file"
	 * using "init_file_struct",
	 * this field points to the output of the mapping containing "data",
	 * which is the page boundary before it.
	 * Otherwise, "data" was taken from a subset of the "data" field of
	 * another "struct file_struct", using "derive_file_struct",
	 * and this pointer is NULL.
	 */
	void *mapping_start;
};

/*
 * Initialize a struct chunk, with a mapping to the data in the file.
 * to_init:		the chunk for which to map the data
 * src_file:		the source wrapper,
 *			and the value for the "src_file" field
 * size:		the size of the struct
 * start_in_file:	the starting location of the chunk in the file
 * returns		0 on success,
 *			-1 if requested chunk is beyond the range of the file,
 *			   indicated by its size,
 *			   in which case errno will be set to ERANGE,
 *			   or if "mmap" failed, in which case
 *			   the failed function will set errno
 */
int
init_file_struct(struct file_struct *to_init, struct file_structor *src_file,
		 off_t size, off_t start_in_file);
/*
 * a wrapper around "init_file_struct" that
 * automatically finds the size of the struct
 * to_init:		the chunk for which to map the data
 * src_file:		the source wrapper,
 *			and the value for the "src_file" field
 * data_type:		the type of the source destination
 * start_in_file:	the starting location of the chunk in the file
 * returns		0 on success,
 *			-1 if requested chunk is beyond the range of the file,
 *			   indicated by its size,
 *			   in which case errno will be set to ERANGE,
 *			   or "mmap" failed, in which case
 *			   the failed function will set errno
 */
#define INIT_FILE_STRUCT(to_init, src_file, data_type, start_in_file) \
	init_file_struct(to_init, src_file, sizeof(data_type), start_in_file)

/*
 * Initialize a sub struct chunk inside the given struct chunk.
 * to_init:		the chunk for which to map a subrange of the data
 * big_struct:		the larger struct chunk containing "to_init"
 * size:		the size of the new struct
 * start_in_struct:	the starting location of the chunk in the source struct
 * returns		0 on success,
 *			-1 if requested chunk is beyond the range
 *			   of the larger struct indicated by its size,
 *			   in which case errno will be set to ERANGE
 */
int derive_file_struct(struct file_struct *to_init,
		       struct file_struct *big_struct, off_t size,
		       size_t start_in_struct);

/*
 * wrapper around "derive_file_struct"
 * that calculates "size" and "start_in_struct"
 * based on the struct types of the containing, or "big",
 * and contained, or "small", structs,
 * and the member name of the "small" struct
 * to_init:		the chunk for which to map the data
 * big_struct:		the larger struct chunk containing "to_init"
 * big_type:		the type of "big_struct"
 * small_member:	the name of the member in "big_struct" from which to
 *			derive the data range
 * returns		0 on success,
 *			-1 if requested chunk is
 *			   beyond the range of "big_struct",
 *			   indicated by its size,
 *			   in which case errno will be set to ERANGE
 */
#define DERIVE_FILE_STRUCT(to_init, big_struct, big_type, small_member) \
	derive_file_struct(to_init, big_struct, \
			   sizeof(((big_type *) NULL)->small_member), \
			   offsetof(big_type, small_member))

/*
 * Unmap the data chunk, if this struct contains the original mapping,
 * so that the struct can be deallocated.
 * Set all the pointers to NULL.
 * to_teardown:		the data chunk whose data to unmap,
 *			and have pointer fields set to NULL
 * returns		0 iff successful, or unmapping is not needed;
 *			-1 on error in unmapping,
 *			   with errno set by the failing function: "munmap"
 */
int teardown_file_struct(struct file_struct *to_teardown);

/*
 * Helper function to "copy_section" to copy memory in reverse,
 * for flipping endiannes.
 * dest:	the destination to copy to
 * src:		the source to copy from
 * size:	the number of bytes to copy
 * returns	the dest pointer
 */
inline static void *memcpy_rev(void *dest, void *src, size_t size)
{
	uint8_t *dest_bytes = dest, *src_bytes = src;
	size_t byte_i;

	for (byte_i = 0; byte_i < size; byte_i++) {
		dest_bytes[byte_i] = src_bytes[size - 1 - byte_i];
	}

	return dest;
}

/*
 * the endianness types
 */
enum endianness {
	BIG_END,
	LITTLE_END
};

/* the pattern used to test for endianness */
#define ENDIAN_TESTER_PATTERN	0x0001
/*
 * the high byte of the test pattern,
 * which should be the first byte if the machine is big endian
 */
#define ENDIAN_TESTER_HIGH	(ENDIAN_TESTER_PATTERN >> 8)

/*
 * helper function to "portable_memcpy"
 * to check if endianness needs to be reversed for this machine,
 * and to "COPY_DIRECT_MEMBER",
 * so that "copy_section" copies the bytes in the original order.
 * returns	the endianness type of this machine
 */
inline static enum endianness machine_endianness()
{
	uint16_t tester = ENDIAN_TESTER_PATTERN;
	uint8_t *first_tester = (uint8_t *) &tester;

	if (*first_tester == ENDIAN_TESTER_HIGH) {
		return BIG_END;
	} else {
		debug_assert(*first_tester == (uint8_t) (ENDIAN_TESTER_PATTERN &
							 0xff));
		return LITTLE_END;
	}
}

/*
 * Copies memory, so that either the source or the destination
 * has the machine byte order,
 * and the other memory area should be in the specified byte order.
 * If they were both in machine order or both in specified order,
 * then the regular "memcpy" function should be used instead.
 * dst:		the destination pointer,
 *		which will be in either the machine byte order,
 *		or the specified byte order.
 * src:		the source pointer.
 *		If "dst" will be in the machine byte order,
 *		"src" will be in the specified byte order;
 *		otherwise "src" will be in the machine byte order.
 * size:	the number of bytes to copy
 * endianness:	The specified byte order of either "src" or "dst", but not both.
 */
inline static void portable_memcpy(void *dst, void *src, size_t size,
				   enum endianness endianness)
{
	if (endianness == machine_endianness()) {
		memcpy(dst, src, size);
	} else {
		memcpy_rev(dst, src, size);
	}
}

/*
 * Convert and copy a piece of data in the struct chunk to memory
 * dst:		the pointer to the destination struct,
 *		ie. the base, not the member
 * src:		the source chunk
 * offset:	the offset in the destination struct and in the raw data
 * size:	the number of bytes to copy
 * endianness	the desired endianness
 * returns	0 on success;
 *		-1 if the requested section is outside the range of the chunk,
 *		   in which case errno will be set to ERANGE
 */
inline static int
copy_section(void *dst, struct file_struct *src, off_t offset, size_t size,
	     enum endianness endianness)
{
	if (offset + size > src->size) {
		printlg(ERROR_LEVEL,
			"Requesting data in %u-%u, "
			"but struct chunk only has data up to %u.\n",
			(unsigned) offset, (unsigned) (offset + size),
			(unsigned) src->size);
		errno = ERANGE;
		return -1;
	} else {
		void *dst_section = dst + offset;
		void *src_section = src->data + offset;

		portable_memcpy(dst_section, src_section, size, endianness);

		return 0;
	}
}

/*
 * Wrapper function around "copy_section" to copy a chosen struct member
 * dst:		the pointer to the destination struct,
 *		ie. the base, not the member
 * src:		the source chunk
 * type:	the type of the struct
 * member:	the name of the member
 * endianness:	the desired endianness
 * returns	0 on success;
 *		-1 if the requested section is outside the range of the chunk,
 *		   in which case errno will be set to ERANGE
 */
#define COPY_MEMBER(dst, src, type, member, endianness) \
	copy_section(dst, src, offsetof(type, member), \
		     sizeof((((type *) dst)->member)), endianness)
/*
 * Wrapper function around "COPY_MEMBER", and thus "copy_section",
 * to copy a struct member in the original order,
 * as if the endianness matches the machine's
 * dst:		the pointer to the destination struct,
 *		ie. the base, not the member
 * src:		the source chunk
 * type:	the type of the struct
 * member:	the name of the member
 * returns	0 on success;
 *		-1 if the requested section is outside the range of the chunk,
 *		   in which case errno will be set to ERANGE
 */
#define COPY_DIRECT_MEMBER(dst, src, type, member) \
	COPY_MEMBER(dst, src, type, member, machine_endianness())

/*
 * Wrapper function around "copy_section"
 * to copy an array member of a struct,
 * with the elements copied in the original order,
 * but the bytes in each element copied in the chosen order.
 * dst:			the pointer to the destination struct,
 *			ie. the base, not the member
 * src:			the source chunk
 * type:		the type of the struct
 * member:		the name of the member
 * endianness:		the desired endianness
 * status:		will be set to
 *			0 on success;
 *			-1 if the requested section is
 *			   outside the range of the chunk,
 *			   in which case errno will be set to ERANGE
 */
#define COPY_ARRAY_MEMBER(dst, src, type, member, endianness, status) do { \
	size_t full_width = sizeof((((type *) dst)->member)); \
	size_t array_start = offsetof(type, member); \
	size_t width = sizeof((((type *) dst)->member[0])); \
	size_t array_offset; \
	debug_assert(full_width % width == 0); \
	status = 0; \
	for (array_offset = 0; array_offset < full_width; \
	     array_offset += width) { \
		status |= copy_section(dst, src, array_start + array_offset, \
				       width, endianness); \
		if (status) { \
			break; \
		} \
	} \
} while (0);

#endif /* FILE_STRUCTOR_H */
