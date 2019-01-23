" a-vim-config
let vim_plug_just_installed = 0
let vim_plug_path = expand('~/.vim/autoload/plug.vim')
if !filereadable(vim_plug_path)
    echo "Installing Vim-plug..."
    echo ""
    silent !mkdir -p ~/.vim/autoload
    silent !curl -fLo ~/.vim/autoload/plug.vim --create-dirs https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim
    let vim_plug_just_installed = 1
endif

if vim_plug_just_installed " manually load vim-plug the first time
    :execute 'source '.fnameescape(vim_plug_path)
endif

call plug#begin('~/.vim/plugged')

" Plug 'scrooloose/syntastic'
" Plug 'neomake/neomake'
Plug 'GGalizzi/cake-vim'
Plug 'airblade/vim-gitgutter'
Plug 'tpope/vim-surround'
Plug 'tpope/vim-repeat'
Plug 'easymotion/vim-easymotion'
Plug 'terryma/vim-multiple-cursors'
Plug 'itchyny/lightline.vim'
Plug 'tomtom/tcomment_vim'
Plug 'junegunn/fzf', { 'dir': '~/.fzf', 'do': './install --all' }
Plug 'junegunn/fzf.vim'
Plug 'mboughaba/i3config.vim'
Plug 'junegunn/goyo.vim'
Plug 'rking/ag.vim'
Plug 'dracula/vim'
" Plug 'pignacio/vim-yapf-format'

call plug#end()

if vim_plug_just_installed
    echo "Installing Bundles, please ignore key map error messages"
    :PlugInstall
endif

filetype plugin on
filetype indent on
autocmd VimLeave * call system("xsel -ib", getreg('+')) " Prevent Vim from clearing the clipboard on exit

" tabs and spaces handling
set expandtab
set tabstop=4
set softtabstop=4
set shiftwidth=4
set nocompatible " no vi-compatible
set incsearch " search as characters are entered
set hlsearch " highlight matches
set cursorline  " highlight current line
set showmatch " highlight matching [{()}]
syntax on
set scrolloff=2 " keep cursor 3 lines away from screen border
set wildmenu
set path+=** " search for every subdirectory
set laststatus=2 " for powerline
set noswapfile
set timeoutlen=300 " faster insert-normal switch
set ttimeoutlen=0

" ============== better backup, swap and undos storage ==============
set directory=~/.vim/dirs/tmp     " directory to place swap files in
set backup                        " make backup files
set backupdir=~/.vim/dirs/backups " where to put backup files
set undofile                      " persistent undos - undo after you re-open the file
set undodir=~/.vim/dirs/undos

let g:yankring_history_dir = '~/.vim/dirs/' " store yankring history file there too

if !isdirectory(&backupdir) " create needed directories if they don't exist
    call mkdir(&backupdir, "p")
endif
if !isdirectory(&directory)
    call mkdir(&directory, "p")
endif
if !isdirectory(&undodir)
    call mkdir(&undodir, "p")
endif
if has("autocmd") " to open the file where you left of
    au BufReadPost * if line("'\"") > 1 && line("'\"") <= line("$") | exe "normal! g'\"" | 
endif

map qq :run<CR>
map <silent> <C-q> <m-w>v
map <silent> <C-s> <c-w>s
map <silent> <C-h> <c-w>h
map <silent> <C-j> <c-w>j
map <silent> <C-k> <c-w>k
map <silent> <C-l> <c-w>l
map qq "+y " clipboard copy
map qw "+p
map <C-h> <Home>
map <C-l> <End> 
nnoremap tn :tabnew<Space>
nnoremap tk :tabnext<CR>
nnoremap tj :tabprev<CR>
nnoremap th :tabfirst<CR>
nnoremap tl :tablast<CR>
" fuzzy finder
map <leader>fz :Files<CR> 
" T-Comment
map <leader>c <c-_><c-_> 
map  <Leader>f <Plug>(easymotion-bd-f) " <Leader>f{char} to move to {char}
nmap <Leader>f <Plug>(easymotion-overwin-f)
nmap <Leader> <Plug>(easymotion-overwin-f2) " s{char}{char} to move to {char}{char}
map <Leader>l <Plug>(easymotion-bd-jk) " Move to line
nmap <Leader>l <Plug>(easymotion-overwin-line)
map  <Leader>w <Plug>(easymotion-bd-w) " Move to word
nmap <Leader>w <Plug>(easymotion-overwin-w)
highlight Comment ctermfg=green
map <leader>ag :Ag!
" e    to open file and close the quickfix window
" o    to open (same as enter)
" go   to preview file (open but maintain focus on ag.vim results)
" t    to open in new tab
" T    to open in new tab silently
" h    to open in horizontal split
" H    to open in horizontal split silently
" v    to open in vertical split
" gv   to open in vertical split silently
" q    to close the quickfix window

" let g:lightline = {'colorscheme': 'wombat'}
" let g:lightline = {'colorscheme': 'powerline'}
command! MakeTags !ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
" command! MakeTags !ctags -R --c++-kinds=+p --fields=+iaS --extra=+q /usr/include .
" command! MakeTags !ctags -R --c++-kinds=+p --fields=+iaS --extra=+q --fields=+l --languages=python --python-kinds=-iv -f ./tags
" call neomake#configure#automake('nrwi', 500)
" let g:neomake_python_enabled_makers = ['flake8']
:noremap <leader>u :w<Home>silent <End> !urlview<CR>&
color dracula
