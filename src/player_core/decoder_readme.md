gcc main.c -lavformat -lavcodec -lswscale -lavutil -lz
gcc -o haha main.c -lavformat -lavcodec -lswscale -lavutil -lz `sdl2-config --cflags --libs`
gcc -o haha core.c -lavformat -lavcodec -lswscale -lavutil -lswresample -lz `sdl2-config --cflags --libs`
gcc -o haha core_test.c player.c -lavformat -lavcodec -lswscale -lavutil -lswresample -lz `sdl2-config --cflags --libs`

install ffmpeg:

    (maby need: sudo apt-get install yasm)
    ./configure --enable-shared
    make
    sudo make install

if can't find lib*.so:
    sudo vi /etc/ld.so.conf
    add follow lines:
        include ld.so.conf.d/*.conf
        /usr/local/libevent-1.4.14b/lib
        /usr/local/lib

    then:
        sudo ldconfig

