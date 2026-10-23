#!/bin/sh
# Guía la secuencia A/B: avisa por voz/notificación y marca el log.
LOG="$HOME/Documents/zt-watch.log"
step() { # texto segundos
    echo "$(date '+%H:%M:%S') === MARCA: $1 ===" >> "$LOG"
    osascript -e "display notification \"$1\" with title \"ZeroTier A/B\"" 
    say -v Monica "$1" 2>/dev/null || say "$1"
    sleep "$2"
}
step "Abre el overlay ahora. Tres minutos." 180
step "Cierra el overlay. Cinco minutos." 300
step "Abre el overlay. Tres minutos." 180
step "Cierra el overlay. Cinco minutos." 300
step "Fin de la prueba. Vuelve a la red de la consola." 1
