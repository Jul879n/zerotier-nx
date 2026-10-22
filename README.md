# ZeroTier NX

Overlay Ultrahand/Tesla para controlar ZeroTier en Nintendo Switch. Usa [sys-GRID0](https://github.com/redluigi323/sys-GRID0) como backend (sin modificarlo) y comparte con él los archivos de `sdmc:/config/sys-zerotier/`.

Pantallas: Menú, Estado, Login QR (servidor HTTP local en el overlay), Login manual y Redes guardadas.

## Requisitos
- Atmosphère, nx-ovlloader y Ultrahand.
- **sys-GRID0 v0.1.2 instalado.**

## Instalación
Copiar `zerotier-nx.ovl` a `sdmc:/switch/.overlays/`.

## Build
`git submodule update --init` y luego `DEVKITPRO=/opt/devkitpro make`.

## Tests
`make -C tests run`

## Prueba en consola (pendiente)
- [ ] a) El overlay aparece en Ultrahand como "ZeroTier NX".
- [ ] b) Estado muestra IP, Node ID, Peers y Ping.
- [ ] c) Login QR: el teléfono (misma Wi‑Fi) envía `8056c2e21c000001` y en ≤2 s el overlay muestra "Conectando a 8056C2E21C000001…"; `networks.ini` tiene la sección nueva y `selected`.
- [ ] d) Un POST con token viejo responde 403.
- [ ] e) El login manual guarda y conecta.
- [ ] f) Redes guardadas: Ⓐ cambia de red, Ⓨ la olvida.
- [ ] g) El toggle Conexión renombra `boot2.flag`.

## Créditos y licencias
GPL-2.0 (por libultrahand). sys-GRID0 (MPL-2.0), libultrahand (GPL-2.0), nayuki/QR-Code-generator (MIT).
