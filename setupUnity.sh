#!/bin/bash

sudo apt update
sudo apt update
sudo apt upgrade
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
    tmux\
    tilda\
    slack\
    python3-dev\
    python3-tk\
    python3-pip\
    msr-tools\
    tilda\
    python-pip\
    lm-sensors\
    gparted\


# telegram
sudo add-apt-repository -y ppa:atareao/telegram
sudo apt-get update
sudo apt-get install -y telegram

# papirus icons
sudo add-apt-repository -y ppa:papirus/papirus
sudo apt-get update
sudo apt-get install -y papirus-icon-theme

# python3
sudo pip3 install seaborn numpy pandas sklearn
sudo pip install undervolt

# chrome
wget -q -O - https://dl-ssl.google.com/linux/linux_signing_key.pub | sudo apt-key add -
echo 'deb [arch=amd64] http://dl.google.com/linux/chrome/deb/ stable main' | sudo tee /etc/apt/sources.list.d/google-chrome.list
sudo apt-get update
sudo apt-get install -y google-chrome-stable

# dropbox
sudo apt-key adv --keyserver pgp.mit.edu --recv-keys 5044912E
sudo sh -c 'echo "deb http://linux.dropbox.com/ubuntu/ xenial main" >> /etc/apt/sources.list.d/dropbox.list'
sudo apt-get update 
sudo apt-get install -y dropbox 

# redshift-gtk
sudo apt-get install -y redshift-gtk

# oomox theme generator
sudo add-apt-repository -y ppa:nilarimogard/webupd8
sudo apt update
sudo apt install -y oomox

# qdirstat
sudo add-apt-repository -y ppa:nathan-renniewaldock/qdirstat
sudo apt-get update
sudo apt-get install -y qdirstat
# sublime
sudo add-apt-repository -y ppa:webupd8team/sublime-text-3
sudo apt-get update
sudo apt-get install -y sublime-text-installer
# clean
sudo apt-get -y autoremove
