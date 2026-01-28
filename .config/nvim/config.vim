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
Plug 'mbbill/undotree'
Plug 'unblevable/quick-scope'
Plug 'David-Kunz/gen.nvim'
Plug 'neovim/nvim-lspconfig'
Plug 'tpope/vim-commentary'
Plug 'morhetz/gruvbox'
Plug 'tweekmonster/startuptime.vim'
Plug 'f-person/git-blame.nvim'
Plug 'williamboman/mason.nvim', { 'tag': 'stable' }
Plug 'williamboman/mason-lspconfig.nvim', { 'tag': 'stable' }
Plug 'nvim-lua/plenary.nvim'
Plug 'nvim-telescope/telescope.nvim', { 'branch': '0.1.x' }
Plug 'nvim-treesitter/nvim-treesitter'
Plug 'junegunn/goyo.vim'
Plug 'kyoz/purify', { 'rtp': 'vim' }
Plug 'bignimbus/pop-punk.vim'
Plug 'folke/tokyonight.nvim'
Plug 'airblade/vim-gitgutter'
Plug 'f-person/auto-dark-mode.nvim'
call plug#end()

set cot=menuone,noinsert,noselect shm+=c
set bg=dark

" OPEN THE FILE WHERE YOU LEFT OF:
if has("autocmd")
    au BufReadPost * if line("'\"") > 1 && line("'\"") <= line("$") | exe "normal! g'\"" | 
endif
" BETTER BACKUP AND RESTORE MECHANISM:
let &directory = $XDG_DATA_HOME . '/nvim/dirs/tmp//'
set backup
let &backupdir = $XDG_DATA_HOME . '/nvim/dirs/backups//'
set undofile
let &undodir = $XDG_DATA_HOME . '/nvim/undodir//'

let g:yankring_history_dir = expand('$XDG_DATA_HOME') . 'nvim/dirs/'
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

nmap <leader>s :source ~/.config/nvim/init.vim<CR>
nmap <leader>i :LspInfo<CR>
" nmap <leader>l :lua vim.cmd('e'..vim.lsp.get_log_path())<CR>
nmap gc gcc
unmap gcc

set relativenumber
set number
" color elflord
set scrolloff=2 " Keeps cursor 3 lines away from screen border.
syntax enable
filetype plugin on
filetype indent on
set incsearch " Search as characters are entered.
set hlsearch " Highlight matches.
set showmatch " Highlight matching [{()}].
" set laststatus=2 " For powerline.
set path+=**
set wildmenu

" search and highlight but do not jump
nnoremap * :keepjumps normal! mi*`i<CR>
autocmd BufWritePost *sxhkdrc !pkill sxhkd; sleep 1; setsid sxhkd &


" Ag silversearcher
if executable('ag')
    set grepprg=ag\ --vimgrep\ $*
    set grepformat^=%f:%l:%c:%m
endif

command! Format execute 'lua vim.lsp.buf.formatting()'

function! ExecuteLeader(suffix)
  let l:leader = get(g:,"mapleader","\\")

  if l:leader == ' '
    let l:leader = '1' . l:leader
  endif

  execute "normal ".l:leader.a:suffix
endfunction
command! -nargs=1 NormLead call ExecuteLeader(<f-args>)

let g:gitblame_message_template = ' <author> • <date> • <summary> • <sha>'
let g:gitblame_date_format = '%r'

" Find files using Telescope command-line sugar.
nnoremap <leader>ff <cmd>Telescope find_files<cr>
nnoremap <leader>fg <cmd>Telescope live_grep<cr>
nnoremap <leader>fb <cmd>Telescope buffers<cr>
nnoremap <leader>fh <cmd>Telescope help_tags<cr>

" Using Lua functions
nnoremap <leader>ff <cmd>lua require('telescope.builtin').find_files()<cr>
nnoremap <leader>fg <cmd>lua require('telescope.builtin').live_grep()<cr>
nnoremap <leader>fb <cmd>lua require('telescope.builtin').buffers()<cr>
nnoremap <leader>fh <cmd>lua require('telescope.builtin').help_tags()<cr>
noremap <leader>c :Commentary<cr>

" if &diff
"     colorscheme tokyonight-night
" endif

:imap kj <Esc>
:imap jk <Esc>

color purify
let g:gitgutter_diff_base = 'origin/master'
