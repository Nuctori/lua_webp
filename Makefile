CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lsharpyuv -lwebp -lexampleutil -limagedec  -limageioutil -lwebpmux -lwebpdemux  -limageioutil -limageenc -lsharpyuv  -lpng -lz -ljpeg -ltiff -lm -lpthread
# LDFLAGS += -llua -lm
INCLUDES=-I./libwebp/examples -I./libwebp/imageio -I./libwebp/src -I./libwebp -I./lua-5.4.6/src -I.
LOADLIB=-L./libwebp/examples -L./libwebp/imageio -L./libwebp/src -L./libwebp/build -L./libwebp -L./lua-5.4.6/src -L.
OBJS=./libwebp/examples/example_util.c ./libwebp/imageio/image_dec.o ./libwebp/imageio/imageio_util.c ./libwebp/examples/unicode.h ./libwebp/src/webp/decode.h ./libwebp/src/webp/encode.h ./libwebp/src/dec/webp_dec.o ./libwebp/src/dec/idec_dec.o
EXEC=lua_webp.so

build: $(OBJS)
	$(CC) $(INCLUDES) lua_webp.c lua_cwebp.c lua_dwebp.c -O3 -fPIC --shared $(CFLAGS) $(OBJS) -o $(EXEC) $(LDFLAGS) $(LOADLIB)

$(OBJS):
	$(CC) -fPIC $(CFLAGS) $(INCLUDES) -c $*.c -o $@

clean:
	rm -f $(OBJS) $(EXEC)
