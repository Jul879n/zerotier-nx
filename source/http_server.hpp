// Servidor HTTP mínimo para el login QR: corre solo mientras la pantalla QR está abierta.
#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include <switch.h>

namespace ztnx {

class QrServer {
public:
    ~QrServer() { Stop(); }

    bool Start();
    void Stop();
    bool TakePending(std::string &nwid, std::string &name);

    const std::string &Url() const { return m_url; }
    const std::string &Error() const { return m_error; }

private:
    static void Loop(void *arg);
    void Serve(int client);

    Thread m_thread{};
    bool m_threadStarted = false;
    bool m_nifm = false;
    bool m_ownSocket = false;
    int m_fd = -1;
    std::atomic<bool> m_stop{false};
    std::string m_url, m_error, m_token;

    std::mutex m_mutex;
    bool m_hasPending = false;
    std::string m_pendingNwid, m_pendingName;
};

}  // namespace ztnx
