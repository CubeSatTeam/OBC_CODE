#!/bin/bash

#CHECK IF SCRIPT IS RUN WITH SUDO
if [ "$EUID" -eq 0 ]
  then echo "Script should NOT be called as sudo"
fi

#ADDING TELEGRAF REPOSITORY (from https://www.influxdata.com/downloads/)
# influxdata-archive_compat.key GPG fingerprint:
#     9D53 9D90 D332 8DC7 D6C8 D3B9 D8FF 8E1F 7DF8 B07E

wget -q https://repos.influxdata.com/influxdata-archive_compat.key && 
echo '393e8779c89ac8d958f81f942f9ad7fb82a25e133faddaf92e15b16e6ac9ce4c influxdata-archive_compat.key' | sha256sum -c && cat influxdata-archive_compat.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/influxdata-archive_compat.gpg > /dev/null &&
echo 'deb [signed-by=/etc/apt/trusted.gpg.d/influxdata-archive_compat.gpg] https://repos.influxdata.com/debian stable main' | sudo tee /etc/apt/sources.list.d/influxdata.list &&
rm influxdata-archive_compat.key &&

#sudo apt update && sudo apt install telegraf && #INSTALLING TELEGRAF

#COPYING DEFINED ENVIRONMENTAL VARIABLES ON TELEGRAF ENV VARIABLES FILE
sudo cat telegrafVariables.txt > /etc/default/telegraf &&

#DISABLING DEFAULT TELEGRAF CONFIGURATION FILE
sudo mv /etc/telegraf/telegraf.conf /etc/telegraf/telegraf.conf.default &&
#COPYING CUSTOM TELEGRAF CONFIGURATION FILE
sudo cp telegraf.conf /etc/telegraf/telegraf.conf &&

sudo service telegraf start #STARTING TELEGRAF SERVICE

