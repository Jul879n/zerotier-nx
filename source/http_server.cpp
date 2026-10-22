#include "http_server.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "core.hpp"

namespace ztnx {

namespace {

constexpr const char *kFormHtml =
    "<!doctype html><html lang=es><head><meta charset=utf-8>"
    "<meta name=viewport content='width=device-width,initial-scale=1'><title>ZeroTier NX</title>"
    "<style>body{margin:0;padding:24px;background:#0e162c;color:#e8ecff;font:16px system-ui,sans-serif}"
    "h1{font-size:22px;margin:0 0 20px}label{display:block;margin:14px 0 6px;color:#9fb0e8}"
    "input{width:100%;box-sizing:border-box;padding:12px;font:inherit;color:inherit;background:#162244;"
    "border:2px solid #2b3d78;border-radius:10px}input:focus{outline:none;border-color:#6c8cff}"
    "button{width:100%;margin-top:22px;padding:14px;font:inherit;font-weight:600;color:#fff;"
    "background:#4f7cff;border:2px solid #6c8cff;border-radius:10px}</style></head><body>"
    "<h1>ZeroTier NX</h1><form method=post action=/join>"
    "<label for=n>Network ID</label>"
    "<input id=n name=nwid maxlength=16 pattern='[0-9a-fA-F]{16}' autocapitalize=off autocomplete=off required>"
    "<label for=m>Nombre de la red (opcional)</label><input id=m name=name maxlength=32>"
    "<input type=hidden name=t id=t><button>Conectar</button></form>"
    "<script>document.getElementById('t').value=new URLSearchParams(location.search).get('t')||''</script>"
    "</body></html>";

constexpr const char kTokenChars[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

}  // namespace

bool QrServer::Start()
{
    Stop();
    m_error.clear();
    m_stop = false;
    m_hasPending = false;

    if (R_FAILED(nifmInitialize(NifmServiceType_User))) {
        m_error = "Conecta la consola a Wi‑Fi";
        return false;
    }
    m_nifm = true;
    u32 ip = 0;
    if (R_FAILED(nifmGetCurrentIpAddress(&ip)) || ip == 0) {
        m_error = "Conecta la consola a Wi‑Fi";
        Stop();
        return false;
    }

    const SocketInitConfig cfg = {
        .tcp_tx_buf_size = 0x1000,
        .tcp_rx_buf_size = 0x1000,
        .tcp_tx_buf_max_size = 0,
        .tcp_rx_buf_max_size = 0,
        .udp_tx_buf_size = 512,
        .udp_rx_buf_size = 512,
        .sb_efficiency = 1,
        .bsd_service_type = BsdServiceType_Auto,
    };
    const Result rc = socketInitialize(&cfg);
    if (R_SUCCEEDED(rc)) {
        m_ownSocket = true;
    } else if (rc != MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized)) {
        m_error = "No se pudo iniciar la red";
        Stop();
        return false;
    }

    m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    int port = 0;
    if (m_fd >= 0) {
        const int one = 1;
        ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        for (int p = 8080; p <= 8089; ++p) {
            sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(static_cast<uint16_t>(p));
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (::bind(m_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0) {
                port = p;
                break;
            }
            if (errno != EADDRINUSE) { break; }
        }
    }
    if (port == 0 || ::listen(m_fd, 2) != 0) {
        m_error = "Puerto ocupado";
        Stop();
        return false;
    }

    char raw[6];
    randomGet(raw, sizeof(raw));
    m_token.clear();
    for (const char c : raw) {
        m_token += kTokenChars[static_cast<unsigned char>(c) % (sizeof(kTokenChars) - 1)];
    }

    in_addr a = {};
    a.s_addr = ip;  // nifm entrega la IP en orden de red
    m_url = std::string("http://") + inet_ntoa(a) + ":" + std::to_string(port) + "/?t=" + m_token;

    if (R_FAILED(threadCreate(&m_thread, Loop, this, nullptr, 0x4000, 0x2C, -2)) ||
        R_FAILED(threadStart(&m_thread))) {
        m_error = "No se pudo iniciar el servidor";
        Stop();
        return false;
    }
    m_threadStarted = true;
    return true;
}

void QrServer::Stop()
{
    m_stop = true;
    if (m_threadStarted) {
        threadWaitForExit(&m_thread);
        threadClose(&m_thread);
        m_threadStarted = false;
    }
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    if (m_ownSocket) {
        socketExit();
        m_ownSocket = false;
    }
    if (m_nifm) {
        nifmExit();
        m_nifm = false;
    }
}

bool QrServer::TakePending(std::string &nwid, std::string &name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_hasPending) { return false; }
    nwid = m_pendingNwid;
    name = m_pendingName;
    m_hasPending = false;
    return true;
}

void QrServer::Loop(void *arg)
{
    auto *self = static_cast<QrServer *>(arg);
    while (!self->m_stop) {
        pollfd pfd = {self->m_fd, POLLIN, 0};
        if (::poll(&pfd, 1, 200) <= 0 || !(pfd.revents & POLLIN)) { continue; }
        const int client = ::accept(self->m_fd, nullptr, nullptr);
        if (client < 0) { continue; }
        self->Serve(client);
        ::close(client);
    }
}

void QrServer::Serve(int client)
{
    timeval tv = {2, 0};
    ::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::string raw;
    HttpRequest req;
    char buf[1024];
    while (!req.complete) {
        const ssize_t n = ::recv(client, buf, sizeof(buf), 0);
        if (n <= 0) { return; }
        raw.append(buf, static_cast<size_t>(n));
        if (raw.size() > 4096) {
            const std::string r = HttpResponse(400, "text/plain; charset=utf-8", "Solicitud demasiado grande");
            ::send(client, r.data(), r.size(), 0);
            return;
        }
        req = ParseHttpRequest(raw);
    }

    std::string resp;
    const char *text = "text/plain; charset=utf-8";
    if (req.path == "/") {
        resp = req.method == "GET" ? HttpResponse(200, "text/html; charset=utf-8", kFormHtml)
                                   : HttpResponse(405, text, "Método no permitido");
    } else if (req.path == "/join") {
        if (req.method != "POST") {
            resp = HttpResponse(405, text, "Método no permitido");
        } else {
            std::string nwid, name;
            switch (ValidateJoin(req.body, m_token, nwid, name)) {
                case JoinResult::BadToken:
                    resp = HttpResponse(403, text, "Código QR vencido: vuelve a escanear");
                    break;
                case JoinResult::BadId:
                    resp = HttpResponse(400, text, "Network ID inválido (16 caracteres hex)");
                    break;
                case JoinResult::Ok: {
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_pendingNwid = nwid;
                        m_pendingName = name;
                        m_hasPending = true;
                    }
                    resp = HttpResponse(200, text, "Listo. La Switch se está conectando a " + nwid + ".");
                    break;
                }
            }
        }
    } else {
        resp = HttpResponse(404, text, "No encontrado");
    }
    ::send(client, resp.data(), resp.size(), 0);
}

}  // namespace ztnx
