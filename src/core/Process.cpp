#include "core/Process.hpp"

#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <vector>

Process::Process(int pid)
    : pid(pid), name(""), cpuUsage(0.0), memUsage(0.0), elapsedTime(0L)
{
    updateStats();
}

void Process::updateStats() {
    const std::string statPath = "/proc/" + std::to_string(pid) + "/stat";
    const std::string statmPath = "/proc/" + std::to_string(pid) + "/statm";

    std::ifstream statFile(statPath);
    if (!statFile.is_open()) {
        name = "";
        cpuUsage = memUsage = 0.0;
        elapsedTime = 0;
        return;
    }
    std::string line;
    std::getline(statFile, line);
    statFile.close();

    std::istringstream iss(line);
    std::vector<std::string> fields;
    std::string token;
    while (iss >> token) fields.push_back(token);

    long utime  = std::stol(fields[13]);
    long stime  = std::stol(fields[14]);
    long startT = std::stol(fields[21]);
    long totalJiffies = utime + stime;

    std::string comm = fields[1];
    if (comm.front()=='(' && comm.back()==')')
        comm = comm.substr(1, comm.size()-2);
    name = comm;

    std::ifstream upFile("/proc/uptime");
    double sysUptime = 0.0;
    if (upFile.is_open()) {
        upFile >> sysUptime;
        upFile.close();
    }

    long hz = sysconf(_SC_CLK_TCK);
    double seconds = sysUptime - (startT / static_cast<double>(hz));
    elapsedTime = static_cast<long>(seconds);

    double usage = 0.0;
    if (!firstUpdate && seconds > prevSeconds) {
        double deltaJ = (totalJiffies - prevJiffies) / static_cast<double>(hz);
        double deltaT = seconds - prevSeconds;
        usage = 100.0 * (deltaJ / deltaT);
    } else if (seconds > 0) {
        usage = 100.0 * ((totalJiffies / static_cast<double>(hz)) / seconds);
        firstUpdate = false;
    }
    cpuUsage = usage;
    prevJiffies = totalJiffies;
    prevSeconds = seconds;

    std::ifstream statmFile(statmPath);
    long sizePages = 0, rssPages = 0;
    if (statmFile.is_open()) {
        statmFile >> sizePages >> rssPages;
        statmFile.close();
    }
    long pageSize    = sysconf(_SC_PAGESIZE);
    long rssBytes    = rssPages * pageSize;

    std::ifstream meminfo("/proc/meminfo");
    long memTotalKb = 0;
    if (meminfo.is_open()) {
        std::string label;
        while (meminfo >> label >> memTotalKb) {
            if (label == "MemTotal:") break;
        }
        meminfo.close();
    }
    long memTotalBytes = memTotalKb * 1024;
    memUsage = (memTotalBytes > 0)
        ? 100.0 * (rssBytes / static_cast<double>(memTotalBytes))
        : 0.0;
}

std::string Process::formatForDisplay() const {
    std::ostringstream oss;
    oss << pid
        << "\t" << name
        << "\tCPU:"  << (cpuUsage  < 0.0 ? 0.0 : cpuUsage ) << "%"
        << "\tMEM:"  << (memUsage  < 0.0 ? 0.0 : memUsage ) << "%"
        << "\tTIME:" << elapsedTime << "s";
    return oss.str();
}

int Process::getPid()           const { return pid; }
const std::string& Process::getName() const { return name; }
double Process::getCpuUsage()   const { return cpuUsage; }
double Process::getMemUsage()   const { return memUsage; }
long Process::getElapsedTime()  const { return elapsedTime; }
