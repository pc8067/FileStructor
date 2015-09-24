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
 * if and when an error should be returned.
 * Larger values mean the error should occur later,
 * so the largest value means that no error should occur at all.
 */
enum fs_fail_stage {
	/*
	 * An error should be returned
	 * during the opening of "struct file_structor".
	 */
	FSFAIL_OPEN,
	/*
	 * An error should be returned during the initalization of
	 * "struct file_struct".
	 */
	FSFAIL_INIT,
	/* An error should be returned when reading data. */
	FSFAIL_READ,
	/* No error should be returned. */
	FSFAIL_NEVER
};

/* expected error values */
struct fail_result {
	/* What should the returned error status be? */
	enum fs_status app_error;
	/* If "app_error" == FSERR_ERRNO, what should errno be? */
	int expected_errno;
};

/*
 * a test vector for testing
 * "open_file_structor", "init_file_struct" and copying functions.
 */
struct file_struct_tv {
	/* the name of the file inside the "test_inputs" folder to open */
	char *test_name;
	/* Do we expect a failure, and if so, when? */
	enum fs_fail_stage fail_stage;

	/* the size of the struct to initialize */
	size_t size;
	/* the starting location in the file to read */
	size_t start_in_file;

	/*
	 * Assumming initialization passed,
	 * this function tests copying functions, expecting to pass.
	 * output:	the buffer for storing the copied struct data
	 * input:	contains the wrapper to the raw data
	 * returns	1 if test has not failed yet
	 *			(but the output may be wrong)
	 *		0 otherwise
	 */
	int (*good_reader)(void *output, struct file_struct *input);
	/*
	 * Assumming initialization passed,
	 * this function tests copying functions, expecting to fail.
	 * output:	the buffer for storing the copied struct data
	 * input:	contains the wrapper to the raw data
	 * failure:	the expected error data
	 * returns	1 if test failed as expected
	 *		0 otherwise
	 */
	int (*bad_reader)(void *output, struct file_struct *input,
			  struct fail_result *failure);

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
		struct fail_result failure;
	} result;
};

/*
 * Assuming the failure occured at the correct step,
 * check if the particular error is as expected,
 * and, if needed, also check that "errno" has the correct value.
 * failure:	the expected error information
 * status:	the expected error return value
 * returns	1 if "status" matches what was expected,
 *			and, if "status" == FSERR_ERRNO,
 *			that the value of "errno"
 *			(a global variable, not a parameter) is correct;
 *		0 otherwise
 */
int check_error(struct fail_result *failure, enum fs_status status);

/*
 * declare the array of test vectors that will be run by "test_file_structs"
 * in "test_file_structor.c"
 */
#define N_FILE_STRUCT_TVS	9
extern struct file_struct_tv *file_struct_tvs[N_FILE_STRUCT_TVS];
