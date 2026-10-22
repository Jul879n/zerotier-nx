// Acceso a los archivos de sys-GRID0 en la SD.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "core.hpp"

namespace ztnx {

struct Snapshot {
    std::string state, ip, nodeId, network;
    PeerSummary peers;
    bool backend = false;  // status.txt presente
};

std::string ReadFile(const char *path);
Snapshot ReadSnapshot();

std::vector<std::pair<std::string, std::string>> SavedNetworks();  // sección -> nwid
std::string SelectedNetwork();
void SelectNetwork(const std::string &section);
void SaveAndSelectNetwork(const std::string &nwid, const std::string &name);
void ForgetNetwork(const std::string &section);

bool BootEnabled();
void SetBootEnabled(bool enabled);
void EnsureConfigDir();

}  // namespace ztnx
