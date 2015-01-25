/* declares test cases for copying struct data from a file */
#include <stdlib.h>

#include <file_structor.h>

/* the expected value for a single chunk of memory inside a mapped struct */
struct output_chunk {
	/* the location of the chunk inside the struct */
	size_t offset;
	/* the number of contiguous bytes to check */
	size_t size;
	/* the expected data value */
	void *expected_data;
};

/*
 * a test vector for testing
 * "open_file_structor", "init_file_struct" and copying functions.
 */
struct file_struct_tv {
	/* the name of the file inside the "test_inputs" folder to open */
	char *test_name;
	/* Do we expect a successful reading of the data? */
	int expect_success;

	/* the size of the struct to initialize */
	size_t size;
	/* the starting location in the file to read */
	size_t start_in_file;

	/*
	 * Assumming initialization passed,
	 * this function tests copying functions.
	 * output:	the buffer for storing the copied struct data
	 * input:	contains the wrapper to the raw data
	 * returns	1 if test passed
	 *		  (including if an expected error was found);
	 *		0 otherwise
	 */
	int (*reader)(void *output, struct file_struct *input);

	/* data for the expected results */
	union {
		/* Do we expect success? */
		struct {
			/* the number of output chunks to check */
			size_t n_outputs;
			/* the output chunks to check */
			struct output_chunk *outputs;
		} success;
		/* Or do we expect failure? */
		struct {
			/* Should the failure occur during initialization? */
			int fail_at_init;
			/* If so, what should errno be? */
			int init_failure;
		} failure;
	} result;
};

/*
 * declare the array of test vectors that will be run by "test_file_structs"
 * in "test_file_structor.c"
 */
#define N_FILE_STRUCT_TVS	6
extern struct file_struct_tv *file_struct_tvs[N_FILE_STRUCT_TVS];
