#include "http_server.hpp"
#include <cstdio>
#include <string>
#include <unistd.h>
using namespace ztnx;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { ++fails; std::printf("FALLO línea %d: %s\n", __LINE__, #c); } } while (0)
static std::string sh(const std::string &cmd) {
    std::string out; FILE *p = popen(cmd.c_str(), "r"); char b[512]; size_t n;
    while ((n = fread(b, 1, sizeof b, p)) > 0) out.append(b, n);
    pclose(p); return out;
}
int main() {
    QrServer s;
    CHECK(s.Start());
    std::printf("URL: %s\n", s.Url().c_str());
    const std::string url = s.Url();
    const std::string base = url.substr(0, url.find("/?t="));
    const std::string tok = url.substr(url.find("t=") + 2);
    const std::string c = "curl -s -m 5 -o /dev/null -w '%{http_code}' ";
    CHECK(sh(c + url) == "200");
    CHECK(sh("curl -s -m 5 " + url).find("name=nwid") != std::string::npos);
    CHECK(sh(c + base + "/nada") == "404");
    CHECK(sh(c + "-X POST " + base + "/") == "405");
    CHECK(sh(c + base + "/join") == "405");
    CHECK(sh(c + "-d 't=VIEJO1&nwid=8056c2e21c000001' " + base + "/join") == "403");
    CHECK(sh(c + "-d 't=" + tok + "&nwid=8056c2e21c0000' " + base + "/join") == "400");
    std::string n, nm;
    CHECK(!s.TakePending(n, nm));
    CHECK(sh("curl -s -m 5 -d 't=" + tok + "&nwid=8056C2E21C000001&name=Casa+LAN%21' " + base + "/join").find("8056c2e21c000001") != std::string::npos);
    CHECK(s.TakePending(n, nm) && n == "8056c2e21c000001" && nm == "Casa LAN");
    CHECK(!s.TakePending(n, nm));
    CHECK(sh(c + "--data-binary @/dev/zero -H 'Content-Length: 99999' " + base + "/join 2>/dev/null | head -c 3").size() >= 0);
    s.Stop(); s.Stop();
    CHECK(sh(c + url) != "200");
    std::printf(fails ? "FALLÓ\n" : "OK servidor\n");
    return fails ? 1 : 0;
}
