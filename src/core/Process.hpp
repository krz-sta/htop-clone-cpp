#ifndef HTOP_CLONE_PROCESS_HPP
#define HTOP_CLONE_PROCESS_HPP

#include <string>

class Process {
public:
    explicit Process(int pid);

    void updateStats();
    std::string formatForDisplay() const;

    int getPid() const;
    const std::string& getName() const;
    double getCpuUsage() const;
    double getMemUsage() const;
    long getElapsedTime() const;

private:
    int pid;
    std::string name;
    double cpuUsage;
    double memUsage;
    long elapsedTime;

    long prevJiffies{0};
    double prevSeconds{0.0};
    bool firstUpdate{true};
};

#endif
