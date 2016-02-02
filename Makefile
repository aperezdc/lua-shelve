
T = .
include $T/Rules.mk

C_SRCS		= shelve.c marshal.c
C_OBJS 		= $(C_SRCS:.c=.o)
C_DEPS		= $(C_SRCS:.c=.d)
LIBNAME		= shelve
SLIB			= $(LIB_PREFIX)$(LIBNAME)$(STATICLIB_SUFFIX)
DLIB			= $(LIBNAME)$(SHLIB_SUFFIX)
PLIBS	 	  = -lgdbm -llua -llualib


all: $(SLIB) $(DLIB)

$(C_DEPS): $(C_SRCS)
-include $(C_DEPS)

$(SLIB): $(C_OBJS)
$(DLIB): $(C_OBJS)
	$(PLUG_LINK_CMD)

distclean: clean
	$(RM) $(C_DEPS)

clean:
	$(RM) $(SLIB) $(DLIB) $(C_OBJS)
	$(RM) test.db

test: test.lua $(DLIB)
	OS=$(OS) lua test.lua

.PHONY: test

