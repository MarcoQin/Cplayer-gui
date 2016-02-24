# Cplayer-gui
Cplayer with a GTK+ GUI.

####Requirments:

- **mplayer:**
    - ubuntu: `sudo apt-get install mplayer`
    - others: [official site](http://www.mplayerhq.hu/design7/dload.html)

####Lib Dependencies:

- **GTK+-3.0** `sudo apt-get install libgtk-3-dev`

####Compile:

    gcc -o cplayer main.c $(pkg-config --cflags --libs gtk+-3.0)

####GThread example

    gcc -o thread_example thread_example.c $(pkg-config --cflags --libs gtk+-3.0 gthread-2.0)

####Screenshots:

![Cplayer-launch][launch]


![Cplayer-add-files][add-files]

[launch]: https://github.com/MarcoQin/gallery/blob/master/Cplayer-gui/launch.png
[add-files]: https://github.com/MarcoQin/gallery/blob/master/Cplayer-gui/add-files.png
