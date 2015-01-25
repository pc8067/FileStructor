.PHONY:
include ../common.mk
INCLUDE=-I../include
CPPFLAGS=$(_CPPFLAGS) $(INCLUDE)
SUBDIRS=
OBJS=file_structor.o
TARGETS=file_structor.a
all: $(SUBDIRS) $(OBJS) $(TARGETS)
file_structor.a: file_structor.o
	$(AR) $(AR_FLAGS) $@ $^
clean:
	$(RM) $(RM_FLAGS) $(OBJS) $(TARGETS)
