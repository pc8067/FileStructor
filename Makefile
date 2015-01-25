.PHONY:libs src tests
include common.mk
INCLUDE=-Iinclude
CPPFLAGS=$(_CPPFLAGS) $(INCLUDE)
SUBDIRS=libs src tests
OBJS=
TARGETS=file_structor.a
all: $(SUBDIRS) $(OBJS) $(TARGETS)
file_structor.a:
	cp src/file_structor.a file_structor.a
libs:
	$(MAKE) -C libs
src:
	$(MAKE) -C src
tests:
	$(MAKE) -C tests
clean:
	$(RM) $(RM_FLAGS) $(OBJS) $(TARGETS)
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
