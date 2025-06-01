#include "core/ProcessManager.hpp"

#include <dirent.h>
#include <algorithm>
#include <cctype>

ProcessManager::ProcessManager() {
    refresh();
}

void ProcessManager::attach(IObserver* obs) {
    observers.push_back(obs);
}

void ProcessManager::refresh() {
    processes.clear();

    DIR* procDir = opendir("/proc");
    if (!procDir) return;

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        std::string name = entry->d_name;
        if (entry->d_type == DT_DIR &&
            !name.empty() &&
            std::all_of(name.begin(), name.end(),
                        [](char c){ return std::isdigit(c); })) {
            int pid = std::stoi(name);
            processes.emplace_back(pid);
        }
    }
    closedir(procDir);

    for (auto* obs : observers) {
        obs->onUpdate();
    }
}

const std::vector<Process>& ProcessManager::getProcesses() const {
    return processes;
}

void ProcessManager::sortByPid() {
    std::sort(processes.begin(), processes.end(),
              [](const Process& a, const Process& b){
                  return a.getPid() < b.getPid();
              });
}

void ProcessManager::sortByCpu() {
    std::sort(processes.begin(), processes.end(),
              [](const Process& a, const Process& b){
                  return a.getCpuUsage() > b.getCpuUsage();
              });
}

void ProcessManager::sortByMem() {
    std::sort(processes.begin(), processes.end(),
              [](const Process& a, const Process& b){
                  return a.getMemUsage() > b.getMemUsage();
              });
}
