// Estructura y editor de Network ID adaptados de sys-GRID0 (MPL-2.0),
// https://github.com/redluigi323/sys-GRID0
#define NDEBUG
#define STBTT_STATIC
#define TESLA_INIT_IMPL

#include <exception_wrap.hpp>
#include <tesla.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "core.hpp"
#include "http_server.hpp"
#include "qrcodegen.h"
#include "sysgrid.hpp"

namespace {

using namespace ztnx;

const tsl::Color kAccent(4, 7, 15, 15);   // #4f7cff
const tsl::Color kGray(8, 8, 8, 15);
const tsl::Color kWhite(15, 15, 15, 15);
const tsl::Color kBlack(0, 0, 0, 15);

constexpr u64 kSecondNs = 1000000000ULL;

bool Due(u64 &last, u64 intervalNs)
{
    const u64 now = armGetSystemTick();
    if (last != 0 && armTicksToNs(now - last) < intervalNs) { return false; }
    last = now;
    return true;
}

class StatusGui final : public tsl::Gui {
public:
    tsl::elm::Element *createUI() override
    {
        auto *frame = new tsl::elm::OverlayFrame("ZeroTier NX", "Estado");
        auto *list = new tsl::elm::List();
        const char *labels[] = {"Estado", "IP", "Node ID", "Peers", "Ping", "Red"};
        for (size_t i = 0; i < 6; ++i) {
            m_items[i] = new tsl::elm::ListItem(labels[i], "—", true);
            m_items[i]->setClickListener([](u64) { return false; });
            list->addItem(m_items[i]);
        }
        frame->setContent(list);
        Refresh();
        return frame;
    }

    void update() override
    {
        if (Due(m_last, kSecondNs)) { Refresh(); }
    }

private:
    void Refresh()
    {
        const Snapshot s = ReadSnapshot();
        if (m_items[0] == nullptr) { return; }
        m_items[0]->setValue(s.state);
        m_items[1]->setValue(s.ip);
        m_items[2]->setValue(s.nodeId.empty() ? "—" : s.nodeId);
        m_items[3]->setValue(std::to_string(s.peers.count));
        m_items[4]->setValue(s.peers.avgLatencyMs < 0 ? "—" : std::to_string(s.peers.avgLatencyMs) + " ms");
        m_items[5]->setValue(s.network.empty() ? "Ninguna" : s.network);
    }

    u64 m_last = 0;
    tsl::elm::ListItem *m_items[6] = {};
};

class QrGui final : public tsl::Gui {
public:
    QrGui()
    {
        if (!m_server.Start()) {
            m_status = m_server.Error();
            return;
        }
        m_started = qrcodegen_encodeText(m_server.Url().c_str(), m_tmp, m_qr, qrcodegen_Ecc_MEDIUM,
                                         qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                         qrcodegen_Mask_AUTO, true);
        if (!m_started) {
            m_status = "No se pudo generar el QR";
            m_server.Stop();
            return;
        }
        m_status = "Esperando…";
    }

    ~QrGui() override { m_server.Stop(); }

    tsl::elm::Element *createUI() override
    {
        auto *frame = new tsl::elm::OverlayFrame("ZeroTier NX", "Iniciar sesión con QR");
        frame->setContent(new tsl::elm::CustomDrawer(
            [this](tsl::gfx::Renderer *r, s32 x, s32 y, s32 w, s32 h) { Draw(r, x, y, w, h); }));
        return frame;
    }

    void update() override
    {
        if (!m_started) { return; }
        std::string nwid, name;
        if (m_server.TakePending(nwid, name)) {
            SaveAndSelectNetwork(nwid, name);
            m_status = "Conectando a " + ToUpperHex(nwid) + "…";
        }
    }

private:
    void Draw(tsl::gfx::Renderer *r, s32 x, s32 y, s32 w, s32 h)
    {
        (void)h;
        constexpr s32 box = 276;
        s32 textY = y + 20;
        if (m_started) {
            const s32 size = qrcodegen_getSize(m_qr);
            const s32 px = box / (size + 8);
            const s32 total = px * (size + 8);
            const s32 bx = x + (w - box) / 2;
            const s32 by = y + 10;
            r->drawRect(bx, by, box, box, kWhite);
            const s32 ox = bx + (box - total) / 2 + 4 * px;
            const s32 oy = by + (box - total) / 2 + 4 * px;
            for (s32 j = 0; j < size; ++j) {
                for (s32 i = 0; i < size; ++i) {
                    if (qrcodegen_getModule(m_qr, i, j)) { r->drawRect(ox + i * px, oy + j * px, px, px, kBlack); }
                }
            }
            textY = by + box + 34;
            r->drawString("Escanea con tu teléfono", false, x + 20, textY, 19, kWhite);
            textY += 26;
            r->drawString(m_server.Url(), true, x + 20, textY, 14, kGray);
            textY += 28;
        }
        r->drawString(m_status, false, x + 20, textY, 17, m_started ? kAccent : kWhite);
    }

