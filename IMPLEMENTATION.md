# Plan: ZeroTier NX — overlay Ultrahand (opción 2) sobre sys-GRID0, con login QR por HTTP local

## Context
Petición: "crear la app", más "crear un md de implementación y repo privado". Se construye un overlay Tesla/Ultrahand con la UI del mockup elegido (`~/Documents/switch-zerotier-ui/mockup-ultrahand.html`, opción 2 "Ultrahand moderno"). Tiene 5 pantallas: Menú, Estado, Login QR, Login manual y Redes guardadas. El código vive en un repo privado nuevo `Jul879n/zerotier-nx`, con este plan copiado como `IMPLEMENTATION.md`. Estado final: el repo publicado (privado), el `.ovl` compilando con devkitPro, los tests de host pasando y un checklist de prueba en consola.

**Decisión de arquitectura:** no se escribe un sysmodule propio. Se usa **sys-GRID0** (https://github.com/redluigi323/sys-GRID0, MPL-2.0, release `v0.1.2`, title ID `0x4200000000005A54`) como backend sin modificarlo. Razones:
- Es un port de ZeroTierOne que enruta el tráfico LAN de los juegos mediante las MITM `bsd:u` y `nifm:u`. Ya está probado con MK8D y Splatoon.
- libzt-nx solo enruta sus propios sockets `zts_*`, así que no sirve para LAN play.

El overlay se comunica con sys-GRID0 por los archivos de la SD, igual que su overlay oficial. **El servidor HTTP del login QR corre dentro del overlay** mientras la pantalla QR está abierta, así que el sysmodule no se toca.

Decisiones del usuario: MVP completo, login QR por servidor web local, y consola con Atmosphère + Ultrahand (no disponible ahora). Por eso la verificación de esta etapa es la compilación más los tests de host. La prueba en hardware queda como checklist.

## Hechos verificados (fuente leída en esta sesión)
- **devkitPro:** está en `/opt/devkitpro` con libnx 4.12.0 y devkitA64 r30 (gcc 16.1). Portlibs presentes: `libcurl`, `libz`, `libminizip`, `libmbedtls`, `libmbedx509` y `libmbedcrypto`. La variable `DEVKITPRO` no está exportada, así que hay que anteponer `DEVKITPRO=/opt/devkitpro` a cada `make`. El `cmake` del sistema no existe (no se usa).
- **Compilador del host:** Apple clang 17 (`c++`). `gh` tiene sesión iniciada como `Jul879n` con scope `repo`. El repo `Jul879n/zerotier-nx` no existe.
- **Contrato de sys-GRID0** (`overlay/source/main.cpp` y `source/zt_port.cpp` en `main`):
  - `sdmc:/config/sys-zerotier/status.txt` usa líneas `clave<espacios>valor`. Claves usadas:
    - `network` (16 hex)
    - `online` (`yes`/`no`)
    - `netstatus` (texto)
    - `address` (`a.b.c.d/n`; `0.0.0.0…` significa sin IP)
    - `uptime` (`N s …`)
  - `sdmc:/config/sys-zerotier/peers.txt`:
    - Primera línea: `peers <N>   (uptime …)`.
    - Luego una línea por peer: `<10hex> role <r>  latency <ms>  paths <p>  v…`, con indentación de 4 espacios en las líneas de path.
    - Se reescribe cada ≥30 s.
  - `sdmc:/config/sys-zerotier/networks.ini`:
    - Una sección por red con `nwid = <16hex>`.
    - La sección interna `[sys-zerotier]` guarda la clave `selected = <nombre de sección>`.
    - El sysmodule detecta el cambio y cambia de red en vivo.
  - `sdmc:/config/sys-zerotier/config.ini`, sección `[sys-zerotier]`: claves `bsd_mitm`, `nifm_mitm` y `debug_logging` (`1`/`0`), que se aplican al reiniciar.
  - Arranque: `sdmc:/atmosphere/contents/4200000000005A54/flags/boot2.flag`. Se desactiva renombrándolo a `boot2.flag.disabled`, con efecto al reiniciar.
  - Identidad: `sdmc:/atmosphere/contents/4200000000005A54/zt/identity.public`. Formato estándar de ZeroTier `<10hex>:0:<clave>`. El Node ID son los 10 primeros caracteres (formato *unverified — confirm first* leyendo un archivo real en la consola).
- **libultrahand**, commit `856ddbddd796fc4a59ad2e0bf939c5963e6f9dd2` (el que fija sys-GRID0):
  - `ult::parseSectionsFromIni`, `ult::parseValueFromIniSection`, `ult::setIniFileValue`, `ult::removeIniSection`, `ult::createDirectory` (`libultra/include/ini_funcs.hpp`).
  - `tsl::elm::CustomDrawer(std::function<void(gfx::Renderer*, s32 x, s32 y, s32 w, s32 h)>)`, `Renderer::drawRect(x,y,w,h,Color)` y `Renderer::drawString(text, monospace, x, y, fontSize, color)` (`libtesla/include/tesla.hpp`).
  - `ult::syncNtp` (`libultra/source/download_funcs.cpp`) muestra el patrón de sockets bajo demanda: `socketInitialize(&cfg)` y luego `socketExit()`.
  - La configuración global vive en `/config/ultrahand/`.
- **Makefile de referencia:** `overlay/Makefile` de sys-GRID0. Requiere `--nacp=` en `elf2nro`; sin ese flag Ultrahand omite el `.ovl` sin avisar.
- **QR:** nayuki/QR-Code-generator, versión C (`c/qrcodegen.c` y `c/qrcodegen.h`, MIT según la cabecera).

## Approach

### 1. Crear el repo local y la estructura (sin código todavía)
- Crear `~/Documents/zerotier-nx/` y ejecutar `git init -b main`.
- Agregar el submódulo: `git submodule add https://github.com/ppkantorski/libultrahand lib/libultrahand`. Fijarlo con `git -C lib/libultrahand checkout 856ddbddd796fc4a59ad2e0bf939c5963e6f9dd2` (el mismo commit que valida sys-GRID0).
- Copiar sin mover los mockups: `cp ~/Documents/switch-zerotier-ui/{mockups.html,mockup-ultrahand.html} docs/mockups/`.
- `IMPLEMENTATION.md`: copia literal de este plan.
- `README.md` (español, ≤40 líneas) con:
  - Qué es.
  - Requisitos: Atmosphère, nx-ovlloader + Ultrahand y **sys-GRID0 v0.1.2 instalado**.
  - Instalación: copiar `zerotier-nx.ovl` a `/switch/.overlays/`.
  - Build: `DEVKITPRO=/opt/devkitpro make`.
  - Tests: `make -C tests run`.
  - Créditos: sys-GRID0 (MPL-2.0), libultrahand (GPL-2.0) y nayuki qrcodegen (MIT).
- `LICENSE`: GPL-2.0, texto de https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt. Es obligatorio porque se enlaza libultrahand (GPL-2.0).
- `.gitignore`: `build/`, `*.ovl`, `*.nro`, `*.nacp`, `*.elf`, `*.map`, `tests/core_test`.
- Vendorizar `source/qrcodegen.c` y `source/qrcodegen.h` desde `https://raw.githubusercontent.com/nayuki/QR-Code-generator/master/c/`, sin cambios. No existe un generador QR en libnx ni en libultrahand.

### 2. Makefile del overlay
`Makefile` en la raíz: copia de `sys-GRID0/overlay/Makefile` con solo estos cambios.
- `APP_TITLE := ZeroTier NX`
- `APP_AUTHOR := Jul879n`
- `APP_VERSION := 0.1.0`
- `TARGET := zerotier-nx`
- `-DUI_OVERRIDE_PATH="\"/config/zerotier-nx/overlay/\""`

Se mantienen tal cual `LIBS`, `ARCH`, los flags, `-fno-exceptions -fno-rtti` y el bloque `NROFLAGS += --nacp=…` con `printf 'ULTR'`. El Makefile ya compila `.c`, así que `qrcodegen.c` entra sin cambios.

### 3. Lógica pura testeable en el host: `source/core.hpp` y `source/core.cpp`
Sin libnx ni tesla; solo `<string>`, `<vector>`, `<cstdint>` y `<cstdio>`. No existe equivalente reutilizable: las funciones de sys-GRID0 leen archivos directamente; aquí se reescriben sobre texto. Firmas exactas, todas en `namespace ztnx`:
- `bool IsHexNetworkId(const std::string&)`: 16 caracteres hex, no todos cero. Misma regla que sys-GRID0.
- `std::string ToLowerHex(std::string)` y `std::string ToUpperHex(std::string)`.
- `std::string StatusValue(const std::string& text, const std::string& key)`:
  - La clave debe ir seguida de espacio o tab. Se recortan espacios y CR/LF.
  - Devuelve `""` si no está. Reproduce el `StatusValue` de sys-GRID0 sobre texto.
- `struct PeerSummary { unsigned count = 0; int avgLatencyMs = -1; };` y `PeerSummary ParsePeers(const std::string& text)`:
  - `count` sale de la primera línea `peers <N>`.
  - `avgLatencyMs` es el promedio entero de `latency` en las líneas de peer (sin indentación) con `role 0` (LEAF) y `latency >= 0`. Si no hay ninguna, queda en `-1`.
  - Texto vacío o malformado devuelve `{0,-1}`.
- `std::string NodeIdFromIdentity(const std::string& text)`: 10 primeros caracteres si son hex y van seguidos de `:`; si no, `""`.
- `std::string DisplayAddress(const std::string& address)`: `""` o que empiece con `0.0.0.0` → `"Sin asignar"`; si no, quita el sufijo `/n`.
- `std::string DisplayState(const std::string& online, const std::string& netstatus)`:
  - `online=="yes"` → `"Conectado"`, o el `netstatus` si no es `OK` ni está vacío.
  - En otro caso → `"Conectando…"`.
  - Si `online` está vacío (no hay `status.txt`) → `"sys-GRID0 no detectado"`.
- HTTP:
  - `struct HttpRequest { std::string method, path, query, body; bool complete = false; };`
  - `HttpRequest ParseHttpRequest(const std::string& raw)`:
    - Lee la request line y separa el path del query por `?`.
    - Usa `Content-Length` (sin distinguir mayúsculas).
    - `complete=true` solo si llegaron los headers (`\r\n\r\n`) y `body.size() >= Content-Length`.
  - `std::string FormValue(const std::string& urlencoded, const std::string& key)`: decodifica `+` → espacio y `%XX`; devuelve `""` si la clave no está.
  - `std::string HttpResponse(int status, const std::string& contentType, const std::string& body)`:
    - Status line con texto: 200 `OK`, 400 `Bad Request`, 403 `Forbidden`, 404 `Not Found` y 405 `Method Not Allowed`.
    - Headers `Content-Type`, `Content-Length` y `Connection: close`.
- `enum class JoinResult { Ok, BadToken, BadId };` y `JoinResult ValidateJoin(const std::string& body, const std::string& token, std::string& outNwid, std::string& outName)`:
  - El campo `t` debe ser igual a `token`.
  - El campo `nwid`, sin espacios, debe pasar `IsHexNetworkId`. Se devuelve en minúsculas.
  - `name` es opcional. Solo se aceptan `[A-Za-z0-9 _-]` y se trunca a 32 caracteres; si queda vacío, se usa el nwid.

### 4. Acceso a la SD de sys-GRID0: `source/sysgrid.hpp` y `source/sysgrid.cpp`
Envuelve `core` con E/S de libnx y libultrahand. Constantes con las rutas exactas de "Hechos verificados".
- `std::string ReadFile(const char* path)`: `fopen`/`fread`, máximo 4096 bytes; `""` si no existe.
- `struct Snapshot { std::string state, ip, nodeId, network; PeerSummary peers; };` y `Snapshot ReadSnapshot()`:
  - Lee `status.txt`, `peers.txt` e `identity.public`.
  - Arma los campos con las funciones de `core`.
- `std::vector<std::pair<std::string,std::string>> SavedNetworks()`: sección → nwid. Mismo bucle que `MainGui::createUI` de sys-GRID0: omite `sys-zerotier` y las secciones con nwid inválido.
- `std::string SelectedNetwork()`, `void SelectNetwork(const std::string& section)` y `void SaveAndSelectNetwork(const std::string& nwid, const std::string& name)`:
  - Se copia el patrón de sys-GRID0: `createDirectory` + `setIniFileValue`.
  - Diferencia: la sección se llama `name` (validado en `ValidateJoin`) en lugar del nwid.
  - Si ya existe una sección con ese nombre y otro nwid, se sobrescribe su `nwid`.
- `void ForgetNetwork(const std::string& section)`:
  - Llama a `ult::removeIniSection(NetworksPath, section)`.
  - Si era la seleccionada, primero hace `setIniFileValue(NetworksPath,"sys-zerotier","selected","")`.
- `bool BootEnabled()` y `void SetBootEnabled(bool)`: copia literal de `SetBootEnabled`/`FileExists` de sys-GRID0. `BootEnabled() = FileExists(BootFlag) || !FileExists(DisabledBootFlag)`.
- **Cabecera de atribución:** `// Partes adaptadas de sys-GRID0 (MPL-2.0), https://github.com/redluigi323/sys-GRID0`.

### 5. Servidor HTTP del login QR: `source/http_server.hpp` y `source/http_server.cpp`
No existe equivalente en libnx ni en libultrahand. Clase `ztnx::QrServer`:
- `bool Start()`:
  1. `nifmInitialize(NifmServiceType_User)` + `nifmGetCurrentIpAddress(&ip)`. Si falla o `ip==0` → `m_error = "Conecta la consola a Wi‑Fi"` y devuelve false.
  2. `socketInitialize(&cfg)` con `cfg = {tcp_tx_buf_size=0x1000, tcp_rx_buf_size=0x1000, tcp_tx_buf_max_size=0, tcp_rx_buf_max_size=0, udp_tx_buf_size=512, udp_rx_buf_size=512, sb_efficiency=1, bsd_service_type=BsdServiceType_Auto}`. Es el patrón de `ult::syncNtp` con buffers TCP. Si falla → `m_error = "No se pudo iniciar la red"`.
  3. `socket(AF_INET, SOCK_STREAM)`, `SO_REUSEADDR`, y `bind` en el puerto 8080; si falla con `EADDRINUSE`, prueba de 8081 a 8089. Luego `listen(fd, 2)`. Si todos fallan → `m_error = "Puerto ocupado"`.
  4. Token: 6 caracteres de `ABCDEFGHJKLMNPQRSTUVWXYZ23456789` generados con `randomGet`.
  5. URL: `http://a.b.c.d:PORT/?t=TOKEN`.
  6. Crea el hilo con `threadCreate(&m_thread, Loop, this, nullptr, 0x4000, 0x2C, -2)` + `threadStart`.
- `Loop`: mientras `!m_stop`:
  - `poll` sobre el fd de escucha con timeout de 200 ms, luego `accept`.
  - `SO_RCVTIMEO` de 2 s. Lee hasta que la request esté `complete`, con un máximo de 4096 bytes; si se excede → 400.
  - Ruteo:
    - `GET /`: 200 con la página HTML de abajo, aunque el token del query no coincida (el token se valida en el POST).
    - `POST /join`: llama a `ValidateJoin`.
      - `BadToken` → 403 con `"Código QR vencido: vuelve a escanear"`.
      - `BadId` → 400 con `"Network ID inválido (16 caracteres hex)"`.
      - `Ok` → guarda `{nwid,name}` en `m_pending` bajo `std::mutex`, marca `m_hasPending=true` y responde 200 con `"Listo. La Switch se está conectando a <nwid>."`.
    - Otro path → 404. Otro método en `/join` → 405.
  - Cierra la conexión tras cada respuesta.
- `void Stop()`: `m_stop=true`, `threadWaitForExit` + `threadClose`, `close(fd)`, `socketExit()` y `nifmExit()`. Es idempotente y se llama en el destructor.
- `bool TakePending(std::string& nwid, std::string& name)`: lee y limpia `m_pending` bajo el mutex.
  - La escritura del `.ini` **no** se hace en el hilo del servidor: la hace la GUI en el hilo principal (`update()`), porque `ult::setIniFileValue` no está documentado como thread-safe.
- Página HTML: constante `kFormHtml` en `http_server.cpp`, autocontenida, en español, ≤3 KB, estilo de la opción 2 (tema `#0e162c`/`#6c8cff`, botones con borde completo). Contenido:
  - Título "ZeroTier NX".
  - Campo "Network ID" (`name="nwid"`, `maxlength=16`, `pattern="[0-9a-fA-F]{16}"`, `autocapitalize=off`).
  - Campo opcional "Nombre de la red" (`name="name"`).
  - Campo oculto `t`, que un script de 1 línea llena desde `location.search`.
  - Botón "Conectar". Envío `POST /join` como `application/x-www-form-urlencoded`.

### 6. GUI del overlay: `source/main.cpp` (opción 2 "Ultrahand moderno")
Se copia la estructura de `sys-GRID0/overlay/source/main.cpp`:
- `#define TESLA_INIT_IMPL`, `exception_wrap.hpp` y `tesla.hpp`.
- `tsl::loop<Overlay, tsl::impl::LaunchFlags::CloseOnExit>`.
- `initServices` crea el directorio de config.
- Botón B en la raíz: `tsl::setNextOverlay(ult::OVERLAY_PATH + "ovlmenu.ovl")` + `close()`.

Las 5 pantallas son clases `tsl::Gui`, todas con `OverlayFrame("ZeroTier NX", APP_VERSION)`:
- **`MainGui` (Menú):**
  - Primero un `CustomDrawer` de 70 px de alto, que es el encabezado de la opción 2:
    - Línea `● <state> · <ip> · <ping> ms`, con `drawString` de 17 px: el punto en `#4f7cff` si está conectado y en gris si no.
    - Sparkline de 140×34 px a la derecha: polilínea de los últimos 12 `avgLatencyMs` (buffer circular de la GUI, un punto por refresco de 5 s), dibujada con segmentos `drawRect` de 2 px. Si hay menos de 2 muestras, no se dibuja.
  - Ítems:
    - `CategoryHeader("Conexión (aplica al reiniciar)")`.
    - `ToggleListItem("Conexión", BootEnabled())` → `SetBootEnabled`.
    - `ListItem("Estado")` → `StatusGui`.
    - `ListItem("Iniciar sesión con QR")` → `QrGui`.
    - `ListItem("Iniciar sesión manual")` → `ManualGui`.
    - `ListItem("Redes guardadas")` → `NetworksGui`.
    - `ListItem("Acerca de")` → `AboutGui`.
  - Refresca `ReadSnapshot()` cada 1 s en `update()`, el mismo patrón de `armGetSystemTick` de sys-GRID0.
- **`StatusGui`:** `ListItem`s no clicables con Estado, IP, Node ID, Peers (`count`), Ping (`avg` ms o `—`) y Red (nwid en mayúsculas o `Ninguna`). Refresco de 1 s.
- **`QrGui`:**
  - Al construirse llama a `m_server.Start()`. Si arranca, codifica la URL con `qrcodegen_encodeText(url, tmp, qr, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true)`.
  - Un `CustomDrawer`:
    1. Fondo blanco de 276×276 px centrado, con zona de silencio de 4 módulos y módulos negros de `floor(276/(size+8))` px.
    2. Debajo, "Escanea con tu teléfono" (19 px), la URL en monospace a 14 px y el estado.
  - Estados:
    - Esperando.
    - `"Conectando a <NWID>…"` después de `TakePending` → `SaveAndSelectNetwork`.
    - `m_error` si `Start()` falló; en ese caso se muestra el texto de error en vez del QR.
  - `update()` llama a `TakePending` cada frame. El destructor llama a `Stop()`.
- **`ManualGui`:** copia de `NetworkEditorGui` de sys-GRID0 (líneas ~143–325 del mismo archivo; leerla completa al implementar).
  - Cambios: todos los literales visibles traducidos al español ("Network ID", "Ⓐ Escribir", "Ⓧ Borrar", "Ⓨ Limpiar", "+ Unirse").
  - Al validar llama a `SaveAndSelectNetwork(nwid, nwid)`.
  - Se mantienen los controles del original: D-pad/A/X/Y/+ y teclado USB.
- **`NetworksGui`:**
  - Un `ListItem` por `SavedNetworks()`, con valor `"Activa"` si es la seleccionada o el nwid en mayúsculas.
  - A → `SelectNetwork` + valor `"Cambiando…"`. Y → `ForgetNetwork` + `tsl::goBack()` + `tsl::changeTo<NetworksGui>()` para recargar.
  - Sin redes → un `ListItem("Sin redes guardadas")`.
- **`AboutGui`:** `ListItem`s con versión, "Backend: sys-GRID0" (con `"no detectado"` si falta `status.txt`) y las licencias GPL-2.0 / MPL-2.0 / MIT.

Ítems sin íconos: libultrahand no ofrece íconos por `ListItem` en las APIs verificadas.

### 7. Tests de host: `tests/Makefile` y `tests/core_test.cpp`
- `tests/Makefile`: el objetivo `run` hace `c++ -std=c++20 -Wall -Wextra -I../source ../source/core.cpp core_test.cpp -o core_test && ./core_test`.
- `core_test.cpp` usa una macro `CHECK(cond)` que cuenta fallos; sale con código 1 si hubo fallos e imprime `OK <n> checks`. Casos mínimos:
  - `IsHexNetworkId`: `"8056c2e21c000001"` → true; `"0000000000000000"`, `"8056c2e21c00000"` y `"8056c2e21c00000g"` → false.
  - `StatusValue`: sobre el bloque `"network   8056c2e21c000001\nonline    yes\nnetstatus OK\naddress   10.147.17.42/24\n"` devuelve `"yes"` para `online` y `"10.147.17.42/24"` para `address`. La clave `onlinex` no confunde a `online`.
  - `ParsePeers`: `"peers 3   (uptime 10 s)\na1b2c3d4e5 role 0  latency 20  paths 1  v1.14.0\n    1.2.3.4:9993 …\nb1b2c3d4e5 role 0  latency 30  paths 1  v1.14.0\nc1c2c3d4e5 role 2  latency 90  paths 1  v1.14.0\n"` → `{3, 25}`. `""` → `{0,-1}`.
  - `NodeIdFromIdentity("a1b2c3d4e5:0:abcd…")` → `"a1b2c3d4e5"`.
  - `DisplayAddress("0.0.0.0/0")` → `"Sin asignar"` y `"10.147.17.42/24"` → `"10.147.17.42"`.
  - `ParseHttpRequest`: un POST con `Content-Length: 30` y body parcial da `complete=false`; con el body completo da `complete=true`, `path="/join"`.
  - `FormValue("nwid=8056c2e21c000001&name=Casa+LAN%21","name")` → `"Casa LAN!"`.
  - `ValidateJoin`:
    - Token correcto con nwid en mayúsculas → `Ok`, nwid en minúsculas y name `"Casa LAN"` (`!` filtrado).
    - Token incorrecto → `BadToken`.
    - nwid de 15 caracteres → `BadId`.
  - `HttpResponse(404,…)` empieza con `"HTTP/1.1 404 Not Found\r\n"`.

### 8. Repo privado y primer commit
Después de que pasen el paso 7 y el build (ver Verification):
1. `git add -A`.
2. `git commit -m "feat: overlay ZeroTier NX (opción 2) sobre sys-GRID0 con login QR local"`, con la línea de atribución `Claude-Session: https://claude.ai/code/session_015JtqzR8saJAewAR1stPfiU` al final del mensaje.
3. `gh repo create Jul879n/zerotier-nx --private --source ~/Documents/zerotier-nx --remote origin --push --description "Overlay Ultrahand para ZeroTier en Switch (backend sys-GRID0)"`.

Si el nombre ya estuviera tomado al momento de crearlo, se usa `zerotier-nx-overlay`.

## Critical files & anchors
- `sys-GRID0/overlay/source/main.cpp` (GitHub, `main`): `NetworkEditorGui` (~143–325) para copiar a `ManualGui`; `MainGui::handleInput` para el cierre hacia `ovlmenu.ovl`; `SetBootEnabled` para el toggle.
- `sys-GRID0/overlay/Makefile`: base del `Makefile`, sobre todo el flag `--nacp=` y `printf 'ULTR'`.
- `sys-GRID0/source/zt_port.cpp`: el `snprintf` de `status.txt` (~2110–2165) y de `peers.txt` (~1870–1902) definen los formatos que parsea `core.cpp`.
- `libultrahand/libtesla/include/tesla.hpp@856ddbd`: `CustomDrawer` (~6208), `Renderer::drawRect` (~2021) y `drawString` (~4077).
- `libultrahand/libultra/source/download_funcs.cpp@856ddbd`: `syncNtp` (~175–195), el patrón de `socketInitialize`/`socketExit`.

## Verification
Prerrequisito: directorio `~/Documents/zerotier-nx`.
1. **Tests de host (comportamiento nuevo):** `make -C tests run` debe imprimir `OK <n> checks` con código de salida 0. Cubre el parseo de estado, peers e identidad, el HTTP y la validación del login QR.
2. **Build del overlay:** `DEVKITPRO=/opt/devkitpro PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitA64/bin:$PATH make -j8` debe generar `zerotier-nx.ovl`. Comprobar:
   - `tail -c 4 zerotier-nx.ovl` → `ULTR`.
   - `strings zerotier-nx.ovl | grep -c "ZeroTier NX"` ≥ 1, lo que confirma que se embebió el NACP.
3. **Servidor HTTP en el host:** no aplica, porque depende de libnx. La lógica de ruteo y validación queda cubierta por el paso 1.
4. **Repo:** `gh repo view Jul879n/zerotier-nx --json visibility,url` → `"visibility":"PRIVATE"`. Además, `git -C ~/Documents/zerotier-nx submodule status` muestra `856ddbd…` en `lib/libultrahand`.
5. **Checklist en consola** (pendiente, el usuario no tiene la consola a mano; se deja en `README.md`, sección "Prueba en consola"):
   - a) Con sys-GRID0 v0.1.2 instalado, el overlay aparece en Ultrahand como "ZeroTier NX".
   - b) Estado muestra IP, Node ID, Peers y Ping.
   - c) En Login QR, el teléfono (misma Wi‑Fi) escanea, envía el ID `8056c2e21c000001` y en ≤2 s el overlay muestra "Conectando a 8056C2E21C000001…"; `networks.ini` tiene la sección nueva y `selected`.
   - d) El POST con un token viejo responde 403.
   - e) El login manual guarda y conecta.
   - f) Redes guardadas: A cambia de red, Y la olvida.
   - g) El toggle Conexión renombra `boot2.flag`.

