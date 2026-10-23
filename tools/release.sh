#!/bin/sh
# Paquete completo para la SD (módulo + overlay) y release en GitHub.
# Uso: [RELEASE_REPO=usuario/repo] tools/release.sh <tag>   Requiere: overlay y módulo compilados, gh autenticado.
set -eu
TAG="${1:?uso: tools/release.sh <tag>}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export RELEASE_REPO="${RELEASE_REPO:-$(git -C "$ROOT" remote get-url origin | sed 's#.*github.com[/:]##; s#\.git$##')}"
exec "$ROOT/module/tools/release.sh" "$TAG" "$ROOT/zerotier-nx.ovl"
