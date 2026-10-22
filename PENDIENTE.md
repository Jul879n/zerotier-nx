# Pendiente

Todo requiere consola con Atmosphère + Ultrahand + sys-GRID0 v0.1.2. Checklist funcional en `README.md`.

## Supuestos sin verificar
- `nifmGetCurrentIpAddress` entrega la IP en orden de red. Si la URL del QR sale invertida, corregir en `QrServer::Start`.
- `identity.public` empieza con `<10hex>:`. Si no, Node ID muestra `—`.
- El click listener de `ListItem` recibe `KEY_Y` (Redes guardadas: olvidar). Si no, mover a `handleInput`.
- `socketInitialize` con buffers TCP de 0x1000 alcanza en el overlay; si falla, bajar a 0x800.
- Formatos de `status.txt` y `peers.txt` según v0.1.2; si difieren, ajustar `core.cpp`.
- Compartir sockets con libultrahand: si ya estaban inicializados, se trata como éxito y no se llama a `socketExit()`.

## No cubierto por tests de host
- Dibujo de las 5 pantallas (cabecera, sparkline, QR) y su aspecto con el tema activo.
- Servidor HTTP sobre `bsd:u` real.
- Toggle Conexión (`boot2.flag`) y cambio de red en vivo.

## Mejoras posibles
- Íconos por ítem (libultrahand no los ofrece en las APIs verificadas).
- `theme.ini` propio con los colores de la opción 2.
