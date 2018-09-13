#!/bin/sh
make clean &&
make hex &&
avrdude -c usbasp -p attiny44 -U flash:w:main.hex:i
avrdude -c usbtiny -p attiny44 -U lfuse:w:0xDF:m
avrdude -c usbtiny -p attiny44 -U hfuse:w:0xDF:m