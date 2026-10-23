#!/bin/sh
# Espera el listado FTP por la IP ZeroTier (hasta 55 min), luego mide una descarga de 746 KB con hash.
IP="${1:?uso: zt-bench-ftp.sh <IP ZeroTier> <IP LAN>}"; LAN="${2:?falta IP LAN de la consola}"; D="$HOME/Documents/zt-ftp-test"; mkdir -p "$D"; i=0
while [ $i -lt 165 ]; do
    out=$(curl -s -m 12 -l ftp://$IP:5000/ 2>/dev/null)
    [ -n "$out" ] && break
    i=$((i+1)); sleep 20
done
[ -z "$out" ] && { echo "timeout: sin FTP por VPN"; exit 1; }
up=$(curl -s -m 10 ftp://$LAN:5000/sdmc:/config/sys-zerotier/status.txt | head -1)
echo "FTP por VPN OK $(date +%H:%M:%S) | $up"
s=$(date +%s)
curl -s -m 200 ftp://$IP:5000/sdmc:/atmosphere/contents/4200000000005A54/exefs.nsp -o "$D/bench.nsp"
e=$(date +%s)
echo "descarga: $(stat -f %z "$D/bench.nsp") B en $((e-s)) s | sha256 $(shasum -a 256 "$D/bench.nsp" | cut -c1-16)"
