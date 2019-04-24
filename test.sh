#!/bin/bash

P="/ /dev .. testdir checklist ls"

echo "./ls -A "$P
./ls -A $P
echo "./ls -a "$P
./ls -a $P
echo "./ls -C "$P
./ls -C $P
echo "./ls -lc "$P
./ls -lc $P
echo "./ls -d "$P
./ls -d $P
echo "./ls -F "$P
./ls -F $P
echo "./ls -f "$P
./ls -f $P
echo "./ls -lh "$P
./ls -lh $P
echo "./ls -i "$P
./ls -i $P
echo "./ls -sk "$P
./ls -sk $P
echo "./ls -l "$P
./ls -l $P
echo "./ls -n "$P
./ls -n $P
echo "./ls -q "$P
./ls -q $P
echo "./ls -R .."
./ls -R ..
echo "./ls -r "$P
./ls -r $P
echo "./ls -lS "$P
./ls -lS $P
echo "./ls -s "$P
./ls -s $P
echo "BLOCKSIZE=1024 ./ls -s "$P
BLOCKSIZE=1024 ./ls -s $P
echo "./ls -lt "$P
./ls -lt $P
echo "TZ=\"New York\" ./ls -lt "$P
TZ="New York" ./ls -lt $P
echo "./ls -lu "$P
./ls -lu $P
echo "./ls -w "$P
./ls -w $P
echo "./ls -x "$P
./ls -x $P
echo "./ls -1 "$P
./ls -1 $P

