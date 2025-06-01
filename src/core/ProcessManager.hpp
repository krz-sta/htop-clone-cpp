#ifndef HTOP_CLONE_PROCESS_MANAGER_HPP
#define HTOP_CLONE_PROCESS_MANAGER_HPP

#include <vector>
#include "core/Process.hpp"
#include "patterns/Observer.hpp"

class ProcessManager {
public:
    ProcessManager();

    void refresh();

    const std::vector<Process>& getProcesses() const;

    void sortByPid();
    void sortByCpu();
    void sortByMem();

    void attach(IObserver* obs);

private:
    std::vector<Process> processes;
    std::vector<IObserver*> observers;
};

#endif
