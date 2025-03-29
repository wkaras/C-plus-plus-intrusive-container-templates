YR=14

rm -f a.out *.o

cc --std=c++${YR} -c crc32.cpp fnv_hash.cpp >| /tmp/xx 2>&1

for F in avl_ex1.cpp avl_ex2.cpp test_avl.cpp test_cq.cpp test_cq_lf.cpp \
test_hash.cpp test_list.cpp test_modulus.cpp test_util.cpp
do
    rm -f a.out *.o
    cc --std=c++${YR} $F -lstdc++ -lpthread
    ./a.out
done >> /tmp/xx 2>&1

rm -f a.out *.o

cc --std=c++${YR} test_hash_speed.cpp crc32.cpp fnv_hash.cpp -lm -lstdc++ \
  >> /tmp/xx 2>&1
./a.out 0 10000 >> /tmp/xx 2>&1

rm -f a.out *.o
