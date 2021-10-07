#!/bin/bash

# make
./proc3 &
echo "initiated proc3"
./proc2 &
echo "initiated proc2"
time python3 proc1.py file.csv &
echo "initiated proc1"
