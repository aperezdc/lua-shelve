all: .stamp

.stamp: $(wildcard *.c) $(wildcard *.rockspec)
	luarocks make --pack-binary-rock
	touch $@

clean:
	$(RM) *.rock *.o *.so test.db .stamp

test: .stamp
	lua test.lua

.PHONY: test
