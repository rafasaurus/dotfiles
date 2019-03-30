VERSION=2.8
echo "***WARNING***: this script will override existing tmux config"
sudo apt-get -y remove tmux
sudo apt-get -y install wget tar libevent-dev libncurses-dev
wget https://github.com/tmux/tmux/releases/download/${VERSION}/tmux-${VERSION}.tar.gz
tar xf tmux-${VERSION}.tar.gz
rm -f tmux-${VERSION}.tar.gz
cd tmux-${VERSION}
./configure
make
sudo make install
cd -
sudo rm -rf /usr/local/src/tmux-*
sudo mv tmux-${VERSION} /usr/local/src
wget -O $HOME/.tmux.conf https://raw.githubusercontent.com/rafasaurus/config/master/dotfiles/.tmux.conf
