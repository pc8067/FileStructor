#include "file_structor_tests.h"

#include <logger.h>
#include <stdlib.h>
#include <errno.h>

/*
 * In the first successful test,
 * we will test a struct with an 8-byte, big-endian integer,
 * a 2-byte, little-endian integer,
 * and 16 characters,
 * inside a file, called "default_test",
 * with 16 bytes, ie. two 8-byte integers, before and after it.
 * In the unsuccessful tests, we will try variations.
 */
/* the file to test */
#define DEFAULT_TEST_FILE	"default_test"
/* the size of an 8-byte integer, which is the first member of the struct */
#define LONG_INT_SIZE		sizeof(uint64_t)
/* the size of a 2-byte integer, which is the second member of the struct */
#define HALF_INT_SIZE		sizeof(uint16_t)
/* the length of the string */
#define N_CHARS	0x10
/* the size of the string, the last member of the struct */
#define STRING_SIZE		(sizeof(char) * N_CHARS)
/*
 * the beginning of the desired struct and its first element,
 * after the initial padding
 */
#define FIRST_NUMBER_START	0x10
/* the beginning of the second element */
#define SECOND_NUMBER_START	(FIRST_NUMBER_START + LONG_INT_SIZE)
/* the beginning of the third element */
#define STRING_START		(SECOND_NUMBER_START + HALF_INT_SIZE)
/* the end of the struct inside the file */
#define STRUCT_END		(STRING_START + STRING_SIZE)
/* the number of bytes in the struct */
#define STRUCT_SIZE		(STRUCT_END - FIRST_NUMBER_START)
/* the total size of the file, including the final padding */
#define FILE_SIZE		(STRUCT_END + LONG_INT_SIZE * 2)

/* a position beyond the range of the file */
#define OVER_FILE_SIZE		(FILE_SIZE + 1)

/*
 * The struct starts in the file, but is too large,
 * so we expect a failure during initialization.
 */
struct file_struct_tv chunk_too_large = {
	.test_name = DEFAULT_TEST_FILE,
	.expect_success = 0,

	.size = FILE_SIZE,
	.start_in_file = FIRST_NUMBER_START,

	.reader = NULL,

	.result = {
		.failure = {
			.fail_at_init = 1,
			.app_error = FSERR_OUT_OF_FILE
		}
	},
};

/*
 * The struct starts after the file,
 * so we expect a failure during initialization.
 */
struct file_struct_tv chunk_out_of_range = {
	.test_name = DEFAULT_TEST_FILE,
	.expect_success = 0,

	.size = LONG_INT_SIZE,
	.start_in_file = OVER_FILE_SIZE,

	.reader = NULL,

	.result = {
		.failure = {
			.fail_at_init = 1,
			.app_error = FSERR_OUT_OF_FILE
		}
	},
};

static int read_member_too_large(void *output, struct file_struct *input)
{
	enum fs_status status;
	if ((status = copy_section(output, input, SECOND_NUMBER_START,
				   STRUCT_SIZE, BIG_END))) {
		if (status == FSERR_OUT_OF_STRUCT) {
			return 1;
		} else {
			printlg(ERROR_LEVEL,
				"Expected error %d for reading large size, "
				"but got %d.\n", FSERR_OUT_OF_STRUCT, status);
			return 0;
		}
	} else {
		printlg(ERROR_LEVEL,
			"Did not catch large size copy error.\n");
		return 0;
	}
}

/*
 * Read too many bytes from a file starting inside the struct chunk,
 * so expect a failure during reading.
 */
struct file_struct_tv member_too_large = {
	.test_name = DEFAULT_TEST_FILE,
	.expect_success = 0,

	.size = STRUCT_SIZE,
	.start_in_file = FIRST_NUMBER_START,

	.reader = read_member_too_large,

	.result = {
		.failure = {
			.fail_at_init = 0,
		}
	},
};

static int read_member_out_of_range(void *output, struct file_struct *input)
{
	enum fs_status status;
	if ((status = copy_section(output, input, STRUCT_END, LONG_INT_SIZE,
				   BIG_END))) {
		if (status != FSERR_OUT_OF_STRUCT) {
			printlg(ERROR_LEVEL,
				"Expected error %d for reading out of range, "
				"but got %d.\n",
				FSERR_OUT_OF_STRUCT, status);
			return 0;
		} else {
			return 1;
		}
	} else {
		printlg(ERROR_LEVEL,
			"Did not catch out of range copy error.\n");
		return 0;
	}
}

/* Read after the struct chunk, so expect a failure during reading. */
struct file_struct_tv member_out_of_range = {
	.test_name = DEFAULT_TEST_FILE,
	.expect_success = 0,

	.size = STRUCT_SIZE,
	.start_in_file = FIRST_NUMBER_START,

	.reader = read_member_out_of_range,

