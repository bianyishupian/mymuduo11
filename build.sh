#!/bin/bash

set -e

# 如果没有build目录 创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -fr `pwd`/build/*
cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo       .so库拷贝到 /usr/lib
if [ ! -d /usr/include/mymuduo11 ]; then
    mkdir /usr/include/mymuduo11
fi

if [ ! -d /usr/include/mymuduo11/base ]; then
    mkdir /usr/include/mymuduo11/base
fi

if [ ! -d /usr/include/mymuduo11/net ]; then
    mkdir /usr/include/mymuduo11/net
fi

cd `pwd`/base
for header in `ls *.h`
do
    cp $header /usr/include/mymuduo11/base
done

cd ..

cd `pwd`/net
for header in `ls *.h`
do
    cp $header /usr/include/mymuduo11/net
done

cd ..

cp `pwd`/lib/libmymuduo11.so /usr/lib

ldconfig