project2: project2.c libfdr/libfdr.a
	gcc -o $@ -w project2.c libfdr/libfdr.a
clean:
	rm -f project2