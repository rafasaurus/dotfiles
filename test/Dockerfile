FROM archlinux:base-devel

RUN pacman -Syu --noconfirm --needed \
      git \
      stow \
      zsh \
      tmux \
      neovim \
      fzf \
      ripgrep \
      fd \
      make \
      gcc \
    && pacman -Scc --noconfirm

ENV DOTFILES=/root/dotfiles
WORKDIR /root/dotfiles

CMD ["/root/dotfiles/test/run-tests.sh"]