	.result = {
		.failure = {
			.fail_at_init = 0,
		}
	},
};

/* the struct type to which to write the data */
struct test_struct {
	uint64_t first_int;
	uint16_t second_int;
	char string[N_CHARS];
};

static int read_all_orders(void *output, struct file_struct *input)
{
	struct test_struct *output_struct = (struct test_struct *) output;
	enum fs_status status;

	if ((status = COPY_MEMBER(output_struct, input, struct test_struct,
				  first_int, BIG_END))) {
		printlg(ERROR_LEVEL,
			"Unexpected error %d while copying first_int.\n",
			status);
		return 0;
	}

	if ((status = COPY_MEMBER(output_struct, input, struct test_struct,
				  second_int, LITTLE_END))) {
		printlg(ERROR_LEVEL,
			"Unexpected error %d while copying second_int.\n",
			status);
		return 0;
	}

	if ((status = COPY_DIRECT_MEMBER(output_struct, input,
					 struct test_struct, string))) {
		printlg(ERROR_LEVEL,
			"Unexpected error %d while copying string.\n", status);
		return 0;
	}

	return 1;
}

/* the integers will be stored depending on the machine order */
uint64_t first_int = 0x0001020304050607;
uint16_t second_int = 0x0123;
/* the string will be copied directly */
char string[N_CHARS] = "0123456789abcdef";

#define N_OUTPUTS_ALL_ORDERS	3
static struct output_chunk outputs_all_orders[N_OUTPUTS_ALL_ORDERS] = {
	{
		.offset = 0,
		.size = LONG_INT_SIZE,
		.expected_data = &first_int,
	}, {
		.offset = LONG_INT_SIZE,
		.size = HALF_INT_SIZE,
		.expected_data = &second_int,
	}, {
		.offset = LONG_INT_SIZE + HALF_INT_SIZE,
		.size = LONG_INT_SIZE,
		.expected_data = string,
	}
};

/* Expect to successfully read the three elements of "struct test_struct". */
struct file_struct_tv all_orders = {
	.test_name = DEFAULT_TEST_FILE,
	.expect_success = 1,

	.size = STRUCT_SIZE,
	.start_in_file = FIRST_NUMBER_START,

	.reader = read_all_orders,

	.result = {
		.success = {
			.n_outputs = N_OUTPUTS_ALL_ORDERS,
			.outputs = outputs_all_orders
		}
	},
};

/*
 * The second successful test will consist of a file with only a struct with
 * two arrays of 8 short integers,
 * one whose elements are read in little-endian order,
 * and one whose elements are read in big-endian order.
 */
#define ARRAY_TEST_FILE	"array_test"
#define N_SHORTS	8
struct array_struct {
	uint16_t little_array[N_SHORTS];
	uint16_t big_array[N_SHORTS];
};

/* the total size of each array */
#define ARRAY_SIZE	(sizeof(uint16_t) * N_SHORTS)

static int read_array_order(void *output, struct file_struct *input)
{
	struct array_struct *output_struct = (struct array_struct *) output;
	enum fs_status status;

	COPY_ARRAY_MEMBER(output_struct, input, struct array_struct,
			  little_array, LITTLE_END, status);
	if (status) {
		printlg(ERROR_LEVEL,
			"Unexpected error %d while copying little_array.\n",
			status);
		return 0;
	}

	COPY_ARRAY_MEMBER(output_struct, input, struct array_struct,
			  big_array, BIG_END, status);
	if (status) {
		printlg(ERROR_LEVEL,
			"Unexpected error %d while copying big_array.\n",
			status);
		return 0;
	}

	return 1;
}

/*
 * The two output arrays will be identical,
 * but the input shorts are in opposite orders.
 */
uint16_t shorts[N_SHORTS] = {0x0001, 0x0203, 0x0405, 0x0607,
			     0x0809, 0x0a0b, 0x0c0d, 0x0e0f};

#define N_ARRAYS		2
static struct output_chunk outputs_array_order[N_ARRAYS] = {
	{
		.offset = 0,
		.size = ARRAY_SIZE,
		.expected_data = shorts,
	}, {
		.offset = ARRAY_SIZE,
		.size = ARRAY_SIZE,
		.expected_data = shorts,
	}
};

/* Expect to successfully read the two elements of "struct array_struct". */
struct file_struct_tv array_order = {
	.test_name = ARRAY_TEST_FILE,
	.expect_success = 1,

	.size = sizeof(struct array_struct),
	.start_in_file = 0,

	.reader = read_array_order,

	.result = {
		.success = {
			.n_outputs = N_ARRAYS,
			.outputs = outputs_array_order
		}
	},
};

struct file_struct_tv *file_struct_tvs[N_FILE_STRUCT_TVS] = {
	&chunk_too_large, &chunk_out_of_range,
	&member_too_large, &member_out_of_range,
	&all_orders, &array_order
};
