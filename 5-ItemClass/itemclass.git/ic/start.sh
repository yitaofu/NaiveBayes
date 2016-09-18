#!/bin/bash

gcc -g -O2 -o ic ic.c ic_core.c ic_conf.c ../../libco/*.c ../../libco/url/*.c ../../libco/err/*.c -I./ -I../../libco -I../../libco/err -lcurl -lm -Wall
