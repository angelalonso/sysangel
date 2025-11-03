require 'yaml'
pwd = File.expand_path File.dirname(__FILE__)
conf = YAML.load(File.read("#{pwd}/../config.yaml"))
homedir = "/home/#{conf['USER']}"


## APT PACKAGES
##############

# Update the package cache
apt_update 'update' do
  action :update
end

package (
conf['CHEF_PCKGS']
) do
    action :install
end


## DIRECTORIES
##############

directory "#{homedir}/Software" do
  owner conf['USER']
  group conf['USER']
  mode '0755'
  action :create
end
directory "#{homedir}/Software/Dev" do
  owner conf['USER']
  group conf['USER']
  mode '0755'
  action :create
end
directory "#{homedir}/Software/Work" do
  owner conf['USER']
  group conf['USER']
  mode '0755'
  action :create
end
directory "#{homedir}/.ssh" do
  owner conf['USER']
  group conf['USER']
  mode '0700'
  action :create
end

## Oh My ZSH
############
execute 'ohmyzsh_install' do
  command 'sh -c "$(wget https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh -O -)"'
  not_if {::File.directory?("#{homedir}/.oh-my-zsh") }
end

## Rust
############
# Install rustup
execute 'install rustup' do
  command 'curl --proto "=https" --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y'
  user conf['USER']
  environment ({ 'HOME' => "#{homedir}" }) # Set the home directory for the user
  not_if {::File.file?("#{homedir}/.cargo/bin/rustup") }
end
# Add Rust to the PATH
execute 'add rust to PATH' do
  command 'echo \'export PATH="$HOME/.cargo/bin:$PATH"\' >> $HOME/.zshrc'
  user conf['USER']
  environment ({ 'HOME' => "#{homedir}" }) # Set the home directory for the user
  not_if "grep -q \"HOME/.cargo/bin\" #{homedir}/.zshrc"
end

## Python3.11
############
# Install the prerequisites
package %w(software-properties-common) do
  action :install
end
# Add the deadsnakes PPA for Python 3.11
apt_repository 'deadsnakes-ppa' do
  uri 'ppa:deadsnakes/ppa'
  action :add
end
# Install Python 3.11
package 'python3.11' do
  action :install
end
# Install pip for Python 3.11
package 'python3-pip' do
  action :install
end
# Update the alternatives to set Python 3.11 as the default
execute "update-alternatives" do
  command "update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.11 1"
  action :run
end
# Update the default Python version
execute "update-alternatives-set" do
  command "update-alternatives --set python3 /usr/bin/python3.11"
  action :run
end

# Copy Secrets, keys...from mount volume on Chef.sh
# wine
# 
