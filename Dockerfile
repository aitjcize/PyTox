FROM ubuntu:trusty

RUN sudo apt-get update
RUN sudo apt-get install -y git wget make libtool automake pkg-config python-pip

# test requirements
RUN sudo pip install tox

## python interpreters
RUN sudo apt-get install -y software-properties-common python-software-properties
RUN sudo add-apt-repository -y ppa:fkrull/deadsnakes
RUN sudo apt-get update
RUN sudo apt-get install -y python3.3 python3.3-dev python3.2 python3.2-dev python2.6 python2.6-dev

# installing libsodium, needed for Core
RUN git clone https://github.com/jedisct1/libsodium.git
RUN cd libsodium && git checkout tags/0.7.0 && ./autogen.sh && ./configure --prefix=/usr && make && make install

# installing libopus, needed for audio encoding/decoding
RUN wget http://downloads.xiph.org/releases/opus/opus-1.0.3.tar.gz
RUN tar xzf opus-1.0.3.tar.gz
RUN cd opus-1.0.3 && ./configure && make && make install

# installing vpx
RUN apt-get install -y yasm
RUN git clone https://chromium.googlesource.com/webm/libvpx
RUN cd libvpx && ./configure --enable-shared && make && make install

# creating librarys' links and updating cache
RUN ldconfig
RUN git clone https://github.com/irungentoo/toxcore.git toxcore
RUN cd toxcore && autoreconf -i
RUN cd toxcore && ./configure --prefix=/usr --disable-tests --disable-ntox
RUN cd toxcore && make
RUN cd toxcore && make install

# PyTox
RUN sudo apt-get install -y python-dev
ADD pytox PyTox/pytox
ADD setup.py PyTox/setup.py
ADD examples PyTox/examples
ADD tests PyTox/tests
ADD MANIFEST.in PyTox/MANIFEST.in
ADD tox.ini PyTox/tox.ini
RUN cd PyTox && python setup.py install
