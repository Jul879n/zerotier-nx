#!/bin/sh
# Solo lectura: cada 10 s registra ping, par de la consola e info local de ZeroTier.
CONSOLE_IP=192.168.196.194
NODE=e8dc8f406f
LOG="$HOME/Documents/zt-watch.log"
while :; do
    ts=$(date '+%H:%M:%S')
    p=$(ping -c1 -W2000 "$CONSOLE_IP" 2>&1 | grep -E 'time=|100.0% packet loss' | head -1)
    peer=$(zerotier-cli peers 2>&1 | grep "$NODE")
    info=$(zerotier-cli info 2>&1)
    echo "$ts | ping: ${p:-sin respuesta} | peer: ${peer:-ausente} | $info" >> "$LOG"
    sleep 10
done