    QrServer m_server;
    bool m_started = false;
    std::string m_status;
    uint8_t m_qr[qrcodegen_BUFFER_LEN_MAX] = {};
    uint8_t m_tmp[qrcodegen_BUFFER_LEN_MAX] = {};
};

class ManualGui final : public tsl::Gui {
public:
    ManualGui()
    {
        const std::string active = ReadSnapshot().network;
        m_value = active;
    }

    tsl::elm::Element *createUI() override
    {
        auto *frame = new tsl::elm::OverlayFrame("Network ID", "Escribe los 16 dígitos");
        auto *list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("El teclado escribe el ID   Ⓐ: dígito del teclado"));
        m_id = new tsl::elm::ListItem("Network ID de ZeroTier", DisplayValue());
        list->addItem(m_id);
        m_key_rows[0] = new tsl::elm::CategoryHeader(KeypadRow(0), false);
        m_key_rows[1] = new tsl::elm::CategoryHeader(KeypadRow(1), false);
        list->addItem(m_key_rows[0]);
        list->addItem(m_key_rows[1]);
        m_result = new tsl::elm::CategoryHeader(Hint(), false);
        list->addItem(m_result);
        frame->setContent(list);
        return frame;
    }

    bool handleInput(u64 keys_down, u64, const HidTouchState &,
                     HidAnalogStickState, HidAnalogStickState) override
    {
        // Enter del teclado USB se latchea en update() y se consume aquí.
        if (m_keyboard_commit) {
            m_keyboard_commit = false;
            return Commit();
        }
        if (keys_down & KEY_LEFT) {
            m_key = (m_key + 15) % 16;
        } else if (keys_down & KEY_RIGHT) {
            m_key = (m_key + 1) % 16;
        } else if (keys_down & (KEY_UP | KEY_DOWN)) {
            m_key ^= 8;
        } else if (keys_down & KEY_A) {
            AppendHex(Hex[m_key]);
        } else if (keys_down & KEY_X) {
            EraseHex();
        } else if (keys_down & KEY_Y) {
            ClearHex();
        } else if (keys_down & KEY_PLUS) {
            return Commit();
        } else {
            return false;
        }
        RefreshEditor();
        return true;
    }

    void update() override
    {
        HidKeyboardState current{};
        if (hidGetKeyboardStates(&current, 1) == 0) { return; }

        bool changed = false;
        if (!m_keyboard_ready || current.sampling_number != m_keyboard_previous.sampling_number) {
            static constexpr struct { HidKeyboardKey key; char value; } HexKeys[] = {
                {HidKeyboardKey_D0, '0'}, {HidKeyboardKey_D1, '1'},
                {HidKeyboardKey_D2, '2'}, {HidKeyboardKey_D3, '3'},
                {HidKeyboardKey_D4, '4'}, {HidKeyboardKey_D5, '5'},
                {HidKeyboardKey_D6, '6'}, {HidKeyboardKey_D7, '7'},
                {HidKeyboardKey_D8, '8'}, {HidKeyboardKey_D9, '9'},
                {HidKeyboardKey_A, 'A'}, {HidKeyboardKey_B, 'B'},
                {HidKeyboardKey_C, 'C'}, {HidKeyboardKey_D, 'D'},
                {HidKeyboardKey_E, 'E'}, {HidKeyboardKey_F, 'F'},
            };
            for (const auto &entry : HexKeys) {
                if (hidKeyboardStateGetKey(&current, entry.key) &&
                    (!m_keyboard_ready || !hidKeyboardStateGetKey(&m_keyboard_previous, entry.key))) {
                    AppendHex(entry.value);
                    changed = true;
                }
            }
            if (hidKeyboardStateGetKey(&current, HidKeyboardKey_Backspace) &&
                (!m_keyboard_ready || !hidKeyboardStateGetKey(&m_keyboard_previous, HidKeyboardKey_Backspace))) {
                EraseHex();
                changed = true;
            }
            if (hidKeyboardStateGetKey(&current, HidKeyboardKey_Return) &&
                (!m_keyboard_ready || !hidKeyboardStateGetKey(&m_keyboard_previous, HidKeyboardKey_Return))) {
                m_keyboard_commit = true;
            }
        }
        m_keyboard_previous = current;
        m_keyboard_ready = true;
        if (changed) { RefreshEditor(); }
    }

private:
    static constexpr char Hex[] = "0123456789ABCDEF";

