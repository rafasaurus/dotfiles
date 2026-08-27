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

ENV DOTFILES=/dotfiles
WORKDIR /dotfiles

CMD ["/dotfiles/test/run-tests.sh"]
