# docker build --rm -f ./openexr-ht/Dockerfile -t openexr:latest .
# docker run -it --rm -v C:\\temp:/tmp/ openexr:latest
FROM ubuntu:jammy

RUN apt-get update

# disable interactive install 
ENV DEBIAN_FRONTEND noninteractive

# install developement tools
RUN apt-get -y install cmake
RUN apt-get -y install g++
RUN apt-get -y install git
RUN apt-get -y install unzip
RUN apt-get -y install libnuma-dev
RUN apt-get -y install python3

# install developement debugging tools
RUN apt-get -y install valgrind

# set Kakadu distribution version and unique serial number 
ARG KDU_SOURCE_NAME=v8_4_1-00462N
# set path to location of source zip, in this case its here ./v8_2_1-00462N.zip
ARG KDU_SOURCE_ZIP_DIRECTORY=./
WORKDIR /usr/src/kakadu
COPY $KDU_SOURCE_ZIP_DIRECTORY/$KDU_SOURCE_NAME.zip $KDU_SOURCE_NAME.zip
#COPY v8_4_1-00462N.zip /usr/src/kakadu.zip
RUN unzip $KDU_SOURCE_NAME.zip 
RUN rm -f $KDU_SOURCE_NAME.zip
# enable HTJ2K
WORKDIR /usr/src/kakadu/$KDU_SOURCE_NAME/
RUN mv srclib_ht srclib_ht_noopt; cp -r altlib_ht_opt srclib_ht
# compile Kakadu SDK and demo apps with HTJ2K enabled (#define FBC_ENABLED)
WORKDIR /usr/src/kakadu/$KDU_SOURCE_NAME/make
RUN make CXXFLAGS=-DFBC_ENABLED -f Makefile-Linux-x86-64-gcc all_but_jni
# set environment variables
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/src/kakadu/$KDU_SOURCE_NAME/lib/Linux-x86-64-gcc
ENV PATH=$PATH:/usr/src/kakadu/$KDU_SOURCE_NAME/bin/Linux-x86-64-gcc
ENV KDU_INCLUDE_DIR=/usr/src/kakadu/$KDU_SOURCE_NAME/managed/all_includes
ENV KDU_LIBRARY=/usr/src/kakadu/$KDU_SOURCE_NAME/lib/Linux-x86-64-gcc/libkdu_a84R.so

# build OpenEXR 
WORKDIR /usr/src/OpenEXR
COPY ./openexr-ht .
WORKDIR /usr/src/OpenEXR/build
RUN cmake .. -DKDU_INCLUDE_DIR=$KDU_INCLUDE_DIR -DKDU_LIBRARY=$KDU_LIBRARY
RUN make
RUN make install

# finalize docker environment



