### mounting external hard drive in linux
edit `/etc/fstab` with root
```
UUID=f92d94ee-109c-49b3-9779-1bcfc37d4d42  /home/folder   ext4  defaults,noatime      0      2
```
get UUID
```
sudo blkid
```
### openvpn setup
with ubuntu network manager import vpn config `*.ovpn`
then add key `ta.key`

### ffmpeg
make video 5x faster
```
ffmpeg -i input.mov -vf "setpts=0.2*PTS" -an output.mov
```
make video out of images
```
 ffmpeg -y -r 25 -f concat -safe 0 -i tovid.txt -c:v libx264 -vf fps=25,format=yuv420p out.mp4
```
inside tovid.txt
*   file 'file0 path'
*   file 'file1 path'

### latex package setup
to find latex packages directory
download zip and extract in
```
kpsewhich --expand-var '$TEXMFMAIN'
```

### vpn terminal

# or nmcli con list
```
nmcli con
```
connect
```
nmcli con up uuid <uuid>
```
### change keyboard layout in i3
```
setxkbmap -option grp:switch,grp:alt_space_toggle,grp_led:scroll,compose:menu -layout 'us,lt,am' -variant ',,phonetic'
```
### keyboard repead time
```
xset r rate 400 50
```

### change/replace variable name
`sed -i "s/old/new/g" <file_name>`
### change/replace with backup
```
sed -i.bak 's/lgflags_shared/lgflags/g  link.txt
sed -i.bak 's/lgflags_shared/lgflags/g' `find -name link.txt
```

### docker run /bin/bash
```
`install docker raspberrypi`
    curl -sSL https://get.docker.com | sh
`add user to docker group`
    sudo usermod -aG docker $USER
docker run -it <container_hash> /bin/bash
docker commit <container_hash> new_image_name:tag_name(optional)
docker save <image> --output <name>.tar.gz
docker load < load <name>.tar.gz
docker pull image_name:latest
```

### make a swap file
```
sudo dd if=/dev/zero of=/extraswap bs=1M count=20512
sudo mkswap /extraswap
sudo swapon /extraswap
```

### write images to sd card
```
dd bs=4M if=2018-11-13-raspbian-stretch.img of=/dev/sdX conv=fsync
```

### photorec for disk recovery

### setup ip linux
```
auto lo
iface lo inet loopback

allow-hotplug wlan0
iface wlan0 inet dhcp
    wpa-conf /etc/wpa_supplicant/wpa_supplicant.conf

auto eth0
iface eth0 inet static
    address 10.10.10.1
    netmask 255.255.255.0

### for manual ip address

iface eth0 inet static
    address 192.168.1.1
    netmask 255.255.255.0
    gateway 192.168.1.254
/etc/init.d/networking restart

```

### jupyter notebook permision fix
```
sudo chown -R user:user ~/.local/share/jupyter 
```

### youtube-dl audio only 
```
youtube-dl -f bestaudio --audio-quality 0 --audio-format mp3 <url>
```

### i3debug
`exports log to i3 site`

DISPLAY=:0 i3-dump-log | bzip2 -c | curl --data-binary @- https://logs.i3wm.org

### ripgrep, search string in many files
`https://github.com/phiresky/ripgrep-all`

### apt-file search file
`apt-file find kwallet.h`

### watch cpu freq
`watch -n.1 "cat /proc/cpuinfo | grep \"^[c]pu MHz\""`

### git config project only
`git config user.name "John Doe"`


### archiving with tar with exclude
`tar -cvf backup.tar --exclude="public_html/template/cache" public_html/`

### disable shutdownbutton
#### edit /etc/systemd/logind.service
HandlePowerKey=ignore
sudo systemctl restart systemd-logind.service

### install virtualbox in arch linux
sudo pacman -Sy virtualbox linux44-virtualbox-host-modules and sudo pacman -R virtualbox-host-dkms
# if you want to install extension pack, use 
yay virtualbox-ext-oracle

### virtualbox optimize space (windows vm)
As the SDelete page on Microsoft’s website notes, the -z option is “good for virtual disk optimization”.
`https://www.howtogeek.com/312883/how-to-shrink-a-virtualbox-virtual-machine-and-free-up-disk-space/`
*   download Sdelete
**  sdelete.exe c: -z

### pair with bluetoothctl
service bluetooth start
bluetoothctl
$ power on
$ agent on
$ scan on
pair AA:BB:CC:DD:EE:FF
connect AA:BB:CC:DD:EE:FF

### Download offline site with wget 
wget \
 --recursive \
 --no-clobber \
 --page-requisites \
 --html-extension \
 --convert-links \
 --wait=5 --limit-rate=200K \
 --restrict-file-names=windows \
 -P /Volumes/AppleHDD/Documents \
 --domains developer.android.com \
 --no-par\
     http://https://developer.android.com/training/wearables/

### Arduino serial monitor fix
archlinux-java status
sudo pacman -Syu jre11-openjdk
sudo archlinux-java set java-11-openjdk

### batch file rename/catdoc
for file in *.doc; do catdoc $file > ${file%.doc}.txt; done

### start vncserver as other user then root in raspberry pi
su -c "vncserver :2" -s /bin/sh pi

### after the wifite "sengmsg ping: operation not permitted"
iptables -L
sudo iptables -P INPUT ACCEPT
sudo iptables -P OUTPUT ACCEPT

##### save the configuration of iptables
sh -c "iptables-save -c > /etc/iptables.rules" 
##### enable iptables.service
systemctl enable iptables.service

#### install solidworks vm, cpuid problem fix
insert this lines into .vbox file of the vm
```
    <ExtraDataItem name="VBoxInternal/Devices/ahci/0/Config/Port0/ModelNumber" value="SEAGATE ST3750525AS"/>
    <ExtraDataItem name="VBoxInternal/Devices/pcbios/0/Config/DmiBIOSVendor" value="American Megatrends Inc"/>
    <ExtraDataItem name="VBoxInternal/Devices/pcbios/0/Config/DmiSystemVendor" value="ASUSTek Computer"/>
```

# adjust time
timedatectl set-ntp true

# pacman delete with dependencies
	pacman -Rcns

# add user to the group
sudo usermod -a -G group $(whoami)
# scan ports with nmap
nmap -v -sT 192.168.1.18/24
