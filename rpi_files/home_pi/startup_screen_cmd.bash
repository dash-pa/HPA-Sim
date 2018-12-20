#!/bin/bash

/home/pi/ble.py $1 > /home/pi/ble_$BASHPID.log 2> /home/pi/ble_e_$BASHPID.log
