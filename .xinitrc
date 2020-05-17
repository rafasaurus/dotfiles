# #!/bin/sh

[ -f ~/.xprofile ] && . ~/.xprofile

# Loop enables dwm's "restart" feature (mod+Shift+F2).
while :; do
    ssh-agent dwm > ~/.dwm.log || break
done
