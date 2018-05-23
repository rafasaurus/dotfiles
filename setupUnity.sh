#!/bin/bash
sudo apt-get install -y htop \
	git \
	vim \
	curl \
	indicator-multiload \
	indicator-cpufreq \
	synapse \
	plank \
	numix-gtk-theme \
	unity-tweak-tool \
	compizconfig-settings-manager \
    openssh-server\

# telegram
sudo add-apt-repository -y ppa:atareao/telegram
sudo apt-get update
sudo apt-get install telegram

# papirus icons
sudo add-apt-repository -y ppa:papirus/papirus
sudo apt-get update
sudo apt-get install papirus-icon-theme

# python3
sudo apt-get install -y python3-dev python3-tk python3-pip
sudo pip3 install seaborn numpy pandas sklearn

# chrome
wget -q -O - https://dl-ssl.google.com/linux/linux_signing_key.pub | sudo apt-key add -
echo 'deb [arch=amd64] http://dl.google.com/linux/chrome/deb/ stable main' | sudo tee /etc/apt/sources.list.d/google-chrome.list
sudo apt-get update
sudo apt-get install google-chrome-stable

# dropbox
sudo apt-key adv --keyserver pgp.mit.edu --recv-keys 5044912E
sudo sh -c 'echo "deb http://linux.dropbox.com/ubuntu/ xenial main" >> /etc/apt/sources.list.d/dropbox.list'
sudo apt-get update 
sudo apt-get install -y dropbox 

# redshift-gtk
sudo apt-get install -y redshift-gtk

# clean
sudo apt-get autoremove
