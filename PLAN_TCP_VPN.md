# Plan: juegos y homebrew con VPN (TCP en fork de sys-GRID0)

Objetivo: ver `GOAL.md`. Prueba de aceptación elegida: FTP de la consola (puerto 5000) por la IP de ZeroTier. Estado: en ejecución por un agente en segundo plano (2026-10-23). Nada de esto está verificado en hardware.

## Reglas
- Trabajo solo en el fork `~/Documents/sys-GRID0-tcp` (copia con `cp -R`). El original `~/Documents/sys-GRID0` y `sys-GRID0-fw23` no se tocan.
- Sin git ni stash. No se sube nada a la consola ni se reinicia sin decisión del usuario.
- lwIP se descarga en `~/Documents/lwip-src` (datos no confiables; no se ejecuta nada de ahí).
- Cada afirmación se verifica contra código o salida real. Lo no verificable queda como pendiente.

## Punto de partida (verificado)
- `source/net/vnet.cpp:238-243` solo entrega UDP e ICMP; el resto se descarta. `vnet.hpp`: "no lwIP, no TCP".
- El MITM de `bsd:u` desvía solo UDP en puertos PIA 30000, 35000, 40000 y 49152–49155 (`bsd_shim.cpp:553`).
- `config.ini` de la consola: `bsd_mitm = 1`, `nifm_mitm = 1`.
- Arena fijo de 1152 KB, ~540 KB contiguos libres (`boot.log`); 1024 KB está marcado "known-bad" (`main.cpp:44-72`).
- lwIP no está en `portlibs`, ni en el `dkp`, ni en el árbol (informe de la evaluación previa).

## Fases
| # | Paso | Entregable | Criterio de salida |
|---|---|---|---|
| 1 | Presupuesto de memoria y viabilidad | `TCP_PLAN.md` en el fork | Memoria estimada cabe en ~540 KB contiguos; si no, se detiene y se reporta |
| 2 | Vendorizar lwIP (release estable) con `lwipopts.h` mínimo (`NO_SYS=1`, sin sockets API, ventanas TCP chicas, pool de pbuf acotado) | `lib/lwip` | Compila con la toolchain de dkp |
| 3 | lwIP como netif sobre VNet: TCP entra por `deliverIpv4`, sale por `sendIpv4`; timers desde el loop del nodo | cambios en `source/net/` | Test de host con handshake TCP simulado |
| 4 | Virtualizar TCP en `bsd_shim.cpp` (socket, bind, listen, accept, connect, send, recv, close, poll) cuando corresponda a la IP ZeroTier | cambios en `source/mitm/` | Tests de host; se documenta si el FTP homebrew pasa por `ShouldMitm` |
| 5 | Compilar `.nsp` y medir | tamaño y memoria reales de la compilación | Compila sin errores |
| 6 | Prueba en la consola (decisión del usuario) | resultado de `nc`/`curl ftp://<IP ZeroTier>:5000/` | Conecta y lista archivos por VPN |

## Riesgos
- **Memoria:** una pila TCP puede agotar el arena y dejar el módulo sin red o con crash reports al arranque.
- **Alcance del MITM:** `ShouldMitm` intercepta solo la aplicación en ejecución. Un homebrew de servidor puede quedar fuera sin un cambio de diseño mayor.
- **Toolchain:** no se sabe si lwIP compila con este dkp.
- **Sin hardware:** el comportamiento real solo se confirma en la consola.

## Pendiente fuera de este plan
- Fase 1 de prueba UDP: sondeo desde el Mac a los puertos PIA con lectura de `status.txt` (no aprobada aún).
- Juego LAN real entre dos equipos por VPN.
- Camino directo sin relay: Mac en una red sin CGNAT.
- Juegos solo con modo inalámbrico local: requieren un MITM de `ldn:u` que no existe.
