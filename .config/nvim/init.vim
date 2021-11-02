let nvim_plug_path = expand('~/.local/share/nvim/site/autoload/plug.vim')
let nvim_plug_just_installed = 0
if !filereadable(nvim_plug_path)
    echo "Installing Vim-plug..."
    echo ""

    silent !curl -fLo "${XDG_DATA_HOME:-$HOME/.local/share}"/nvim/site/autoload/plug.vim --create-dirs https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim
    nvim_plug_just_installed = 1
endif

if nvim_plug_just_installed " manually load nvim-plug the first time
    :execute 'source '.fnameescape(vim_plug_path)
endif
" 
call plug#begin()
Plug 'neovim/nvim-lspconfig'
Plug 'nvim-lua/completion-nvim'
Plug 'nvim-lua/diagnostic-nvim'
Plug 'honza/vim-snippets'
Plug '9mm/vim-closer'
Plug 'tpope/vim-commentary'
Plug 'lifepillar/gruvbox8'
Plug 'tweekmonster/startuptime.vim'
call plug#end()

" " CREATE THE TAGS FILE INSTALL CTAGS FIRST:
" command! MakeTags !ctags -R .
" command! MakeTagsCppFull !ctags -R --c++-kinds=+p --fields=+iaS --extra=+q /usr/include
" command! MakeTagsPython !ctags -R --languages=python "/usr/lib/python3.9/site-packages"
" " - Use ^] to jump to tag under cursor
" " - Use ^t to jump back up the tags stack
" " - Use ^t for ambigious tags
" 
" " SAVE XRESOURCES IN EVERY WRITE AND RELOAD:
" autocmd BufWritePost ~/.Xresources,~/.Xdefaults !xrdb %
" autocmd BufWritePost ~/.github/dwm !update-mime-database ~/.local/share/mime %
" autocmd BufRead ~/Dropbox/todo.txt :Goyo
" autocmd VimLeave * call system("xsel -ib", getreg('+')) " Prevent Vim from clearing the clipboard on exit
" 
" " RUNING MAKE WITH PYTHON:
" " autocmd Filetype python set makeprg=python2\ %:S
" " NOW WE CAN:
" "   -   Run :make
" "   -   :cl to list errors
" "   -   cc# to jump to error by number
" "   -   :cn and :cp to navigate forward and backward
" 
" 
" " RESIZE SPLIT:
" nnoremap <C-h> :vertical resize -5<cr>
" nnoremap <C-j> :resize +5<cr>
" nnoremap <C-k> :resize -5<cr>
" nnoremap <C-l> :vertical resize +5<cr>
" nnoremap <leader>h :split<Space>
" nnoremap <leader>v :vsplit<Space>
" nnoremap <leader>n :NERDTreeToggle<cr>
" nnoremap <leader><space> :noh<cr>
" nnoremap <leader><tab> :FZF<cr>
" nnoremap <leader>s  :FSHere<cr> " vim-fswitch
" 
" " CLIPBOARD COPY ALIAS:
" map qq "+
" map qw "+
" 
" " Update bindekeys when sxhdrc is updated
" autocmd BufWritePost ~/.Xresources,~/.Xdefaults !xrdb %
" set timeout 
" set timeoutlen=150
" 
" " set t_Co=256
" " highlight Normal ctermbg=NONE
" " highlight nonText ctermbg=NONE
" let b:ale_linters = ['flake8', 'pylint']
" " Disable warnings about trailing whitespace for Python files.
" let b:ale_warn_about_trailing_whitespace = 0
" 
" " MARKDOWN PREVIEWER PETTINGS
" " let vim_markdown_preview_github=1
" let vim_markdown_preview_hotkey='<C-m>'
" let vim_markdown_preview_browser='chromium'
" let vim_markdown_preview_use_xdg_open=1
" " let vim_markdown_preview_temp_file=1
" 
" set updatetime=850
" 
" set spell
" set spelllang=en_us
" set background=dark
" 
" " " QUICKSCOPE TRIGGER A HIGHLIGHT IN THE APPROPRIATE DIRECTION WHEN PRESSING THESE KEYS:
" " let g:qs_highlight_on_keys = ['f', 'F', 't', 'T']
" 
" " CODI PLUGIN SETUP FOR REPL WITH VIM
" highlight CodiVirtualText guifg=cyan
" let g:codi#virtual_text_prefix = "❯ "
" 
" let g:fzf_layout = { 'window': { 'width': 0.8, 'height': 0.8 } }
" let $FZF_DEFAULT_OPTS='--reverse'
" let &t_SI = "\e[6 q"
" let &t_EI = "\e[2 q"
" 
" " Optionally reset the cursor on start:
" augroup myCmds
" au!
" autocmd VimEnter * silent !echo -ne "\e[2 q"
" augroup END
" 
" " let g:ale_set_highlights = 0
" let g:fzf_tags_command = 'ctags -R'

set cot=menuone,noinsert,noselect shm+=c
set bg=dark
" OPEN THE FILE WHERE YOU LEFT OF:
if has("autocmd")
    au BufReadPost * if line("'\"") > 1 && line("'\"") <= line("$") | exe "normal! g'\"" | 
endif
" BETTER BACKUP AND RESTORE MECHANISM:
set directory=$XDG_DATA_HOME/nvim/dirs/tmp   " directory to place swap files in
set backup                              " make backup files
set backupdir=$XDG_DATA_HOME/nvim/dirs/backups       " where to put backup files
set undofile
set undodir=$XDG_DATA_HOME/nvim/undodir udf

let g:yankring_history_dir = '$XDG_DATA_HOME/nvim/dirs/' " store yankring history file there too
if !isdirectory(&backupdir) " create needed directories if they don't exist
    call mkdir(&backupdir, "p")
endif
if !isdirectory(&directory)
    call mkdir(&directory, "p")
endif
if !isdirectory(&undodir)
    call mkdir(&undodir, "p")
endif

" Tabs and Spaces
set expandtab
set tabstop=4
set softtabstop=4
set shiftwidth=4

" SHORTCUTS FOR TABS:
nnoremap tn :tabnew<Space>
nnoremap tk :tabnext<CR>
nnoremap tj :tabprev<CR>
nnoremap th :tabfirst<CR>
nnoremap tl :tablast<CR>

nmap <leader>c <Plug>CommentaryLine
set relativenumber
color gruvbox8
set scrolloff=2 " Keeps cursor 3 lines away from screen border.
syntax enable
filetype plugin on
filetype indent on
set incsearch " Search as characters are entered.
set hlsearch " Highlight matches.
set showmatch " Highlight matching [{()}].
set laststatus=2 " For powerline.
set nocompatible
set path+=**
set wildmenu


" search and highlight but do not jump
nnoremap * :keepjumps normal! mi*`i<CR>
autocmd BufWritePost *sxhkdrc !pkill sxhkd; sleep 1; setsid sxhkd &
