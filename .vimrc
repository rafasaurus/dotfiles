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
" Plug 'srstevenson/vim-picker'
Plug 'tpope/vim-scriptease'
" cxiw to chan swap words, cxc to clear
Plug 'tommcdo/vim-exchange'
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
Plug 'mbbill/undotree'
Plug 'ChristianChiarulli/codi.vim'
" Plug 'neoclide/coc.nvim'
Plug 'junegunn/fzf', { 'do': { ->fzf#install() } }
Plug 'junegunn/fzf.vim'
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
command! MakeTagsCppFull !ctags -R --c++-kinds=+p --fields=+iaS --extra=+q /usr/include
command! MakeTagsPython !ctags -R --languages=python shellescape(system(python -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"))
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
autocmd BufRead ~/Dropbox/todo.txt :Goyo
autocmd VimLeave * call system("xsel -ib", getreg('+')) " Prevent Vim from clearing the clipboard on exit

" RUNING MAKE WITH PYTHON:
autocmd Filetype python set makeprg=python2\ %:S
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
autocmd BufWritePost *sxhkdrc !pkill sxhkd; sleep 1; setsid sxhkd &
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
set background=dark

" " QUICKSCOPE TRIGGER A HIGHLIGHT IN THE APPROPRIATE DIRECTION WHEN PRESSING THESE KEYS:
" let g:qs_highlight_on_keys = ['f', 'F', 't', 'T']

" CODI PLUGIN SETUP FOR REPL WITH VIM
highlight CodiVirtualText guifg=cyan
let g:codi#virtual_text_prefix = "❯ "


" ======== Ag command cheat sheet ========
"
"               e    to open file and close the quickfix window
"               o    to open (same as enter)
"               go   to preview file (open but maintain focus on ag.vim results)
"               t    to open in new tab
"               T    to open in new tab silently
"               h    to open in horizontal split
"               H    to open in horizontal split silently
"               v    to open in vertical split
"               gv   to open in vertical split silently
"               q    to close the quickfix window
"
"
" " ======== vim picker command mappings ========
" nmap <unique> <leader>pe <Plug>(PickerEdit)
" nmap <unique> <leader>ps <Plug>(PickerSplit)
" nmap <unique> <leader>pt <Plug>(PickerTabedit)
" nmap <unique> <leader>pv <Plug>(PickerVsplit)
" nmap <unique> <leader>pb <Plug>(PickerBuffer)
" nmap <unique> <leader>p] <Plug>(PickerTag)
" nmap <unique> <leader>pw <Plug>(PickerStag)
" nmap <unique> <leader>po <Plug>(PickerBufferTag)
" nmap <unique> <leader>ph <Plug>(PickerHelp)

let g:fzf_layout = { 'window': { 'width': 0.8, 'height': 0.8 } }
let $FZF_DEFAULT_OPTS='--reverse'
let &t_SI = "\e[6 q"
let &t_EI = "\e[2 q"

" Optionally reset the cursor on start:
augroup myCmds
au!
autocmd VimEnter * silent !echo -ne "\e[2 q"
augroup END

:imap kj <Esc>
:imap jk <Esc>

" todo.sh shortcuts
command! -range Done call MoveSelectedLinesToFile('~/Dropbox/done.txt')
command! -range Udone call MoveSelectedLinesToFile('~/Dropbox/todo.txt')
fun! MoveSelectedLinesToFile(filename)
    exec "'<,'>w! >>" . a:filename
    norm gvd
endfunc
let g:neovide_cursor_vfx_mode = "railgun"
" color gruvbox
