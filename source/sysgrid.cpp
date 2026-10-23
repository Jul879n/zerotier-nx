// Partes adaptadas de sys-GRID0 (MPL-2.0), https://github.com/redluigi323/sys-GRID0
#include "sysgrid.hpp"

#include <cstdio>
#include <sys/stat.h>

#include <tesla.hpp>

namespace ztnx {

namespace {

constexpr const char *ConfigDir = "sdmc:/config/sys-zerotier/";
constexpr const char *NetworksPath = "sdmc:/config/sys-zerotier/networks.ini";
constexpr const char *StatusPath = "sdmc:/config/sys-zerotier/status.txt";
constexpr const char *PeersPath = "sdmc:/config/sys-zerotier/peers.txt";
constexpr const char *IdentityPath = "sdmc:/atmosphere/contents/4200000000005A54/zt/identity.public";
constexpr const char *BootFlag = "sdmc:/atmosphere/contents/4200000000005A54/flags/boot2.flag";
constexpr const char *DisabledBootFlag = "sdmc:/atmosphere/contents/4200000000005A54/flags/boot2.flag.disabled";

bool FileExists(const char *path)
{
    struct stat st = {};
    return ::stat(path, &st) == 0;
}

}  // namespace

std::string ReadFile(const char *path)
{
    FILE *file = std::fopen(path, "r");
    if (file == nullptr) { return {}; }
    char buf[4096];
    const size_t n = std::fread(buf, 1, sizeof(buf), file);
    std::fclose(file);
    return std::string(buf, n);
}

Snapshot ReadSnapshot()
{
    Snapshot s;
    std::string status = ReadFile(StatusPath);
    if (status.empty()) {
        // Ventana de reescritura de sys-GRID0 (delete + create + write).
        svcSleepThread(50'000'000ULL);
        status = ReadFile(StatusPath);
    }
    static Snapshot last;
    static unsigned emptyStreak = 0;
    if (KeepLastOnEmpty(emptyStreak, status.empty()) && last.backend) { return last; }
    s.backend = !status.empty();
    s.state = DisplayState(StatusValue(status, "online"), StatusValue(status, "netstatus"));
    s.ip = DisplayAddress(StatusValue(status, "address"));
    const std::string nw = StatusValue(status, "network");
    s.network = IsHexNetworkId(nw) ? ToUpperHex(nw) : "";
    s.peers = ParsePeers(ReadFile(PeersPath));
    s.nodeId = NodeIdFromIdentity(ReadFile(IdentityPath));
    if (s.backend) { last = s; }
    return s;
}

std::vector<std::pair<std::string, std::string>> SavedNetworks()
{
    std::vector<std::pair<std::string, std::string>> out;
    for (const std::string &section : ult::parseSectionsFromIni(NetworksPath)) {
        if (section == "sys-zerotier") { continue; }
        const std::string nwid = ult::parseValueFromIniSection(NetworksPath, section, "nwid");
        if (!IsHexNetworkId(nwid)) { continue; }
        out.emplace_back(section, nwid);
    }
    return out;
}

std::string SelectedNetwork()
{
    return ult::parseValueFromIniSection(NetworksPath, "sys-zerotier", "selected");
}

void SelectNetwork(const std::string &section)
{
    ult::createDirectory(ConfigDir);
    ult::setIniFileValue(NetworksPath, "sys-zerotier", "selected", section);
}

void SaveAndSelectNetwork(const std::string &nwid, const std::string &name)
{
    const std::string lower = ToLowerHex(nwid);
    const std::string section = name.empty() ? lower : name;
    ult::createDirectory(ConfigDir);
    ult::setIniFileValue(NetworksPath, section, "nwid", lower);
    SelectNetwork(section);
}

void ForgetNetwork(const std::string &section)
{
    if (SelectedNetwork() == section) {
        ult::setIniFileValue(NetworksPath, "sys-zerotier", "selected", "");
    }
    ult::removeIniSection(NetworksPath, section);
}

bool BootEnabled()
{
    return FileExists(BootFlag) || !FileExists(DisabledBootFlag);
}

void SetBootEnabled(bool enabled)
{
    if (enabled) {
        if (FileExists(DisabledBootFlag)) {
            (void)std::rename(DisabledBootFlag, BootFlag);
        } else if (!FileExists(BootFlag)) {
            FILE *file = std::fopen(BootFlag, "w");
            if (file != nullptr) { std::fclose(file); }
        }
    } else if (FileExists(BootFlag)) {
        (void)std::rename(BootFlag, DisabledBootFlag);
    }
}

void EnsureConfigDir()
{
    ult::createDirectory(ConfigDir);
}

}  // namespace ztnx
