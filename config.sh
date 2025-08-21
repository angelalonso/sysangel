
# # Neovim
# # https://www.youtube.com/watch?v=TQn2hJeHQbM&list=PLep05UYkc6wTyBe7kPjQFWVXTlhKeQejM&index=1
# # REQUIREMENTS
# sudo apt install luarocks
# # BUILD
# git clone https://github.com/neovim/neovim
# cd neovim
# make CMAKE_BUILD_TYPE=RelWithDebInfo
# sudo make install
# mkdir -p ~/.config/nvim
# # CONFIG
# cp -R cfgfiles/nvim/* ~/.config/nvim/
wget https://github.com/LuaLS/lua-language-server/releases/download/3.15.0/lua-language-server-3.15.0-linux-x64.tar.gz
tar -xzf lua-language-server-3.15.0-linux-x64.tar.gz -C $HOME/Software/lua-language-server
cp ./00_files/usr_local_bin_lua-language-server $HOME/.local/bin/lua-language-server