    static const char *Hint() { return "+ Unirse   Ⓧ Borrar   Ⓨ Limpiar"; }

    std::string DisplayValue() const
    {
        std::string display = m_value;
        display.append(16 - display.size(), '_');
        for (size_t pos = 12; pos > 0; pos -= 4) { display.insert(pos, " "); }
        return display;
    }

    std::string KeypadRow(size_t row) const
    {
        std::string out;
        const size_t first = row * 8;
        for (size_t i = first; i < first + 8; ++i) {
            if (!out.empty()) { out += "  "; }
            out += i == m_key ? '[' : ' ';
            out += Hex[i];
            out += i == m_key ? ']' : ' ';
        }
        return out;
    }

    void SetMessage(const std::string &message)
    {
        if (m_result != nullptr) { m_result->setText(message); }
    }

    void AppendHex(char value)
    {
        if (m_replace_on_type) {
            m_value.clear();
            m_replace_on_type = false;
        }
        if (m_value.size() >= 16) {
            SetMessage("Máximo 16 dígitos");
            return;
        }
        m_value.push_back(value);
    }

    void EraseHex()
    {
        m_replace_on_type = false;
        if (!m_value.empty()) { m_value.pop_back(); }
    }

    void ClearHex()
    {
        m_value.clear();
        m_replace_on_type = false;
    }

    bool Commit()
    {
        if (!IsHexNetworkId(m_value)) {
            SetMessage(m_value.size() == 16 ? "El ID no puede ser todo ceros" : "Ingresa exactamente 16 dígitos");
            return true;
        }
        SaveAndSelectNetwork(m_value, m_value);
        tsl::goBack();
        return true;
    }

    void RefreshEditor()
    {
        m_id->setValue(DisplayValue());
        m_key_rows[0]->setText(KeypadRow(0));
        m_key_rows[1]->setText(KeypadRow(1));
        SetMessage(Hint());
    }

    std::string m_value;
    size_t m_key = 0;
    bool m_replace_on_type = true;
    bool m_keyboard_ready = false;
    bool m_keyboard_commit = false;
    HidKeyboardState m_keyboard_previous{};
    tsl::elm::ListItem *m_id = nullptr;
    tsl::elm::CategoryHeader *m_key_rows[2] = {};
    tsl::elm::CategoryHeader *m_result = nullptr;
};

class NetworksGui final : public tsl::Gui {
public:
    tsl::elm::Element *createUI() override
    {
        auto *frame = new tsl::elm::OverlayFrame("Redes guardadas", "Ⓐ Activar   Ⓨ Olvidar");
        auto *list = new tsl::elm::List();
        const auto networks = SavedNetworks();
        if (networks.empty()) {
            list->addItem(new tsl::elm::ListItem("Sin redes guardadas"));
        }
        const std::string selected = SelectedNetwork();
        for (const auto &[section, nwid] : networks) {
            auto *item = new tsl::elm::ListItem(section, section == selected ? "Activa" : ToUpperHex(nwid), true);
            item->setClickListener([item, section](u64 keys) {
                if (keys & KEY_A) {
                    SelectNetwork(section);
                    item->setValue("Cambiando…");
                    return true;
                }
                if (keys & KEY_Y) {
                    ForgetNetwork(section);
                    tsl::goBack();
                    tsl::changeTo<NetworksGui>();
                    return true;
                }
                return false;
            });
            list->addItem(item);
        }
        frame->setContent(list);
        return frame;
    }
};

class AboutGui final : public tsl::Gui {
public:
    tsl::elm::Element *createUI() override
    {
        auto *frame = new tsl::elm::OverlayFrame("ZeroTier NX", "Acerca de");
        auto *list = new tsl::elm::List();
        auto add = [&](const char *k, const std::string &v) {
            auto *item = new tsl::elm::ListItem(k, v, true);
            item->setClickListener([](u64) { return false; });
            list->addItem(item);
        };
        add("Versión", APP_VERSION);
        add("Backend", ReadSnapshot().backend ? "sys-GRID0" : "sys-GRID0 (no detectado)");
        add("Licencias", "GPL-2.0 / MPL-2.0 / MIT");
        frame->setContent(list);
        return frame;
    }
};

