# Goal: juegos y homebrew deben funcionar correctamente con VPN

Estado: **CERRADO con límites** (decisión del usuario, 2026-10-23). Cumplido para homebrew, FTP y TCP por VPN; fuera de alcance lo listado en "Límites del cierre". Verificación al 2026-10-23. Prueba de aceptación elegida por el usuario: FTP de la consola (puerto 5000) por la IP de ZeroTier.

## Criterios

| # | Criterio | Estado | Evidencia |
|---|---|---|---|
| 1 | La consola se une a la red y responde ping por VPN | Cumplido | `ping 192.168.196.194` desde el Mac en la red del teléfono: 4/4, media 259 ms, por RELAY. `~/Documents/zt-watch.log`: 48/48 lecturas sin fallo (00:21–00:29) |
| 2 | Cerrar el overlay no corta la VPN | Cumplido | Prueba A/B (`tools/zt-ab.sh`): ping continuo con el overlay cerrado de 00:24 a 00:29. El único corte (00:16–00:20) fue del relay recién establecido |
| 3 | Estado del overlay estable (sin "no detectado") | Pendiente de confirmar | Corrección instalada (sha256 `0c01b67c…ee615` coincide). El usuario vio "Conectado" en una ventana; falta observarlo 2 min completos |
| 4 | Homebrew por TCP (FTP de Sphaira, puerto 5000) accesible por VPN | **Cumplido** (2026-10-23, LAN) | Con `sys-GRID0-tcp.pasv.nsp` (sha256 `bc3feb0b…`): ruta por `feth2298` (interfaz ZeroTier); listado de `sdmc:` (47 entradas, 3 veces seguidas); descarga de `exefs.nsp` (746 535 B) con sha256 idéntico al instalado; subida de 3 MB aleatorios por VPN con sha256 igual verificado por la LAN; `bsd.log`: `ZT tcp accept … from 192.168.196.122`. Archivo de prueba borrado |
| 5 | Juegos con modo LAN (UDP) funcionan por VPN | Sin verificar | El MITM de `bsd:u` desvía solo UDP en puertos 30000, 35000, 40000 y 49152–49155 (`bsd_shim.cpp:553`). No se probó ningún juego |
| 6 | Juegos solo con modo inalámbrico local (`ldn:u`) | No cumplido | No existe MITM de `ldn:u` (`README` de sys-GRID0, "current limitations") |
| 7 | Camino directo (sin relay) | No cumplido | Par en `RELAY`, latencia 232–438 ms. El Mac está tras CGNAT del teléfono |

## Verificado en la consola (00:48, Mac en el Wi-Fi "Fran")
- `config.ini`: `bsd_mitm = 1`, `nifm_mitm = 1`, `debug_logging = 1`, `port_mapping = 1`. Los dos MITM están activos.
- `status.txt`: uptime 4784 s, `online yes`, `netstatus ok`, `fail 0`, `errno 0`. El módulo no se reinició durante la prueba (la prueba fue en uptime ~2700–3700 s).
- `uplink.log` solo cubre uptime 4052–4784 s (~12 min), posterior a la prueba: 24 beats cada 30 s, sin huecos ni `PAUSE`/`OFFLINE`. No cubre la ventana de la prueba, por lo que no sirve como evidencia de ella.
- `rx icmp 688 / tx icmp 888`, `udp 28`: el tráfico por VPN es casi todo ICMP (ping).

## Pendiente en el criterio 4
- Probado con el Mac en la misma LAN que la consola (camino DIRECT, 5 ms). Falta repetirlo desde la red del teléfono (RELAY) y con transferencias largas.
- Estabilidad del módulo tras uso prolongado (arena, PCB, keepalive).

## Sin verificar
- Comportamiento en reposo largo de la consola.
- Ningún juego probado por VPN.

## Para cerrar el goal
1. Elegir un juego LAN concreto y probarlo entre dos equipos en la VPN (criterio 5).
2. Decidir si el TCP entrante se implementa en sys-GRID0 (lwIP + desvío de sockets): cambio grande en software de terceros, necesario para el criterio 4.
3. Probar el Mac en una red sin CGNAT para medir el camino directo (criterio 7).
4. Confirmar el criterio 3 en la consola durante 2 min.

## Límites del cierre
- **Medido:** FTP de Sphaira por VPN con `sys-GRID0-tcp.fast.nsp`: 748 447 B en 20 s con hash correcto (camino DIRECT; por la LAN sin VPN, 9 s). El usuario reportó que desde el teléfono con datos móviles (relay) funciona, algo lento; no se midió con el script.
- **Sin probar:** ningún juego LAN por UDP (criterio 5).
- **Sin soporte:** juegos solo con modo inalámbrico local (`ldn:u`, criterio 6).
- **Sin garantía:** uso prolongado del módulo y camino directo tras CGNAT (criterio 7).
- Entrega: release https://github.com/Jul879n/sys-GRID0-tcp/releases/tag/v0.2.0 y repo `Jul879n/sys-GRID0-tcp` (público).
