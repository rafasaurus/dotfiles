vim.api.nvim_command('source ~/.config/nvim/config.vim')
local nvim_lsp = require('lspconfig')

require('mason-config')
require('gen-llama')

require('telescope').setup{
  defaults = {
     vimgrep_arguments = {
        "rg",
        "--no-heading",
        "--line-number",
        "--column",
        "--smart-case",
        "--vimgrep",
        -- no-heading --line-number --column --smart-case --vimgrep  guided
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

-- vim.lsp.set_log_level("debug")
