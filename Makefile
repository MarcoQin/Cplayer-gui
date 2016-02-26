CC=gcc

GLIB_COMPILE_RESOURCES=glib-compile-resources

CFLAGS=`pkg-config --cflags gtk+-3.0` -O2

NAME=cplayer

ODIR=src

LIBS=`pkg-config --libs gtk+-3.0` -lpthread -ldl


_OBJ = db.o\
	utils.o\
	sqlite3/sqlite3.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

MAIN = $(patsubst %,$(ODIR)/%,main.c)

RESOURCE_NAME = $(ODIR)/$(NAME).gresource.xml

RESOURCE_TARGET = $(ODIR)/$(NAME).resources.c

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(NAME): $(OBJ)
	$(GLIB_COMPILE_RESOURCES) $(RESOURCE_NAME) --target=$(RESOURCE_TARGET) --sourcedir=$(ODIR) --generate-source
	$(CC) -o $@ $(RESOURCE_TARGET) $(MAIN) $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(NAME) $(RESOURCE_TARGET)

install:
	cp $(NAME) /usr/local/bin/

uninstall:
	rm -r /usr/local/bin/$(NAME)