## Assumptions & contingencies
- **Backend:** sys-GRID0 sin modificar. Si en la consola `status.txt` no tuviera las claves leídas aquí (versión distinta), se fija el requisito a `v0.1.2` en el README y se ajusta `core.cpp` al formato real.
- **Toggle "Conexión":** controla el arranque del sysmodule (aplica al reiniciar). sys-GRID0 no ofrece desconexión en vivo, porque una MITM no se puede desregistrar con sesiones abiertas.
- **Sockets del overlay:** si `socketInitialize` falla porque libultrahand ya inicializó sockets en ese proceso, se trata `rc` de "ya inicializado" como éxito y en ese caso no se llama a `socketExit()` en `Stop()`. Si los buffers TCP de 0x1000 no bastan o la memoria del overlay no alcanza (falla `socketInitialize` por falta de memoria), se baja a `tcp_*_buf_size=0x800`.
- **Node ID:** si `identity.public` no tiene el formato `<10hex>:`, Estado muestra `—` en Node ID. No bloquea el MVP.
- **Theme:** se usa el tema activo de Ultrahand; no se envía `theme.ini` propio (la ruta por overlay no está verificada). Los colores de la opción 2 se aplican solo en el encabezado `CustomDrawer` y en la página HTML.
- **Licencias:** el repo es GPL-2.0 por libultrahand. El código adaptado de sys-GRID0 conserva la atribución MPL-2.0 en la cabecera de `sysgrid.cpp`.
