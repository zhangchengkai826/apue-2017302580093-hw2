#!/bin/bash
set -x

P="/ /dev .. testdir checklist ls"

./ls -A $P
./ls -a $P
./ls -C $P
./ls -lc $P
./ls -d $P
./ls -F $P
./ls -f $P
./ls -lh $P
./ls -i $P
./ls -sk $P
./ls -l $P
./ls -n $P
./ls -q $P
./ls -R ..
./ls -r $P
./ls -lS $P
./ls -s $P
BLOCKSIZE=1024 ./ls -s $P
./ls -lt $P
TZ="New York" ./ls -lt $P
./ls -lu $P
./ls -w $P
./ls -x $P
./ls -1 $P

