#!/bin/env bash

mkdir -p /tmp/int_cont
export L=/tmp/int_cont/$$
echo $L

CC=gcc
YR=14
OPTS_BASE='-Wall -Wextra -pedantic -Wno-parentheses'
OPTS_PROD="$OPTS_BASE -O3 -DNDEBUG"
OPTS_TEST="$OPTS_BASE -O3"
OPTS_DBG="$OPTS_BASE -O0 -g"
OPTS="$OPTS_PROD"

rm -f a.out *.o

$CC $OPTS --std=c++${YR} -c crc32.cpp fnv_hash.cpp >| $L 2>&1

for F in avl_ex1.cpp avl_ex2.cpp test_avl.cpp test_cq.cpp test_cq_lf.cpp test_hash.cpp test_list.cpp test_modulus.cpp test_util.cpp
do
    rm -f a.out *.o
    $CC $OPTS --std=c++${YR} $F -lstdc++ -lpthread
    ./a.out
done >> $L 2>&1

rm -f a.out *.o

$CC $OPTS --std=c++${YR} test_hash_speed.cpp crc32.cpp fnv_hash.cpp -lm -lstdc++ >> $L 2>&1
./a.out 0 10000 >> $L 2>&1

rm -f a.out *.o

$CC $OPTS -std=c++17 test_ru_shared_mutex.cpp -lstdc++ >> $L 2>&1
./a.out >> $L 2>&1

rm -f a.out *.o

$CC $OPTS -std=c++17 test_ru_shared_mutex2.cpp ru_shared_mutex.cpp -lstdc++ >> $L 2>&1
./a.out >> $L 2>&1

rm -f a.out *.o

$CC $OPTS -std=c++17 test_ru_shared_mutex_speed.cpp ru_shared_mutex.cpp -lstdc++ >> $L 2>&1
./a.out >> $L 2>&1

rm -f a.out *.o

$CC $OPTS -std=c++17 test_ru_shared_mutex_speed2.cpp ru_shared_mutex.cpp -lstdc++ >> $L 2>&1
./a.out >> $L 2>&1

rm -f a.out *.o
