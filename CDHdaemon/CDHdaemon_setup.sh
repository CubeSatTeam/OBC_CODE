#!/bin/bash

#CHECK IF SCRIPT IS RUN WITH SUDO
if [ "$EUID" -eq 0 ]
  then echo "Script should NOT be called as sudo"
  
fi

#enabling i2c device
sudo raspi-config nonint do_i2c 0 &&

#installing i2c python package
sudo apt install python3-smbus2 &&

#executing serial library makefile
cd serial &&
./serial_setup.sh &&
cd .. &&

#parsing messages
cd messages &&
./parseMessages.py &&
cd .. &&

#creating CDH service file
echo [Unit] > CDH.service &&
echo Description=CDH daemon service >> CDH.service &&
echo >> CDH.service &&
echo [Service] >> CDH.service &&
echo User=$USER >> CDH.service &&
echo WorkingDirectory=$PWD >> CDH.service &&
echo ExecStart=$PWD/CDHdaemon.py >> CDH.service &&
echo Restart=always >> CDH.service &&
echo >> CDH.service &&
echo [Install] >> CDH.service &&
echo WantedBy=multi-user.target >> CDH.service &&

#copying service file on the right place
sudo cp CDH.service /etc/systemd/system/CDH.service &&

#gicing CDHdaemon.py execution permission just to be sure
chmod +x CDHdaemon.py &&

#enabling service
sudo systemctl enable CDH.service &&

#creating client alias
#creating .bash_aliases if not existing
if [ ! -f "$HOME/.bash_aliases" ]; then
    touch "$HOME/.bash_aliases"
fi

#delete existing alias if already defined
sed -i '/alias CDHcli/d' $HOME/.bash_aliases &&
#adding alias
echo alias CDHcli=$PWD/client.py >> $HOME/.bash_aliases &&
echo "Created alias CDHcli for client (active after reboot)"

echo "Done. NOW YOU NEED TO REBOOT PI!"
