#include "core.hpp"

#include <cstdlib>
#include <cstring>

namespace ztnx {

namespace {

bool IsHexChar(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int HexVal(char c)
{
    if (c >= '0' && c <= '9') { return c - '0'; }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    return -1;
}

std::string Trim(const std::string &s)
{
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) { ++b; }
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) { --e; }
    return s.substr(b, e - b);
}

std::string Lower(std::string s)
{
    for (char &c : s) {
        if (c >= 'A' && c <= 'Z') { c = static_cast<char>(c - 'A' + 'a'); }
    }
    return s;
}

// Valor entero tras `word` (separado por espacios) dentro de una línea; false si falta.
bool IntAfter(const std::string &line, const char *word, int &out)
{
    const std::string needle = std::string(" ") + word + " ";
    const size_t pos = line.find(needle);
    if (pos == std::string::npos) { return false; }
    const char *p = line.c_str() + pos + needle.size();
    char *end = nullptr;
    const long v = std::strtol(p, &end, 10);
    if (end == p) { return false; }
    out = static_cast<int>(v);
    return true;
}

}  // namespace

bool IsHexNetworkId(const std::string &value)
{
    if (value.size() != 16) { return false; }
    bool nonzero = false;
    for (const char c : value) {
        if (!IsHexChar(c)) { return false; }
        nonzero |= c != '0';
    }
    return nonzero;
}

std::string ToLowerHex(std::string value)
{
    for (char &c : value) {
        if (c >= 'A' && c <= 'F') { c = static_cast<char>(c - 'A' + 'a'); }
    }
    return value;
}

std::string ToUpperHex(std::string value)
{
    for (char &c : value) {
        if (c >= 'a' && c <= 'f') { c = static_cast<char>(c - 'a' + 'A'); }
    }
    return value;
}

std::string StatusValue(const std::string &text, const std::string &key)
{
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        if (nl == std::string::npos) { nl = text.size(); }
        const std::string line = text.substr(pos, nl - pos);
        pos = nl + 1;

        size_t p = 0;
        while (p < line.size() && (line[p] == ' ' || line[p] == '\t')) { ++p; }
        if (line.compare(p, key.size(), key) != 0) { continue; }
        const size_t after = p + key.size();
        if (after >= line.size() || (line[after] != ' ' && line[after] != '\t')) { continue; }
        return Trim(line.substr(after));
    }
    return {};
}

PeerSummary ParsePeers(const std::string &text)
{
    PeerSummary out;
    long sum = 0;
    int n = 0;
    bool first = true;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t nl = text.find('\n', pos);
        if (nl == std::string::npos) { nl = text.size(); }
        std::string line = text.substr(pos, nl - pos);
        pos = nl + 1;
        if (!line.empty() && line.back() == '\r') { line.pop_back(); }
        if (line.empty()) { continue; }

        if (first) {
            first = false;
            if (line.compare(0, 6, "peers ") != 0) { return {}; }
            out.count = static_cast<unsigned>(std::strtoul(line.c_str() + 6, nullptr, 10));
            continue;
        }
        if (line[0] == ' ' || line[0] == '\t') { continue; }  // líneas de path
        int role = -1, latency = -1;
        if (!IntAfter(line, "role", role) || !IntAfter(line, "latency", latency)) { continue; }
        if (role == 0 && latency >= 0) {
            sum += latency;
            ++n;
        }
    }
    if (n > 0) { out.avgLatencyMs = static_cast<int>(sum / n); }
    return out;
}

std::string NodeIdFromIdentity(const std::string &text)
{
    if (text.size() < 11 || text[10] != ':') { return {}; }
    for (size_t i = 0; i < 10; ++i) {
        if (!IsHexChar(text[i])) { return {}; }
    }
    return text.substr(0, 10);
}

std::string DisplayAddress(const std::string &address)
{
    if (address.empty() || address.rfind("0.0.0.0", 0) == 0) { return "Sin asignar"; }
    const size_t slash = address.find('/');
    return slash == std::string::npos ? address : address.substr(0, slash);
}

bool KeepLastOnEmpty(unsigned &emptyStreak, bool empty, unsigned limit)
{
    if (!empty) {
        emptyStreak = 0;
        return false;
    }
    return ++emptyStreak < limit;
}

