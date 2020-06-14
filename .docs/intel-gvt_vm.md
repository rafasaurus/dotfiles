### In order to intel_GVT work 
[main doc]
https://wiki.archlinux.org/index.php/Intel_GVT-g
[original guide]
https://www.reddit.com/r/VFIO/comments/8h352p/guide_running_windows_via_qemukvm_and_intel_gvtg/

[useful article]
https://computingforgeeks.com/complete-installation-of-kvmqemu-and-virt-manager-on-arch-linux-and-manjaro/


### Make a virtual gpu
generate uuid wtih uuidgen command
sudo /bin/sh -c "echo [My-vGPU-UUID] > /sys/devices/pci0000:00/0000:[My-PCI-Device]/mdev_supported_types/[My-i915-GVTg-Type]/create"

Create /etc/systemd/system/gvtvgpu.service
"""
[Unit]
Description=Create Intel GVT-g vGPU

[Service]
Type=oneshot
ExecStart=/bin/sh -c "echo '62b5e1eb-1193-4ea8-9e22-5df5a5bc366a' > /sys/devices/pci0000:00/0000:00:02.0/mdev_supported_types/i915-GVTg_V5_4/create"
ExecStop=/bin/sh -c "echo '1' > /sys/devices/pci0000:00/0000:02/62b5e1eb-1193-4ea8-9e22-5df5a5bc366a/remove"
RemainAfterExit=yes

[Install]
WantedBy=graphical.target
"""
where 62b5e1eb-1193-4ea8-9e22-5df5a5bc366a is uuid
i914-GVTg-V5_4 is vgpu device type

run ~/vm/wingvt


### Qemu 
[dependencies]
virt-manager qemu vde2 ebtables dnsmasq bridge-utils openbsd-netcat

[run]
sudo systemctl enable libvirtd.service
sudo systemctl enable gvtvgpu.service

uncomment for users in /etc/libvirt/libvirtd.conf
    unix_sock_group = "libvirt"
    unix_sock_rw_perms = "0770"
[run]
sudo usermod -a -G libvirt $(whoami)


### Qmue NAT network
[article]
https://wiki.qemu.org/Documentation/Networking/NAT
[dependencies]
bridge-utils iptables dnsmasq

make /etc/qemu-ifup ad intended in article
chmod 755 /etc/qemu-ifup


### Create qcow2 drive 
qemu-img create -f qcow2 /home/rafael/vm/windows_10.qcow2 64G

### install intel drivers 15.45.23.4860 from zip, through device manager, not microsoft basic vga


### compile qemu changing 

[https://github.com/qemu/qemu]

--- GUI_REFRESH_INTERVAL_DEFAULT 30 
+++ GUI_REFRESH_INTERVAL_DEFAULT 16
make install

### add
```
GRUB_CMDLINE_LINUX_DEFAULT="loglevel=1 i915.enable_gvt=1 kvm.ignore_msrs=1"
```
to /etc/default/grub

then 
```
grub-mkconfig -o /boot/grub/grub.cfg
```
