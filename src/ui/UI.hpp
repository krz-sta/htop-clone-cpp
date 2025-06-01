#ifndef HTOP_CLONE_UI_HPP
#define HTOP_CLONE_UI_HPP

#include "patterns/Observer.hpp"
#include "core/ProcessManager.hpp"

#include <string>
#include <vector>
#include <ncurses.h>

class UI : public IObserver {
public:
    explicit UI(ProcessManager& pm);
    ~UI();

    void onUpdate() override;

    void run();

private:
    ProcessManager& pm;

    enum class SortMode { PID, CPU, MEM };
    SortMode sortMode{SortMode::PID};

    int offset{0};
    int selectedIndex{0};

    bool filtering{false};
    std::string filterStr;

    long prevTotalCpuJiffies{0};
    long prevIdleCpuJiffies{0};

    WINDOW* winStats{nullptr};
    WINDOW* winProcs{nullptr};
    WINDOW* winHelp{nullptr};
    WINDOW* winFilter{nullptr};

    static constexpr char spinnerChars[4] = {'|','/','-','\\'};
    int spinnerIdx{0};

    void initializeWindows();
    void destroyWindows();
    void resizeWindowsIfNeeded();

    void draw();
    void drawStats();
    void drawProcHeader();
    void drawProcessList();
    void drawHelp();
    void drawFilterPrompt();
    void drawSpinner(int row, int col);

    double getTotalCpuUsage();
    double getTotalMemUsage();

    bool nameMatchesFilter(const std::string& name) const;
    std::vector<Process> filteredProcesses() const;
};

#endif
