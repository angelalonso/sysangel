return {
  {
    'nvim-telescope/telescope.nvim',
    tag = '0.1.8',
    dependencies = {
      'nvim-lua/plenary.nvim',
      { 'nvim-telescope/telescope-fzf-native.nvim', build = 'make' }
    },
    config = function()
      vim.keymap.set("n", "ff", require('telescope.builtin').find_files)
      vim.keymap.set("n", "en", function()
	local opts = require('telescope.themes').get_ivy({
	  cwd = vim.fn.stdpath("config")
	})
	require('telescope.builtin').find_files(opts)
      end)
    end
  }
}
