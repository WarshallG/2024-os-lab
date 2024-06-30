#!/bin/bash

filename=./input/puf-qb-c3
steps=50
threads=5

make

# ./life $threads $steps $filename;

# ./life $steps $filename > a.txt ; echo end;
while (true) do  
    # ./life $threads $steps $filename > b.txt ; diff a.txt b.txt; echo 1;

    time ./life $steps $filename;
    time ./life $threads $steps $filename;
    echo end;
done

# 23334m  make-a  o0045-gun  o0075  puf-qb-c3