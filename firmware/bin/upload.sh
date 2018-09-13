#!/bin/sh
avrdude -c usbasp -p attiny44 -U flash:w:$1:i
avrdude -c usbtiny -p attiny44 -U lfuse:w:0xDF:m
avrdude -c usbtiny -p attiny44 -U hfuse:w:0xDF:m