CFLAGS=-Wall -O2 -ggdb
X11LDFLAGS=-lX11
all:x112acab tois tojson toxml

x112acab: x112acab.c
	$(CC) $(CFLAGS) $(X11LDFLAGS) $^ -o $@

clean:
	rm -f x112acab
	rm -f tois
	rm -f tojson
	rm -f toxml

