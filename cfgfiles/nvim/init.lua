require("config.lazy")

-- Editing
vim.opt.shiftwidth = 2

-- Look & Feel
vim.opt.number = true
vim.opt.relativenumber = true

vim.api.nvim_create_user_command('FindFiles', function()
  require('telescope.builtin').find_files()
end, {})
