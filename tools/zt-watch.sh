#!/bin/sh
# Solo lectura: cada 10 s registra ping, par de la consola e info local de ZeroTier.
CONSOLE_IP="${1:?uso: zt-watch.sh <IP ZeroTier de la consola> <NodeID>}"
NODE="${2:?falta NodeID de la consola}"
LOG="$HOME/Documents/zt-watch.log"
while :; do
    ts=$(date '+%H:%M:%S')
    p=$(ping -c1 -W2000 "$CONSOLE_IP" 2>&1 | grep -E 'time=|100.0% packet loss' | head -1)
    peer=$(zerotier-cli peers 2>&1 | grep "$NODE")
    info=$(zerotier-cli info 2>&1)
    echo "$ts | ping: ${p:-sin respuesta} | peer: ${peer:-ausente} | $info" >> "$LOG"
    sleep 10
done
