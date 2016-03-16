CC=gcc

GLIB_COMPILE_RESOURCES=glib-compile-resources

CFLAGS=`pkg-config --cflags gtk+-3.0` `sdl2-config --cflags` -O2

NAME=cplayer

ODIR=src

LIBS=`pkg-config --libs gtk+-3.0` `sdl2-config --libs` -lpthread -lavformat -lavcodec -lswscale -lavutil -lswresample -lz -ldl


_OBJ = db.o\
	player_core.o\
	popen2.o\
	utils.o\
	sqlite3/sqlite3.o\
	player_core/player.o\
	main.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

MAIN = $(patsubst %,$(ODIR)/%,main.c)

RESOURCE_NAME = $(ODIR)/$(NAME).gresource.xml

RESOURCE_TARGET = $(ODIR)/$(NAME).resources.c

ICON = $(ODIR)/$(NAME).png

DESKTOP = $(ODIR)/$(NAME).desktop

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

$(NAME): $(OBJ)
	$(GLIB_COMPILE_RESOURCES) $(RESOURCE_NAME) --target=$(RESOURCE_TARGET) --sourcedir=$(ODIR) --generate-source
	$(CC) -o $@ $(RESOURCE_TARGET) $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(NAME) $(RESOURCE_TARGET)

install:
	cp $(NAME) /usr/bin/
	xdg-icon-resource install --novendor --size 32 $(ICON) $(NAME)
	xdg-icon-resource install --novendor --size 48 $(ICON) $(NAME)
	xdg-icon-resource install --novendor --size 64 $(ICON) $(NAME)
	xdg-icon-resource install --novendor --size 128 $(ICON) $(NAME)
	cp $(DESKTOP) /usr/share/applications

uninstall:
	rm -r /usr/bin/$(NAME)
	rm -r /usr/share/applications/$(DESKTOP)
