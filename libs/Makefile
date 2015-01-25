.PHONY:
include ../common.mk
INCLUDE=-I../include
CPPFLAGS=$(_CPPFLAGS) $(INCLUDE)
GIT_CMD=git clone
SUBDIRS=
OBJS=
TARGETS=CommonC ./commonc.a ../include/data_structs.h ../include/debug_assert.h ../include/logger.h
all: $(SUBDIRS) $(OBJS) $(TARGETS)
CommonC:
	$(GIT_CMD) https://github.com/yujokang/CommonC.git
./commonc.a: CommonC
	$(MAKE) -C CommonC
	cp CommonC//commonc.a ./commonc.a
../include/data_structs.h: CommonC
	cp CommonC/include/data_structs.h ../include/data_structs.h
../include/debug_assert.h: CommonC
	cp CommonC/include/debug_assert.h ../include/debug_assert.h
../include/logger.h: CommonC
	cp CommonC/include/logger.h ../include/logger.h
clean:
	$(RM) $(RM_FLAGS) $(OBJS) $(TARGETS)
