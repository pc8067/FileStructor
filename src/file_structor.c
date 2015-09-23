#include <file_structor.h>
#include <logger.h>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

enum fs_status
open_file_structor(struct file_structor *to_open, const char *path)
{
	to_open->fd = open(path, O_RDONLY);
	if (to_open->fd < 0) {
		printlg(ERROR_LEVEL, "Unable to open file %s.\n", path);
		return FSERR_ERRNO;
	} else {
		struct stat size_stat;

		if (fstat(to_open->fd, &size_stat)) {
			printlg(ERROR_LEVEL,
				"Unable to find the size of file descriptor %d "
				"of file %s.\n", to_open->fd, path);
			close(to_open->fd);
			to_open->fd = -1;
			return FSERR_ERRNO;
		}

		to_open->size = size_stat.st_size;

		return FS_NO_ERROR;
	}
}

enum fs_status close_file_structor(struct file_structor *to_close)
{
	if (to_close->fd < 0) {
		return FS_NO_ERROR;
	}

	if (close(to_close->fd)) {
		printlg(WARNING_LEVEL, "Unable to close file descriptor %d.\n",
			to_close->fd);
		return FSERR_ERRNO;
	}

	to_close->fd = -1;
	to_close->size = 0;

	return FS_NO_ERROR;
}

enum fs_status
init_file_struct(struct file_struct *to_init, struct file_structor *src_file,
		 off_t size, off_t start_in_file)
{
	off_t start_adjustment, adjusted_start;

	if (start_in_file + size > src_file->size) {
		printlg(ERROR_LEVEL,
			"Requesting struct chunk in %u-%u, "
			"but file only has data up to %u.\n",
			(unsigned) start_in_file,
			(unsigned) (start_in_file + size),
			(unsigned) src_file->size);
		return FSERR_OUT_OF_FILE;
	}

	start_adjustment = start_in_file % sysconf(_SC_PAGE_SIZE);
	adjusted_start = start_in_file - start_adjustment;

	to_init->mapping_start = mmap(NULL, (size_t) size,
				      PROT_READ, MAP_SHARED,
				      src_file->fd, adjusted_start);

	if (to_init->mapping_start == MAP_FAILED) {
		printlg(ERROR_LEVEL,
			"Could not map from file %d to range %u-%u: %d.\n",
			src_file->fd,
			(unsigned) start_in_file,
			(unsigned) (start_in_file + size), errno);
		to_init->mapping_start = NULL;
		to_init->data = NULL;
		return -1;
	}

	to_init->data = to_init->mapping_start + start_adjustment;
	to_init->src_file = src_file;
	to_init->size = size;
	to_init->start_in_file = start_in_file;

	return FS_NO_ERROR;
}

enum fs_status
derive_file_struct(struct file_struct *to_init, struct file_struct *big_struct,
		   off_t size, size_t start_in_struct)
{
	if (start_in_struct + size > big_struct->size) {
		printlg(ERROR_LEVEL,
			"Requesting struct chunk in %u-%u, "
			"but struct only has data up to %u.\n",
			(unsigned) start_in_struct,
			(unsigned) (start_in_struct + size),
			(unsigned) big_struct->size);
		return FSERR_OUT_OF_STRUCT;
	}

	to_init->data = big_struct->data + start_in_struct;
	to_init->src_file = big_struct->src_file;
	to_init->size = size;
	to_init->start_in_file = big_struct->start_in_file + start_in_struct;
	to_init->mapping_start = NULL;

	return FS_NO_ERROR;
}

enum fs_status teardown_file_struct(struct file_struct *to_teardown)
{
	if (to_teardown->data == NULL) {
		return FS_NO_ERROR;
	} else {
		if (to_teardown->mapping_start != NULL) {
			if (munmap(to_teardown->mapping_start,
				   to_teardown->size)) {
				printlg(WARNING_LEVEL,
					"Unable to unmap memory range %p-%p: "
					"%d\n",
					to_teardown->data,
					to_teardown->data + to_teardown->size,
					errno);
				return -1;
			}
			to_teardown->mapping_start = NULL;
		}

		to_teardown->data = NULL;
		to_teardown->src_file = NULL;

		return FS_NO_ERROR;
	}
}
