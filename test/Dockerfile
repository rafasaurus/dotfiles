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
    && pacman -Scc --noconfirm

ENV DOTFILES=/dotfiles
WORKDIR /dotfiles

CMD ["/usr/local/bin/run-tests.sh"]
