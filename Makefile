CC=gcc

all: libfdr.a project2

.PHONY: libfdr.a

libfdr.a:
	$(MAKE) -C libfdr
project2:
	${CC} -o $@ -w project2.c libfdr/libfdr.a
clean:
	rm -f project2
	$(MAKE) clean -C libfdr