" a-vim-config
" http://fisadev.github.io/fisa-vim-config/

let vim_plug_just_installed = 0
let vim_plug_path = expand('~/.vim/autoload/plug.vim')
if !filereadable(vim_plug_path)
    echo "Installing Vim-plug..."
    echo ""
    silent !mkdir -p ~/.vim/autoload
    silent !curl -fLo ~/.vim/autoload/plug.vim --create-dirs https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim
    let vim_plug_just_installed = 1
endif

" manually load vim-plug the first time
if vim_plug_just_installed
    :execute 'source '.fnameescape(vim_plug_path)
endif

call plug#begin('~/.vim/plugged')

" use jedi plugin for python find definition
" Plug 'scrooloose/syntastic'
Plug 'tpope/vim-surround'
Plug 'tpope/vim-repeat'
Plug 'easymotion/vim-easymotion'
Plug 'terryma/vim-multiple-cursors'
Plug 'itchyny/lightline.vim'
Plug 'tomtom/tcomment_vim'
Plug 'junegunn/fzf', { 'dir': '~/.fzf', 'do': './install --all' }
Plug 'junegunn/fzf.vim'
" Plug 'pignacio/vim-yapf-format'

" Tell vim-plug we finished declaring plugins, so it can load them
call plug#end()

if vim_plug_just_installed
    echo "Installing Bundles, please ignore key map error messages"
    :PlugInstall
endif

" no vi-compatible
set nocompatible

" allow plugins by file type (required for plugins!)
filetype plugin on
filetype indent on

" tabs and spaces handling
set expandtab
set tabstop=4
set softtabstop=4
set shiftwidth=4

" always show status bar
set ls=2

" incremental search
set incsearch
" highlighted search results
set hlsearch

" syntax highlight on
syntax on

" when scrolling, keep cursor 3 lines away from screen border
set scrolloff=2

" better backup, swap and undos storage
set directory=~/.vim/dirs/tmp     " directory to place swap files in
set backup                        " make backup files
set backupdir=~/.vim/dirs/backups " where to put backup files
set undofile                      " persistent undos - undo after you re-open the file
set undodir=~/.vim/dirs/undos
set viminfo+=n~/.vim/dirs/viminfo
" store yankring history file there too
let g:yankring_history_dir = '~/.vim/dirs/'

" create needed directories if they don't exist
if !isdirectory(&backupdir)
    call mkdir(&backupdir, "p")
endif
if !isdirectory(&directory)
    call mkdir(&directory, "p")
endif
if !isdirectory(&undodir)
    call mkdir(&undodir, "p")
endif

let g:ctrlp_working_path_mode = 0
" ignore these files and folders on file finder
let g:ctrlp_custom_ignore = {
            \ 'dir':  '\v[\/](\.git|\.hg|\.svn|node_modules)$',
            \ 'file': '\.pyc$\|\.pyo$',
            \ }

" to open the file where you left of
if has("autocmd")
    au BufReadPost * if line("'\"") > 1 && line("'\"") <= line("$") | exe "normal! g'\"" | 
endif

map qq :run<CR>
map <silent> <C-q> <m-w>v
map <silent> <C-s> <c-w>s
map <silent> <C-h> <c-w>h
map <silent> <C-j> <c-w>j
map <silent> <C-k> <c-w>k
map <silent> <C-l> <c-w>l
colorscheme ron

" Jedi-vim ------------------------------
" All these mappings work only for python code:

" T-Comment plugin - press "\" + c to comment/uncoumment
map <leader>c <c-_><c-_>

" ###--- Easy Motion Setup ---###
" <Leader>f{char} to move to {char}
map  <Leader>f <Plug>(easymotion-bd-f)
nmap <Leader>f <Plug>(easymotion-overwin-f)

" s{char}{char} to move to {char}{char}
nmap <Leader> <Plug>(easymotion-overwin-f2)

" Move to line
map <Leader>l <Plug>(easymotion-bd-jk)
nmap <Leader>l <Plug>(easymotion-overwin-line)

" Move to word
map  <Leader>w <Plug>(easymotion-bd-w)
nmap <Leader>w <Plug>(easymotion-overwin-w)
" ###---###################---###

" Multi-Cursor https://github.com/terryma/vim-multiple-cursors
" Default mapping
let g:multi_cursor_start_word_key      = '<C-n>'
let g:multi_cursor_select_all_word_key = '<A-n>'
let g:multi_cursor_start_key           = 'g<C-n>'
let g:multi_cursor_select_all_key      = 'g<A-n>'
let g:multi_cursor_next_key            = '<C-n>'
let g:multi_cursor_prev_key            = '<C-p>'
let g:multi_cursor_skip_key            = '<C-x>'
let g:multi_cursor_quit_key            = '<Esc>'

highlight Comment ctermfg=green
set timeoutlen=300
set ttimeoutlen=0

" tab navigation mappings
nnoremap tn :tabnew<Space>
nnoremap tk :tabnext<CR>
nnoremap tj :tabprev<CR>
nnoremap th :tabfirst<CR>
nnoremap tl :tablast<CR>
map qq "+y
map qw "+p
map <C-h> <Home>
map <C-l> <End> 

command! MakeTags !ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
" command! MakeTags !ctags -R --c++-kinds=+p --fields=+iaS --extra=+q /usr/include .
" command! MakeTags !ctags -R --c++-kinds=+p --fields=+iaS --extra=+q --fields=+l --languages=python --python-kinds=-iv -f ./tags

set path+=** " search for every subdirectory
set wildmenu
" fuzzy finder
map <leader>fz :Files<CR>

let g:lightline = {
            \ 'colorscheme': 'powerline',
            \ }
