# kill old mouse driver
gpm -k

# start mouse driver in repeater mode

# RH 6
# gpm -R -t ps2

# RH 9
gpm -R -t imps2 -m /dev/mouse
