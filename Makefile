.PHONY:clean install
.PHONY:default
INCLUDE_LUA?=-I../../skynet/3rd/lua
INCLUDE_SKYNET?=
SHARED:=-fPIC --shared
CFLAGS=-g -O2 -Wall $(INCLUDE_LUA) $(INCLUDE_SKYNET)
LUA_CLIB_PATH:=luaclib
TARGET:=$(LUA_CLIB_PATH)/skiplist.so

default:$(TARGET)

$(LUA_CLIB_PATH) :
	@mkdir $(LUA_CLIB_PATH)

$(TARGET):lua-skiplist.c skiplist.c lua-skiplistsp.c skiplistsp.c | $(LUA_CLIB_PATH)
	$(CC) -std=gnu99 $(CFLAGS) $(SHARED) skiplist.c lua-skiplist.c skiplistsp.c lua-skiplistsp.c -o $@

clean:
	$(RM) $(TARGET)