std::string DisplayState(const std::string &online, const std::string &netstatus)
{
    if (online.empty()) { return "sys-GRID0 no detectado"; }
    // netstatus: minúsculas con guiones, a veces con una explicación entre paréntesis (zt_port.cpp).
    if (netstatus.rfind("requesting-configuration", 0) == 0) { return "Solicitando configuración…"; }
    if (netstatus.rfind("access-denied", 0) == 0) { return "Autoriza este nodo en ZeroTier Central"; }
    if (netstatus.rfind("not-found", 0) == 0) { return "Red no encontrada: revisa el ID"; }
    if (online == "yes") {
        return (netstatus.empty() || netstatus == "ok") ? "Conectado" : netstatus;
    }
    return "Conectando…";
}

HttpRequest ParseHttpRequest(const std::string &raw)
{
    HttpRequest req;
    const size_t headEnd = raw.find("\r\n\r\n");
    if (headEnd == std::string::npos) { return req; }

    const std::string head = raw.substr(0, headEnd);
    const size_t lineEnd = head.find("\r\n");
    const std::string reqLine = head.substr(0, lineEnd);
    const size_t sp1 = reqLine.find(' ');
    if (sp1 == std::string::npos) { return req; }
    const size_t sp2 = reqLine.find(' ', sp1 + 1);
    req.method = reqLine.substr(0, sp1);
    const std::string target = reqLine.substr(sp1 + 1, sp2 == std::string::npos ? std::string::npos : sp2 - sp1 - 1);
    const size_t q = target.find('?');
    req.path = target.substr(0, q);
    if (q != std::string::npos) { req.query = target.substr(q + 1); }

    size_t contentLength = 0;
    size_t pos = lineEnd == std::string::npos ? head.size() : lineEnd + 2;
    while (pos < head.size()) {
        size_t nl = head.find("\r\n", pos);
        if (nl == std::string::npos) { nl = head.size(); }
        const std::string line = head.substr(pos, nl - pos);
        pos = nl + 2;
        const size_t colon = line.find(':');
        if (colon == std::string::npos) { continue; }
        if (Lower(line.substr(0, colon)) == "content-length") {
            contentLength = static_cast<size_t>(std::strtoul(Trim(line.substr(colon + 1)).c_str(), nullptr, 10));
        }
    }

    const std::string body = raw.substr(headEnd + 4);
    if (body.size() >= contentLength) {
        req.body = body.substr(0, contentLength);
        req.complete = true;
    } else {
        req.body = body;
    }
    return req;
}

std::string FormValue(const std::string &urlencoded, const std::string &key)
{
    size_t pos = 0;
    while (pos <= urlencoded.size()) {
        size_t amp = urlencoded.find('&', pos);
        if (amp == std::string::npos) { amp = urlencoded.size(); }
        const std::string pair = urlencoded.substr(pos, amp - pos);
        pos = amp + 1;

        const size_t eq = pair.find('=');
        if (eq == std::string::npos || pair.compare(0, eq, key) != 0) { continue; }
        std::string out;
        for (size_t i = eq + 1; i < pair.size(); ++i) {
            const char c = pair[i];
            if (c == '+') {
                out += ' ';
            } else if (c == '%' && i + 2 < pair.size() && HexVal(pair[i + 1]) >= 0 && HexVal(pair[i + 2]) >= 0) {
                out += static_cast<char>(HexVal(pair[i + 1]) * 16 + HexVal(pair[i + 2]));
                i += 2;
            } else {
                out += c;
            }
        }
        return out;
    }
    return {};
}

std::string HttpResponse(int status, const std::string &contentType, const std::string &body)
{
    const char *text = "OK";
    switch (status) {
        case 200: text = "OK"; break;
        case 400: text = "Bad Request"; break;
        case 403: text = "Forbidden"; break;
        case 404: text = "Not Found"; break;
        case 405: text = "Method Not Allowed"; break;
        default: break;
    }
    return "HTTP/1.1 " + std::to_string(status) + " " + text + "\r\nContent-Type: " + contentType +
           "\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
}

JoinResult ValidateJoin(const std::string &body, const std::string &token,
                        std::string &outNwid, std::string &outName)
{
    if (token.empty() || FormValue(body, "t") != token) { return JoinResult::BadToken; }

    std::string nwid;
    for (const char c : FormValue(body, "nwid")) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') { nwid += c; }
    }
    if (!IsHexNetworkId(nwid)) { return JoinResult::BadId; }
    outNwid = ToLowerHex(nwid);

    std::string name;
    for (const char c : FormValue(body, "name")) {
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                        c == ' ' || c == '_' || c == '-';
        if (ok) { name += c; }
    }
    if (name.size() > 32) { name.resize(32); }
    name = Trim(name);
    outName = name.empty() ? outNwid : name;
    return JoinResult::Ok;
}

}  // namespace ztnx
