# i915
https://wiki.archlinux.org/index.php/ASUS_Zenbook_UX31E#i915
Enabling i915_enable_rc6 will improve battery performance significantly. To enable it, add the following option to your kernel

sudo vim /etc/default/grub

change to
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash i915.i915_enable_rc6=1"

sudo update-grub

# SSD
https://help.ubuntu.com/community/AsusZenbook#SSD

change to
UUID=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx /               ext4    discard,noatime,errors=remount-ro 0       1

# Suspend to RAM
https://wiki.archlinux.org/index.php/ASUS_Zenbook_UX31E#Suspend_to_RAM

sudo vim /etc/pm/config.d/unload_module

add
SUSPEND_MODULES="xhci_hcd ehci_hcd uhci_hcd"