class MainGui final : public tsl::Gui {
public:
    tsl::elm::Element *createUI() override
    {
        auto *frame = new tsl::elm::OverlayFrame("ZeroTier NX", APP_VERSION);
        auto *list = new tsl::elm::List();

        list->addItem(new tsl::elm::CustomDrawer(
            [this](tsl::gfx::Renderer *r, s32 x, s32 y, s32 w, s32 h) { DrawHeader(r, x, y, w, h); }), 70);

        list->addItem(new tsl::elm::CategoryHeader("Conexión (aplica al reiniciar)"));
        auto *boot = new tsl::elm::ToggleListItem("Conexión", BootEnabled());
        boot->setStateChangedListener([](bool state) { SetBootEnabled(state); });
        list->addItem(boot);

        auto add = [&](const char *label, auto go) {
            auto *item = new tsl::elm::ListItem(label);
            item->setClickListener([go](u64 keys) {
                if (!(keys & KEY_A)) { return false; }
                go();
                return true;
            });
            list->addItem(item);
        };
        add("Estado", [] { tsl::changeTo<StatusGui>(); });
        add("Iniciar sesión con QR", [] { tsl::changeTo<QrGui>(); });
        add("Iniciar sesión manual", [] { tsl::changeTo<ManualGui>(); });
        add("Redes guardadas", [] { tsl::changeTo<NetworksGui>(); });
        add("Acerca de", [] { tsl::changeTo<AboutGui>(); });

        frame->setContent(list);
        m_snap = ReadSnapshot();
        return frame;
    }

    void update() override
    {
        if (Due(m_last_refresh, kSecondNs)) { m_snap = ReadSnapshot(); }
        if (Due(m_last_sample, 5 * kSecondNs) && m_snap.peers.avgLatencyMs >= 0) {
            m_history.push_back(m_snap.peers.avgLatencyMs);
            if (m_history.size() > 12) { m_history.erase(m_history.begin()); }
        }
    }

    bool handleInput(u64 keys_down, u64, const HidTouchState &,
                     HidAnalogStickState, HidAnalogStickState) override
    {
        if (!(keys_down & KEY_B)) { return false; }
        // Raíz: devolver el control a Ultrahand en vez de ocultar el overlay.
        tsl::setNextOverlay(ult::OVERLAY_PATH + "ovlmenu.ovl");
        tsl::Overlay::get()->close();
        return true;
    }

private:
    void DrawHeader(tsl::gfx::Renderer *r, s32 x, s32 y, s32 w, s32 h)
    {
        const bool connected = m_snap.state == "Conectado";
        std::string line = "● " + m_snap.state + " · " + m_snap.ip;
        if (m_snap.peers.avgLatencyMs >= 0) { line += " · " + std::to_string(m_snap.peers.avgLatencyMs) + " ms"; }
        r->drawString(line, false, x + 16, y + h / 2 + 6, 17, connected ? kAccent : kGray, w - 180);

        if (m_history.size() < 2) { return; }
        constexpr s32 sw = 140, sh = 34;
        const s32 sx = x + w - sw - 16, sy = y + (h - sh) / 2;
        const int lo = *std::min_element(m_history.begin(), m_history.end());
        const int hi = *std::max_element(m_history.begin(), m_history.end());
        const int span = std::max(hi - lo, 1);
        auto py = [&](int v) { return sy + sh - 2 - (v - lo) * (sh - 2) / span; };
        const s32 segs = static_cast<s32>(m_history.size()) - 1;
        for (s32 i = 0; i < segs; ++i) {
            const s32 x0 = sx + i * sw / segs, x1 = sx + (i + 1) * sw / segs;
            const s32 y0 = py(m_history[i]), y1 = py(m_history[i + 1]);
            for (s32 px = x0; px < x1; ++px) {
                r->drawRect(px, y0 + (y1 - y0) * (px - x0) / std::max(x1 - x0, 1), 2, 2, kAccent);
            }
        }
    }

    u64 m_last_refresh = 0, m_last_sample = 0;
    Snapshot m_snap;
    std::vector<int> m_history;
};

class Overlay final : public tsl::Overlay {
public:
    void initServices() override { EnsureConfigDir(); }
    void exitServices() override {}
    std::unique_ptr<tsl::Gui> loadInitialGui() override { return initially<MainGui>(); }
};

}  // namespace

int main(int argc, char **argv)
{
    return tsl::loop<Overlay, tsl::impl::LaunchFlags::CloseOnExit>(argc, argv);
}
