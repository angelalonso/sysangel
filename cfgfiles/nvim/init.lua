require("config.lazy")

-- Editing
vim.opt.tabstop = 2
vim.opt.expandtab = true
vim.opt.shiftwidth = 2

-- Look & Feel
vim.opt.number = true
vim.opt.relativenumber = true

vim.api.nvim_create_user_command('FindFiles', function()
	require('telescope.builtin').find_files()
end, {})

-- LSP configs
-- Define the custom on_attach function
local on_attach = function(client, bufnr)
	client.server_capabilities.documentFormattingProvider = false
	client.server_capabilities.documentRangeFormattingProvider = false

	vim.api.nvim_buf_create_user_command(bufnr, 'LspFormat', function(_)
		vim.lsp.buf.format({
			filter = function(server) return server.name == client.name end,
			async = true,
			opts = { indent_width = vim.opt.shiftwidth:get() } -- Reuses your shiftwidth=2
		})
	end, { desc = 'Format current buffer with LSP' })
end

require('lspconfig').lua_ls.setup({ on_attach = on_attach })
require('lspconfig').pyright.setup({ on_attach = on_attach })
