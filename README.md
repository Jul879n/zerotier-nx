# ZeroTier NX

ZeroTier en la Nintendo Switch, todo en uno: el **módulo** `sys-GRID0-tcp` (en `module/`, fork de [sys-GRID0](https://github.com/redluigi323/sys-GRID0) con TCP sobre lwIP y MITM ampliado para homebrew) y el **overlay Ultrahand/Tesla** `zerotier-nx` (este repo) para controlarlo.

Con esto la consola entra en una red ZeroTier y funcionan por la VPN el ping, los juegos LAN por UDP de la lista blanca y homebrew por TCP (FTP de Sphaira verificado). Instalación completa y reversión en [INSTALACION.md](INSTALACION.md). Estado y límites en [GOAL.md](GOAL.md).

Pantallas del overlay: Menú, Estado, Login QR (servidor HTTP local en el overlay), Login manual y Redes guardadas.

## Instalar (usuarios)
Descargue `zerotier-switch-<versión>.zip` de [Releases](../../releases) y copie su contenido a la raíz de la SD. Requiere Atmosphère y Ultrahand. Sin logs de depuración (`debug_logging = 0`).

## Compilar (desarrolladores)
```
git clone --recursive https://github.com/Jul879n/zerotier-nx.git
cd zerotier-nx
DEVKITPRO=/opt/devkitpro make                      # overlay
make -C module                                      # módulo (ver module/README.md)
RELEASE_REPO=Jul879n/zerotier-nx tools/release.sh v0.2.0
```
`tools/release.sh` arma el paquete de la SD (módulo + overlay + `config.ini` sin depuración) y crea el release en GitHub.

## Tests
`make -C tests run` (overlay) y `make -C module/tests run SANITIZE=` (módulo).

## Prueba en consola (verificada por el usuario)
Marcadas como verificadas en la consola por el usuario (2026-10-23); no las repetí en esta sesión, salvo que el overlay aparece como "ZeroTier NX" y el Estado muestra "Conectado".
- [x] a) El overlay aparece en Ultrahand como "ZeroTier NX".
- [x] b) Estado muestra IP, Node ID, Peers y Ping.
- [x] c) Login QR: el teléfono (misma Wi‑Fi) envía `8056c2e21c000001` y en ≤2 s el overlay muestra "Conectando a 8056C2E21C000001…"; `networks.ini` tiene la sección nueva y `selected`.
- [x] d) Un POST con token viejo responde 403.
- [x] e) El login manual guarda y conecta.
- [x] f) Redes guardadas: Ⓐ cambia de red, Ⓨ la olvida.
- [x] g) El toggle Conexión renombra `boot2.flag`.

## Créditos y licencias
GPL-2.0 (por libultrahand). sys-GRID0 y el fork con TCP (MPL-2.0), lwIP (BSD), libultrahand (GPL-2.0), nayuki/QR-Code-generator (MIT).
