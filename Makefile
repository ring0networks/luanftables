LUA_VERSION ?= $(shell for v in 5.5 5.4; do pkg-config --exists lua$$v 2>/dev/null && echo $$v && break; done)

LUA_INC  ?= $(shell pkg-config --cflags lua$(LUA_VERSION) 2>/dev/null || echo -I/usr/include/lua$(LUA_VERSION))
NFT_INC  ?= $(shell pkg-config --cflags libnftables 2>/dev/null)
NFT_LIB  ?= $(shell pkg-config --libs libnftables 2>/dev/null || echo -lnftables)
CFLAGS   ?= -O2 -Wall -Wextra -fPIC
LDFLAGS  ?= -shared

LUA_CMOD ?= /usr/local/lib/lua/$(LUA_VERSION)

nftables.so: luanftables.c
	$(CC) $(CFLAGS) $(LUA_INC) $(NFT_INC) -o $@ $< $(LDFLAGS) $(NFT_LIB)

install: nftables.so
	install -D -m 0755 $< $(DESTDIR)$(LUA_CMOD)/$<

clean:
	rm -f nftables.so

.PHONY: install clean

