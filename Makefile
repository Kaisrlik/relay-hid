.PHONY: all clean

CFLAGS += $(shell pkg-config --cflags hidapi-libusb)
LIBDIR = /usr/lib/x86_64-linux-gnu/
LDFLAGS += -L $(LIBDIR) -Wl,-rpath $(LIBDIR) $(shell pkg-config --libs hidapi-libusb)

all: relay-hid

relay-hid: relay-hid.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install: relay-hid
	install -d $(DESTDIR)/usr/local/bin
	install -m 0755 relay-hid $(DESTDIR)/usr/local/bin

clean:
	rm -f $(TARGETS)
