# Cplayer-gui
Cplayer with a GTK+ GUI.

####Requirments:

- **mplayer:**
    - ubuntu: `sudo apt-get install mplayer`
    - others: [official site](http://www.mplayerhq.hu/design7/dload.html)

####Lib Dependencies:

- **GTK+-3.0** `sudo apt-get install libgtk-3-dev`

####Compile:

gcc `pkg-config --cflags gtk+-3.0` -o cplayer main.c `pkg-config --libs gtk+-3.0`
