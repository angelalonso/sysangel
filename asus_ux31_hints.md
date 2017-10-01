# i915
https://wiki.archlinux.org/index.php/ASUS_Zenbook_UX31E#i915
Enabling i915_enable_rc6 will improve battery performance significantly. To enable it, add the following option to your kernel

sudo vim /etc/default/grub

change to
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash i915.i915_enable_rc6=1"

sudo update-grub

# SSD
https://help.ubuntu.com/community/AsusZenbook#SSD


