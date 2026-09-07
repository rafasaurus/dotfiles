-- Markdown checkbos helper keymap
local keymap = vim.keymap.set

-- Toggle Markdown Checkbox or Create Todo
keymap('n', '<C-l>', function()
  local line = vim.api.nvim_get_current_line()
  if line:match("%[ %]") then
    line = line:gsub("%[ %]", "[x]", 1)
  elseif line:match("%[x%]") then
    line = line:gsub("%[x%]", "[ ]", 1)
  else
    -- If it's already a list item (- text), insert the checkbox
    if line:match("^%s*%- ") then
      line = line:gsub("^(%s*%-) ", "%1 [ ] ")
    else
      -- Otherwise, prepend the bullet and checkbox, keeping indentation
      line = line:gsub("^(%s*)", "%1- [ ] ")
    end
  end
  vim.api.nvim_set_current_line(line)
end, { desc = "Toggle Markdown Checkbox" })
