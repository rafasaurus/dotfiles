#!/bin/bash
sudo apt-get install curl vim exuberant-ctags git ack-grep
sudo pip install pep8 flake8 pyflakes isort yapf
cd ~/
git clone https://raw.githubusercontent.com/fisadev/fisa-vim-config/master/.vimrc
