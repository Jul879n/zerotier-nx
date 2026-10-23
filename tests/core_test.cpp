#include "core.hpp"

#include <cstdio>
#include <string>

using namespace ztnx;

static int g_checks = 0, g_fails = 0;
#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_fails; std::printf("FALLO %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

int main()
{
    CHECK(IsHexNetworkId("8056c2e21c000001"));
    CHECK(!IsHexNetworkId("0000000000000000"));
    CHECK(!IsHexNetworkId("8056c2e21c00000"));
    CHECK(!IsHexNetworkId("8056c2e21c00000g"));
    CHECK(ToUpperHex("8056c2e21c000001") == "8056C2E21C000001");
    CHECK(ToLowerHex("8056C2E21C000001") == "8056c2e21c000001");

    const std::string status = "network   8056c2e21c000001\nonline    yes\nnetstatus OK\naddress   10.147.17.42/24\n";
    CHECK(StatusValue(status, "online") == "yes");
    CHECK(StatusValue(status, "address") == "10.147.17.42/24");
    CHECK(StatusValue(status, "nada").empty());
    CHECK(StatusValue("onlinex no\nonline yes\r\n", "online") == "yes");
    CHECK(StatusValue("onlinex no\n", "online").empty());

    const std::string peers = "peers 3   (uptime 10 s)\na1b2c3d4e5 role 0  latency 20  paths 1  v1.14.0\n    1.2.3.4:9993 role 0 latency 999\nb1b2c3d4e5 role 0  latency 30  paths 1  v1.14.0\nc1c2c3d4e5 role 2  latency 90  paths 1  v1.14.0\n";
    PeerSummary ps = ParsePeers(peers);
    CHECK(ps.count == 3 && ps.avgLatencyMs == 25);
    ps = ParsePeers("");
    CHECK(ps.count == 0 && ps.avgLatencyMs == -1);
    ps = ParsePeers("basura\n");
    CHECK(ps.count == 0 && ps.avgLatencyMs == -1);

    CHECK(NodeIdFromIdentity("a1b2c3d4e5:0:abcd") == "a1b2c3d4e5");
    CHECK(NodeIdFromIdentity("zzzzzzzzzz:0:abcd").empty());
    CHECK(NodeIdFromIdentity("").empty());

    unsigned streak = 0;
    CHECK(!KeepLastOnEmpty(streak, false));
    CHECK(KeepLastOnEmpty(streak, true));
    CHECK(KeepLastOnEmpty(streak, true));
    CHECK(!KeepLastOnEmpty(streak, true));
    CHECK(!KeepLastOnEmpty(streak, false));
    CHECK(KeepLastOnEmpty(streak, true));

    CHECK(DisplayAddress("0.0.0.0/0") == "Sin asignar");
    CHECK(DisplayAddress("") == "Sin asignar");
    CHECK(DisplayAddress("10.147.17.42/24") == "10.147.17.42");

    CHECK(DisplayState("", "") == "sys-GRID0 no detectado");
    // Valores reales de status.txt (zt_port.cpp): minúsculas con guiones.
    CHECK(DisplayState("yes", "ok") == "Conectado");
    CHECK(DisplayState("yes", "") == "Conectado");
    CHECK(DisplayState("no", "ok") == "Conectando…");
    CHECK(DisplayState("no", "requesting-configuration") == "Solicitando configuración…");
    CHECK(DisplayState("yes", "requesting-configuration") == "Solicitando configuración…");
    CHECK(DisplayState("no", "access-denied (authorise this node in ZeroTier Central)") == "Autoriza este nodo en ZeroTier Central");
    CHECK(DisplayState("no", "not-found (check the network id in config.ini)") == "Red no encontrada: revisa el ID");
    CHECK(DisplayState("yes", "port-error") == "port-error");

    const std::string body = "t=ABC234&nwid=8056C2E21C000001&name=Casa+LAN%21";
    const std::string head = "POST /join HTTP/1.1\r\ncontent-length: " + std::to_string(body.size()) + "\r\n\r\n";
    HttpRequest r = ParseHttpRequest(head + body.substr(0, 30));
    CHECK(!r.complete);
    r = ParseHttpRequest(head + body);
    CHECK(r.complete && r.path == "/join" && r.method == "POST" && r.body == body);
    r = ParseHttpRequest("GET /?t=XYZ HTTP/1.1\r\nHost: x\r\n\r\n");
    CHECK(r.complete && r.path == "/" && r.query == "t=XYZ");
    CHECK(!ParseHttpRequest("GET / HTTP/1.1\r\nHost").complete);

    CHECK(FormValue("nwid=8056c2e21c000001&name=Casa+LAN%21", "name") == "Casa LAN!");
    CHECK(FormValue("a=1", "b").empty());

    std::string nwid, name;
    CHECK(ValidateJoin(body, "ABC234", nwid, name) == JoinResult::Ok);
    CHECK(nwid == "8056c2e21c000001" && name == "Casa LAN");
    CHECK(ValidateJoin(body, "OTRO11", nwid, name) == JoinResult::BadToken);
    CHECK(ValidateJoin("t=ABC234&nwid=8056c2e21c00000", "ABC234", nwid, name) == JoinResult::BadId);
    CHECK(ValidateJoin("t=ABC234&nwid=8056c2e21c000001", "ABC234", nwid, name) == JoinResult::Ok && name == nwid);

    CHECK(HttpResponse(404, "text/plain", "x").rfind("HTTP/1.1 404 Not Found\r\n", 0) == 0);

    if (g_fails) { return 1; }
    std::printf("OK %d checks\n", g_checks);
    return 0;
}
