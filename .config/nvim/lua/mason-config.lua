require('mason').setup()
require('mason-lspconfig').setup({
    ensure_installed = { }
})

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
    buf_set_keymap('n', 'gr',               '<cmd>lua vim.lsp.buf.references()<CR>', opts)
    buf_set_keymap('n', '<C-k>',            '<cmd>lua vim.lsp.buf.signature_help()<CR>', opts)
    buf_set_keymap('n', '<leader>wa',       '<cmd>lua vim.lsp.buf.add_workspace_folder()<CR>', opts)
    buf_set_keymap('n', '<leader>wr',       '<cmd>lua vim.lsp.buf.remove_workspace_folder()<CR>', opts)
    buf_set_keymap('n', '<leader>wl',       '<cmd>lua print(vim.inspect(vim.lsp.buf.list_workspace_folders()))<CR>', opts)
    buf_set_keymap('n', '<leader>D',        '<cmd>lua vim.lsp.buf.type_definition()<CR>', opts)
    buf_set_keymap('n', '<leader>rn',       '<cmd>lua vim.lsp.buf.rename()<CR>', opts)
    buf_set_keymap('n', '<leader>ta',       '<cmd>lua vim.lsp.buf.code_action()<CR>', opts)
    buf_set_keymap('n', '<leader>e',        '<cmd>lua vim.lsp.diagnostic.show_line_diagnostics()<CR>', opts)
    buf_set_keymap('n', '[d',               '<cmd>lua vim.lsp.diagnostic.goto_prev()<CR>', opts)
    buf_set_keymap('n', ']d',               '<cmd>lua vim.lsp.diagnostic.goto_next()<CR>', opts)
    buf_set_keymap('n', '<leader>q',        '<cmd>lua vim.lsp.diagnostic.set_loclist()<CR>', opts)
    buf_set_keymap('n', '<leader>f',        '<cmd>lua vim.lsp.buf.formatting()<CR>', opts)
end

vim.lsp.config('pylsp',  {
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
})

vim.lsp.config('cmake', {
	cmd = { "cmake-language-server" },
	filetypes = { "cmake" },
    root_dir = vim.fs.root(0, {"CMakePresets.json", "cmake"}),
	init_options = {
		buildDirectory = "build";
	}
})

vim.lsp.config('clangd', {
    on_attach = on_attach,
    cmd = { "clangd" },
    filetypes = { "c", "cpp", "objc", "objcpp" },
    root_dir = vim.fs.root(0, { "compile_commands.json", "build/compile_commands.json" }),
})

vim.lsp.enable('clangd')
vim.lsp.enable('cmake')
vim.lsp.enable('pylsp')
