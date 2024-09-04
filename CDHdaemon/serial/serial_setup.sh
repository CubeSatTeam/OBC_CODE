#!/bin/bash

#CHECK IF SCRIPT IS RUN WITH SUDO
if [ "$EUID" -eq 0 ]
  then echo "Script should NOT be called as sudo"
  exit
fi

#INSTALLING GCC, GIT AND MAKE
sudo apt install -y git make gcc

#enabling UART device (with consolle disabled)
sudo raspi-config nonint do_serial_hw 0 &&
sudo raspi-config nonint do_serial_cons 1 &&

#CLONING SERIAL PROTOCOL REPO
rm -r -f simpleDataLink &&
git clone https://github.com/shimo97/simpleDataLink &&
cd simpleDataLink &&
git submodule init &&
git submodule update &&

#COMPILING EVERYTHING
cd .. &&
make 
