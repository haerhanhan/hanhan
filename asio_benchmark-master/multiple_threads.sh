#!/bin/sh

#set -x

killall asio_test.exe
timeout=${timeout:-2}
bufsize=${bufsize:-2048}
nothreads=1

for nosessions in 10000; do
for nothreads in 1 2 3 4; do


  echo "======================> (test) Bufsize: $bufsize Threads: $nothreads Sessions: $nosessions"
  sleep 1
  ./asio_test.exe server 127.0.0.1 33333 $nothreads $bufsize & srvpid=$!
  sleep 1
  ./asio_test.exe client 127.0.0.1 33333 $nothreads $bufsize $nosessions $timeout
  sleep 1 

  kill -9 $srvpid
  sleep 1
  
done
done
