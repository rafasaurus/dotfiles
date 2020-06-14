"mauimauer        _                    
" __   _(_)_ __ ___  _ __ ___ 
" \ \ / / | '_ ` _ \| '__/ __|
"  \ V /| | | | | | | | | (__ 
"   \_/ |_|_| |_| |_|_|  \___|
"                             
"
" For neovim
" mkdir -p ~/.local/share/nvim
" ln -s ~/.vim ~/.local/share/nvim/site 
" ln -s ~/.vimrc .config/nvim/init.vim
set encoding=UTF-8
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
Plug 'airblade/vim-gitgutter'
" Plug 'itchyny/lightline.vim'

" quickscope https://www.youtube.com/watch?v=EsGSwNySNMU
Plug 'unblevable/quick-scope'
Plug 'tomtom/tcomment_vim'
Plug 'tpope/vim-surround'
Plug 'tpope/vim-repeat'
Plug 'terryma/vim-multiple-cursors'
Plug 'junegunn/goyo.vim'
Plug 'coldfix/hexHighlight'
Plug 'scrooloose/nerdtree'
Plug 'dense-analysis/ale'
Plug 'rking/ag.vim'
Plug 'JamshedVesuna/vim-markdown-preview'
" WORKS WITH NERDTREE:
Plug 'ryanoasis/vim-devicons'
Plug 'junegunn/fzf'
Plug 'mbbill/undotree'
Plug 'ChristianChiarulli/codi.vim'
Plug 'neoclide/coc.nvim'
Plug 'gruvbox-community/gruvbox'
call plug#end()

" PLUGIN CONFIGS:
" let g:lightline = {'colorscheme': 'wombat'}
" let g:syntastic_python_checkers = ['pylint']
" T-Comment
map <leader>c <c-_><c-_> 

" Tabs and Spaces
set expandtab
set tabstop=4
set softtabstop=4
set shiftwidth=4
" ENABLE THE CURRENT MILLENIUM
set nocompatible
" SEARCH FOR EVERY SUBDIRECTORY
set path+=**
set wildmenu

syntax enable
" DEFAULT FILE BROWSER
filetype plugin on
filetype indent on
set incsearch " search as characters are entered
set hlsearch " highlight matches
set showmatch " highlight matching [{()}]
set scrolloff=2 " keep cursor 3 lines away from screen border
set laststatus=2 " for powerline

" CREATE THE TAGS FILE INSTALL CTAGS FIRST:
command! MakeTags !ctags -R .
" - Use ^] to jump to tag under cursor
" - Use ^t to jump back up the tags stack
" - Use ^t for ambigious tags

" SHORTCUTS FOR TABS:
nnoremap tn :tabnew<Space>
nnoremap tk :tabnext<CR>
nnoremap tj :tabprev<CR>
nnoremap th :tabfirst<CR>
nnoremap tl :tablast<CR>

" SAVE XRESOURCES IN EVERY WRITE AND RELOAD:
autocmd BufWritePost ~/.Xresources,~/.Xdefaults !xrdb %
autocmd BufWritePost ~/.github/dwm !update-mime-database ~/.local/share/mime %
autocmd BufRead ~/Dropbox/todo.md :Goyo
autocmd VimLeave * call system("xsel -ib", getreg('+')) " Prevent Vim from clearing the clipboard on exit

" RUNING MAKE WITH PYTHON:
autocmd Filetype python set makeprg=python3\ %:S
" NOW WE CAN:
"   -   Run :make
"   -   :cl to list errors
"   -   cc# to jump to error by number
"   -   :cn and :cp to navigate forward and backward

" BETTER BACKUP AND RESTORE MECHANISM:
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

" OPEN THE FILE WHERE YOU LEFT OF:
if has("autocmd")
    au BufReadPost * if line("'\"") > 1 && line("'\"") <= line("$") | exe "normal! g'\"" | 
endif
" RESIZE SPLIT:
nnoremap <C-h> :vertical resize -5<cr>
nnoremap <C-j> :resize +5<cr>
nnoremap <C-k> :resize -5<cr>
nnoremap <C-l> :vertical resize +5<cr>

nnoremap <leader>n :NERDTreeToggle<cr>
nnoremap <leader><space> :noh<cr>
nnoremap <leader><tab> :FZF<CR>

" CLIPBOARD COPY ALIAS:
map qq "+
map qw "+

set number
set relativenumber
" Update bindekeys when sxhdrc is updated
autocmd BufWritePost *sxhkdrc !pkill sxhkd; setsid sxhkd &
autocmd BufWritePost ~/.Xresources,~/.Xdefaults !xrdb %
set timeout 
set timeoutlen=150

" set t_Co=256
" set background=dark
" highlight Normal ctermbg=NONE
" highlight nonText ctermbg=NONE
let b:ale_linters = ['flake8', 'pylint']
" Disable warnings about trailing whitespace for Python files.
let b:ale_warn_about_trailing_whitespace = 0

" Use K to show documentation in preview window
nnoremap <silent> K :call <SID>show_documentation()<CR>

" MARKDOWN PREVIEWER PETTINGS
" let vim_markdown_preview_github=1
let vim_markdown_preview_hotkey='<C-m>'
let vim_markdown_preview_browser='chromium'
let vim_markdown_preview_use_xdg_open=1
" let vim_markdown_preview_temp_file=1

set updatetime=850

set spell
set spelllang=en_us
color gruvbox
set background=light

" " QUICKSCOPE TRIGGER A HIGHLIGHT IN THE APPROPRIATE DIRECTION WHEN PRESSING THESE KEYS:
" let g:qs_highlight_on_keys = ['f', 'F', 't', 'T']

" CODI PLUGIN SETUP FOR REPL WITH VIM
highlight CodiVirtualText guifg=cyan
let g:codi#virtual_text_prefix = "❯ "
