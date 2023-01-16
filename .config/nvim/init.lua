vim.api.nvim_command('source ~/.config/nvim/config.vim')
local nvim_lsp = require('lspconfig')

-- Use an on_attach function to only map the following keys
-- after the language server attaches to the current buffer
local on_attach = function(client, bufnr)
local function buf_set_keymap(...) vim.api.nvim_buf_set_keymap(bufnr, ...) end
local function buf_set_option(...) vim.api.nvim_buf_set_option(bufnr, ...) end

-- Enable completion triggered by <c-x><c-o>
buf_set_option('omnifunc', 'v:lua.vim.lsp.omnifunc')

-- Mappings.
local opts = { noremap=true, silent=true }

-- See `:help vim.lsp.*` for documentation on any of the below functions
buf_set_keymap('n', 'gD',               '<cmd>lua vim.lsp.buf.declaration()<CR>', opts)
buf_set_keymap('n', 'gd',               '<cmd>lua vim.lsp.buf.definition()<CR>', opts)
buf_set_keymap('n', 'K',                '<cmd>lua vim.lsp.buf.hover()<CR>', opts)
buf_set_keymap('n', 'gi',               '<cmd>lua vim.lsp.buf.implementation()<CR>', opts)
buf_set_keymap('n', '<C-k>',            '<cmd>lua vim.lsp.buf.signature_help()<CR>', opts)
buf_set_keymap('n', '<leader>wa',       '<cmd>lua vim.lsp.buf.add_workspace_folder()<CR>', opts)
buf_set_keymap('n', '<leader>wr',       '<cmd>lua vim.lsp.buf.remove_workspace_folder()<CR>', opts)
buf_set_keymap('n', '<leader>wl',       '<cmd>lua print(vim.inspect(vim.lsp.buf.list_workspace_folders()))<CR>', opts)
buf_set_keymap('n', '<leader>D',        '<cmd>lua vim.lsp.buf.type_definition()<CR>', opts)
buf_set_keymap('n', '<leader>rn',       '<cmd>lua vim.lsp.buf.rename()<CR>', opts)
buf_set_keymap('n', '<leader>ca',       '<cmd>lua vim.lsp.buf.code_action()<CR>', opts)
buf_set_keymap('n', 'gr',               '<cmd>lua vim.lsp.buf.references()<CR>', opts)
buf_set_keymap('n', '<leader>e',        '<cmd>lua vim.lsp.diagnostic.show_line_diagnostics()<CR>', opts)
buf_set_keymap('n', '[d',               '<cmd>lua vim.lsp.diagnostic.goto_prev()<CR>', opts)
buf_set_keymap('n', ']d',               '<cmd>lua vim.lsp.diagnostic.goto_next()<CR>', opts)
buf_set_keymap('n', '<leader>q',        '<cmd>lua vim.lsp.diagnostic.set_loclist()<CR>', opts)
buf_set_keymap('n', '<leader>f',        '<cmd>lua vim.lsp.buf.formatting()<CR>', opts)
end

local nvim_lsp = require('lspconfig')
local util = require 'lspconfig/util'

-- pip install python-lsp-ruff
nvim_lsp.pylsp.setup {
	settings = {
		pylsp = {
			plugins = {
				ruff = {
					enabled = true
				},
				pycodestyle = {
					enabled = false
				},
				pyflakes = {
					enabled = false
				},
				mccabe = {
					enabled = false
				}
			}
		}
	},
	on_attach = on_attach,
	plugins = {
		pydocstyle = { maxLineLength = 200, };
		flake8 = { maxLineLength = 200, };
	}
}

nvim_lsp.arduino_language_server.setup {
	cmd = {
		"arduino-language-server",
		"-cli-config", "$HOME/.arduino15/arduino-cli.yaml",
		"-fqbn", "arduino:avr:nano",
		"-cli", "arduino-cli",
		"-clangd", "clangd"
	},
}

nvim_lsp.cmake.setup {
	cmd = { "cmake-language-server" },
	filetypes = { "cmake" },
	root_dir = util.root_pattern('CMakePresets.json', 'CTestConfig.cmake', '.git', 'build', 'cmake'),
	init_options = {
		buildDirectory = "build";
	}
}

nvim_lsp.ccls.setup {
    on_attach = on_attach,
    cmd = { "ccls" } ,
    filetypes = { "c", "cpp", "objc", "objcpp" },
    root_dir = util.root_pattern("compile_commands.json", "build/compile_commands.json", "build/sitl/compile_commands.json", ".ccls", "compile_flags.txt", ".git"),
    init_options = {
        compilationDatabaseDirectory = "build";
        index = {
            threads = 0;
            };
        clang = {
            excludeArgs = { "-frounding-math"} ;
            };
        }
    }

require('telescope').setup{
  defaults = {
     vimgrep_arguments = {
        "ag",
        "--nocolor",
        "--noheading",
        "--numbers",
        "--column",
        "--smart-case",
        "--silent",
        "--vimgrep",
        "--path-to-ignore=./.agignore"
    }
  }
}
require "telescope".setup {
  pickers = {
    colorscheme = {
      enable_preview = true
    }
  }
}
vim.lsp.set_log_level("debug")
