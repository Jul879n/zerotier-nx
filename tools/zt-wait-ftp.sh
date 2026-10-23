#!/bin/sh
# Espera (hasta 55 min) a que el listado FTP completo funcione por la IP ZeroTier e imprime evidencia.
IP=192.168.196.194; i=0
while [ $i -lt 165 ]; do
    out=$(curl -s -m 12 -l ftp://$IP:5000/ 2>/dev/null)
    if [ -n "$out" ]; then
        echo "LISTADO FTP POR VPN OK ($(date +%H:%M:%S)):"; echo "$out" | head -10
        exit 0
    fi
    i=$((i+1)); sleep 20
done
echo "timeout: sin listado FTP por VPN"
