.PHONY:clean install
.PHONY:default
INCLUDE_LUA?=
INCLUDE_SKYNET?=
SHARED:=-fPIC --shared
CFLAGS=-g -O3 -Wall $(INCLUDE_LUA)
LUA_CLIB_PATH:=luaclib
TARGET:=$(LUA_CLIB_PATH)/skipset.so $(LUA_CLIB_PATH)/skiplist.so

default:$(TARGET)

$(LUA_CLIB_PATH) :
	@mkdir $(LUA_CLIB_PATH)

$(LUA_CLIB_PATH)/skipset.so: lua-skipset.c | $(LUA_CLIB_PATH)
	$(CC) -std=gnu99 $(CFLAGS) $(SHARED) $^ -o $@

$(LUA_CLIB_PATH)/skiplist.so: skiplist.c lua-skiplist.c | $(LUA_CLIB_PATH)
	$(CC) -std=gnu99 $(CFLAGS) $(SHARED) $^ -o $@

clean:
	$(RM) $(TARGET)
