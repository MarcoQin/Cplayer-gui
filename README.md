# Cplayer-gui
Cplayer with GTK+ GUI.

####Lib Dependencies:

- **GTK+-3.0** `sudo apt-get install libgtk-3-dev`
- **SDL2.0**
- **ffmpeg:**
    - make and install the latest ffmpeg:
        - (maby need: sudo apt-get install yasm)
        - ./configure --enable-shared
        - make
        - sudo make install
        - if can't find lib*.so:

            sudo vi /etc/ld.so.conf
            add follow lines:
                include ld.so.conf.d/*.conf
                /usr/local/libevent-1.4.14b/lib
                /usr/local/lib

            then:
                sudo ldconfig

####Compile and install:

    make
    sudo make install  /* This will install the cplayer desktop app to your system */

####Screenshots:

![Cplayer-launch][launch]


![Cplayer-add-files][add-files]

![Cplayer-desktop-app][desktop-app]

[launch]: https://github.com/MarcoQin/gallery/blob/master/Cplayer-gui/launch.png
[add-files]: https://github.com/MarcoQin/gallery/blob/master/Cplayer-gui/add-files.png
[desktop-app]: https://github.com/MarcoQin/gallery/blob/master/Cplayer-gui/desktop-app.png
