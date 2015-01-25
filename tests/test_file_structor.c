#include "file_structor_tests.h"

#include <logger.h>
#include <debug_assert.h>

#include <stdlib.h>
#include <string.h>

/* the directory containing the test input files */
#define TEST_FILE_DIR		"test_inputs/"

/* the length of the directory name, including the separator */
const size_t dir_len = 12;

/*
 * Assumming the reader function did not report an error,
 * check that the copied data matches what was expected.
 * tv:		the test vector containing the expected data
 * output:	the buffer containing the copied data
 * returns	1 if data is correct; 0 otherwise
 */
static int check_output_chunks(struct file_struct_tv *tv, void *output)
{
	size_t chunk_i;
	int matching = 1;

	for (chunk_i = 0; chunk_i < tv->result.success.n_outputs; chunk_i++) {
		struct output_chunk *
		expected_chunk = &(tv->result.success.outputs[chunk_i]);
		size_t byte_size = expected_chunk->size;
		void *expected_bytes = expected_chunk->expected_data;
		void *actual_bytes = output + expected_chunk->offset;

		if (memcmp(expected_bytes, actual_bytes, byte_size)) {
			size_t
			string_size = hex_str_length(byte_size);
			char expected_string[string_size];
			char actual_string[string_size];

			bytes_to_hex(expected_string, expected_bytes,
				     byte_size);
			bytes_to_hex(actual_string, actual_bytes, byte_size);

			printlg(ERROR_LEVEL,
				"Expected copy of element %u "
				"to look like \n"
				"0x%s,\n"
				"but got\n"
				"0x%s.\n",
				(unsigned) chunk_i,
				expected_string, actual_string);

			matching = 0;
		}
	}

	return matching;
}

/*
 * Assumming that initialization of the struct chunk passed,
 * check if it was supposed to pass, and if so, try the reading test.
 * tv:			the test vector containing the expected results
 * input_holder:	contains the raw struct chunk data to copy
 * returns		1 if the result is expected; 0 otherwise
 */
static int
test_reader(struct file_struct_tv *tv, struct file_struct *input_holder)
{
	if (!tv->expect_success && tv->result.failure.fail_at_init) {
		printlg(ERROR_LEVEL, "Uncaught initialization error.\n");
		return 0;
	} else {
		uint8_t output_buffer[tv->size];

		if (!tv->reader(output_buffer, input_holder)) {
			if (tv->expect_success) {
				printlg(ERROR_LEVEL, "Copying test failed.\n");
			} else {
				printlg(ERROR_LEVEL,
					"Did not catch copying error.\n");
			}
			return 0;
		} else {
			if (tv->expect_success) {
				return check_output_chunks(tv, output_buffer);
			} else {
				return 1;
			}
		}
	}
}

/*
 * Assuming the file was successfully opened, run the test.
 * tv:		the vector containing testing parameters and results
 * structor:	contains the opened test file
 * returns	1 if the test passed; 0 otherwise
 */
static int _test_file_struct(struct file_struct_tv *tv,
			     struct file_structor *structor)
{
	struct file_struct input_holder;

	if (init_file_struct(&input_holder, structor, tv->size,
			     tv->start_in_file)) {
		if (tv->expect_success) {
			printlg(ERROR_LEVEL, "Unexpected testing error: %d.\n",
				errno);
			return 0;
		} else {
			if (tv->result.failure.fail_at_init) {
				return 1;
			} else {
				printlg(ERROR_LEVEL,
					"Premature initialization error: %d.\n",
					errno);
				return 0;
			}
		}
	} else {
		int ret = test_reader(tv, &input_holder);
		teardown_file_struct(&input_holder);
		return ret;
	}
}

/*
 * Start test of a single "struct file_struct_tv".
 * tv:		the vector containing testing parameters and results
 * returns	1 if the test passed; 0 otherwise
 */
static int test_file_struct(struct file_struct_tv *tv)
{
	size_t name_len = strlen(tv->test_name);
	char path[dir_len + name_len + 1];
	struct file_structor structor;

	debug_assert(dir_len == strlen(TEST_FILE_DIR));

	strncpy(path, TEST_FILE_DIR, dir_len + 1);
	strncat(path, tv->test_name, name_len + 1);

	if (open_file_structor(&structor, path)) {
		printlg(ERROR_LEVEL, "Could not open file at %s.\n",
			path);
		return 0;
	} else {
		int ret = _test_file_struct(tv, &structor);
		close_file_structor(&structor);
		return ret;
	}
}

/*
 * Run all the tests in "file_struct_tvs".
 */
static void test_file_structs()
{
	size_t tv_i;

	for (tv_i = 0; tv_i < N_FILE_STRUCT_TVS; tv_i++) {
		printlg(INFO_LEVEL, "Testing file struct copying: %u...\n",
			(unsigned) tv_i);
		if (test_file_struct(file_struct_tvs[tv_i])) {
			printlg(INFO_LEVEL, "Passed!\n");
		} else {
			printlg(ERROR_LEVEL, "Failed!\n");
		}
	}
}

int main()
{
	test_file_structs();

	return 0;
}
