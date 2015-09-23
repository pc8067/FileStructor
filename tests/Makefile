.PHONY:
include ../common.mk
INCLUDE=-I../include
CPPFLAGS=$(_CPPFLAGS) $(INCLUDE)

LIBS_DIR=../libs/
LIBS=../src/file_structor.a $(LIBS_DIR)commonc.a

FILE_STRUCTOR_TEST_OBJS=test_file_structor.o file_structor_tests.o
OBJS=$(FILE_STRUCTOR_TEST_OBJS)

TARGETS=test_file_structor

all: $(SUBDIRS) $(OBJS) $(TARGETS)

test_file_structor: $(FILE_STRUCTOR_TEST_OBJS) $(LIBS)
	$(CC) $(CPPFLAGS) -o $@ $^

clean:
	$(RM) $(RM_FLAGS) $(OBJS) $(TARGETS)
