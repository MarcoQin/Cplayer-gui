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

resources = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(ODIR) --generate-dependencies $(ODIR)/$(NAME).gresource.xml)

$(ODIR)/$(NAME).resources.c: $(ODIR)/$(NAME).gresource.xml $(resources)
	$(GLIB_COMPILE_RESOURCES) $(ODIR)/$(NAME).gresource.xml --target=$@ --sourcedir=$(ODIR) --generate-source


$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(NAME): $(OBJ)
	gcc -o src/$@ $(MAIN) $^ $(CFLAGS) $(LIBS)
	mv src/$@ $@

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(NAME)

install:
	cp $(NAME) /usr/local/bin/

uninstall:
	rm -r /usr/local/bin/$(NAME)
