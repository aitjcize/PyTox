FROM ubuntu:14.04
RUN sudo apt-get update \
  && sudo apt-get install -y nano python git wget make libtool automake pkg-config python-pip software-properties-common python-software-properties \
  # installing libsodium, needed for toxcore
  && git clone https://github.com/jedisct1/libsodium.git \
  && cd libsodium && git checkout tags/1.0.3 && ./autogen.sh && ./configure --prefix=/usr && make && make install && cd ../ && rm -rf libsodium \
  # installing libopus, needed for audio encoding/decoding
  && apt-get install -y libopus* \
  # installing vpx
  && apt-get install -y yasm libvpx* \
  # creating librarys' links and updating cache
  && ldconfig \
  && git clone https://github.com/TokTok/c-toxcore \
  && cd c-toxcore && autoreconf -i && mkdir _build && cd _build && ../configure  \
  && make -j4 && make install && cd .. && rm -rf c-toxcore \
  # PyTox
  && sudo apt-get install -y python-dev \
  && git clone https://github.com/aitjcize/PyTox \
  && cd PyTox && python setup.py install && cd .. && rm -rf PyTox
