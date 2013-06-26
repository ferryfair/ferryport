#!/bin/sh
# 
# File:   install.sh
# Author: satya gowtham kudupudi
#
# Created on 15 Apr, 2013, 2:34:27 PM
#

if which ffmpeg >/dev/null; then
    echo "ffmpeg installed."
    if ls /usr/bin/ffmpeg > /dev/null;then
            echo "ffmpeg is allowed to run as root."
    else
            echo "Allowing ffmpeg to run as root."
            sudo ln -s /usr/local/bin/ffmpeg /usr/bin/
    fi
else
    if ls /usr/local/bin/ffmpeg > /dev/null;then
        echo "ffmpeg installed. Allowing ffmpeg to be run as root."
        sudo ln -s /usr/local/bin/ffmpeg /usr/bin/
    else
        echo "Installing ffmpeg..."
        if which apt-get >/dev/null; then
            sudo apt-get install ffmpeg
        fi
        if which yum >/dev/null; then 
            sudo yum update    
            sudo yum erase ffmpeg x264 x264-devel
            sudo yum install autoconf automake gcc gcc-c++ git libtool make nasm pkgconfig wget zlib-devel
            
            wget http://www.tortall.net/projects/yasm/releases/yasm-1.2.0.tar.gz
            tar xzvf yasm-1.2.0.tar.gz
            cd yasm-1.2.0
            ./configure --prefix="$HOME/ffmpeg_build" --bindir="$HOME/bin"
            make
            sudo make install
            cd ..

            git clone git://git.videolan.org/x264
            cd x264
            ./configure --enable-static
            make
            sudo make install
            cd ..
            
            git clone --depth 1 git://github.com/mstorsjo/fdk-aac.git
            cd fdk-aac
            autoreconf -fiv
            ./configure --disable-shared
            make
            sudo make install
            cd ..
            
            wget http://downloads.sourceforge.net/project/lame/lame/3.99/lame-3.99.5.tar.gz
            tar xzvf lame-3.99.5.tar.gz
            cd lame-3.99.5
            ./configure --disable-shared --enable-nasm
            make
            sudo make install
            cd ..
            
            wget http://downloads.xiph.org/releases/ogg/libogg-1.3.0.tar.gz
            tar xzvf libogg-1.3.0.tar.gz
            cd libogg-1.3.0
            ./configure --disable-shared
            make
            sudo make install
            cd ..
            
            wget http://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.gz
            tar xzvf libtheora-1.1.1.tar.gz
            cd libtheora-1.1.1
            ./configure --disable-shared
            make
            sudo make install
            cd ..
            
            wget http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.gz
            tar xzvf libvorbis-1.3.3.tar.gz
            cd libvorbis-1.3.3
            ./configure --disable-shared
            make
            sudo make install
            cd ..
            
            git clone http://git.chromium.org/webm/libvpx.git
            cd libvpx
            ./configure
            make
            sudo make install
            cd ..
            
            git clone git://source.ffmpeg.org/ffmpeg
            cd ffmpeg
            ./configure --prefix="$HOME/ffmpeg_build" --extra-cflags="-I$HOME/ffmpeg_build/include" \
            --extra-ldflags="-L$HOME/ffmpeg_build/lib" --bindir="$HOME/bin" --extra-libs="-ldl" --enable-gpl \
            --enable-libass --enable-libfdk-aac --enable-libmp3lame --enable-libopus --enable-libtheora \
            --enable-libvorbis --enable-libvpx --enable-libx264 --enable-nonfree --enable-x11grab
            make
            sudo make install
            make distclean
            hash -r
        fi
        sudo ln -s /usr/local/bin/ffmpeg /usr/bin/
    fi
fi
if ls /usr/include/libxml2 > /dev/null;then
        echo "libxml2 devel installed."
else
    if which apt-get > /dev/null;then
        sudo apt-get install libxml2-dev
    fi
    if which yum > /dev/null;then
        sudo yum install libxml2-devel
    fi
fi

if which g++ > /dev/null;then
        echo "g++ devel installed."
else
    if which apt-get > /dev/null;then
        sudo apt-get install g++
    fi
    if which yum > /dev/null;then
        sudo yum install g++
    fi
fi