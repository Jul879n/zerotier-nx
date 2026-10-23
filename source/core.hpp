// Lógica pura (sin libnx ni tesla): parseo de archivos de sys-GRID0 y de HTTP.
#pragma once

#include <string>

namespace ztnx {

bool IsHexNetworkId(const std::string &value);
std::string ToLowerHex(std::string value);
std::string ToUpperHex(std::string value);

// Valor de `clave<espacios>valor` en un texto; "" si no está.
std::string StatusValue(const std::string &text, const std::string &key);

struct PeerSummary {
    unsigned count = 0;
    int avgLatencyMs = -1;
};
PeerSummary ParsePeers(const std::string &text);

std::string NodeIdFromIdentity(const std::string &text);
std::string DisplayAddress(const std::string &address);
std::string DisplayState(const std::string &online, const std::string &netstatus);

// sys-GRID0 reescribe status.txt (borra y crea), así que una lectura vacía aislada no
// implica que el backend no esté. Devuelve true mientras haya que conservar el último
// estado bueno; deja de hacerlo tras `limit` lecturas vacías seguidas.
bool KeepLastOnEmpty(unsigned &emptyStreak, bool empty, unsigned limit = 3);

struct HttpRequest {
    std::string method, path, query, body;
    bool complete = false;
};
HttpRequest ParseHttpRequest(const std::string &raw);
std::string FormValue(const std::string &urlencoded, const std::string &key);
std::string HttpResponse(int status, const std::string &contentType, const std::string &body);

enum class JoinResult { Ok, BadToken, BadId };
JoinResult ValidateJoin(const std::string &body, const std::string &token,
                        std::string &outNwid, std::string &outName);

}  // namespace ztnx
