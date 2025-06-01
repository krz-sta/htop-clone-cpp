#include "ui/UI.hpp"

#include <ncurses.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>   
#include <csignal>  
#include <cerrno>   
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <iomanip>  
#include <cstring>  

static constexpr double CPU_GREEN_THRESHOLD  = 30.0;
static constexpr double CPU_YELLOW_THRESHOLD = 70.0;

static constexpr short CP_COLOR_DEFAULT     = 1;
static constexpr short CP_COLOR_RED         = 2;
static constexpr short CP_COLOR_YELLOW      = 3;
static constexpr short CP_COLOR_GREEN       = 4;
static constexpr short CP_COLOR_HEADER_BG   = 5;
static constexpr short CP_COLOR_ROW_ALT_BG  = 6;

UI::UI(ProcessManager& pm)
    : pm(pm)
{
    pm.attach(this);

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE); 

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(CP_COLOR_DEFAULT,   COLOR_WHITE,  -1);
        init_pair(CP_COLOR_RED,       COLOR_RED,    -1);
        init_pair(CP_COLOR_YELLOW,    COLOR_YELLOW, -1);
        init_pair(CP_COLOR_GREEN,     COLOR_GREEN,  -1);
        init_pair(CP_COLOR_HEADER_BG, COLOR_WHITE,  COLOR_BLUE);
        init_pair(CP_COLOR_ROW_ALT_BG, COLOR_BLACK, COLOR_WHITE);
    }

    initializeWindows();

    prevTotalCpuJiffies = prevIdleCpuJiffies = 0;
    getTotalCpuUsage();
}

UI::~UI() {
    destroyWindows();
    endwin();
}

void UI::onUpdate() {}

bool UI::nameMatchesFilter(const std::string& name) const {
    if (filterStr.empty()) return true;
    auto it = std::search(
        name.begin(), name.end(),
        filterStr.begin(), filterStr.end(),
        [](char c1, char c2){
            return std::tolower(c1) == std::tolower(c2);
        }
    );
    return it != name.end();
}

std::vector<Process> UI::filteredProcesses() const {
    std::vector<Process> vec;
    const auto& all = pm.getProcesses();
    for (const auto& p : all) {
        if (nameMatchesFilter(p.getName())) {
            vec.push_back(p);
        }
    }
    return vec;
}

void UI::initializeWindows() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    winStats = newwin(4, cols, 0, 0);

    winProcs = newwin(rows - 6, cols, 4, 0);

    winHelp = newwin(1, cols, rows - 2, 0);

    winFilter = newwin(1, cols, rows - 1, 0);

    box(winStats, 0, 0);
    box(winProcs, 0, 0);
    wrefresh(winStats);
    wrefresh(winProcs);
}

void UI::destroyWindows() {
    if (winStats)  { delwin(winStats);  winStats  = nullptr; }
    if (winProcs)  { delwin(winProcs);  winProcs  = nullptr; }
    if (winHelp)   { delwin(winHelp);   winHelp   = nullptr; }
    if (winFilter) { delwin(winFilter); winFilter = nullptr; }
}

void UI::resizeWindowsIfNeeded() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    wresize(winStats, 4, cols);
    mvwin(winStats, 0, 0);

    wresize(winProcs, rows - 6, cols);
    mvwin(winProcs, 4, 0);

    wresize(winHelp, 1, cols);
    mvwin(winHelp, rows - 2, 0);

    wresize(winFilter, 1, cols);
    mvwin(winFilter, rows - 1, 0);

    werase(winStats);
    box(winStats, 0, 0);
    werase(winProcs);
    box(winProcs, 0, 0);
    werase(winHelp);
    werase(winFilter);
}

double UI::getTotalCpuUsage() {
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) return 0.0;

    std::string line;
    std::getline(statFile, line);
    statFile.close();

    std::istringstream iss(line);
    std::string cpuLabel;
    long user=0, nice=0, system=0, idle=0, iowait=0, irq=0, softirq=0, steal=0;
    iss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long idleAll = idle + iowait;
    long nonIdle = user + nice + system + irq + softirq + steal;
    long total   = idleAll + nonIdle;

    long totalDiff = total - prevTotalCpuJiffies;
    long idleDiff  = idleAll - prevIdleCpuJiffies;

    double cpuPercent = 0.0;
    if (totalDiff > 0) {
        cpuPercent = 100.0 * (static_cast<double>(totalDiff - idleDiff) / totalDiff);
    }

    prevTotalCpuJiffies = total;
    prevIdleCpuJiffies  = idleAll;

    return cpuPercent;
}

double UI::getTotalMemUsage() {
    std::ifstream memFile("/proc/meminfo");
    if (!memFile.is_open()) return 0.0;

    long memTotalKb = 0, memAvailableKb = 0;
    std::string label;
    while (memFile >> label) {
        if (label == "MemTotal:") {
            memFile >> memTotalKb;
        } else if (label == "MemAvailable:") {
            memFile >> memAvailableKb;
            break;
        } else {
            std::string tmp;
            std::getline(memFile, tmp);
        }
    }
    memFile.close();

    if (memTotalKb <= 0) return 0.0;
    long usedKb = memTotalKb - memAvailableKb;
    return 100.0 * (static_cast<double>(usedKb) / memTotalKb);
}

void UI::drawFilterPrompt() {
    werase(winFilter);
    mvwprintw(winFilter, 0, 0, "/%s", filterStr.c_str());
    wclrtoeol(winFilter);
    wrefresh(winFilter);
}

void UI::drawSpinner(int row, int col) {
    mvwaddch(winProcs, row, col, spinnerChars[spinnerIdx]);
    spinnerIdx = (spinnerIdx + 1) % 4;
}

void UI::drawStats() {
    werase(winStats);
    box(winStats, 0, 0);
    mvwprintw(winStats, 0, 2, " SYSTEM USAGE ");

    int wRows, wCols;
    getmaxyx(winStats, wRows, wCols);

    double totalCpu = getTotalCpuUsage();
    double totalMem = getTotalMemUsage();

    std::ostringstream tmpCpu;
    tmpCpu << "CPU Total: " << std::fixed << std::setprecision(2) << totalCpu << "% ";
    int cpuLabelWidth = tmpCpu.str().length();
    int cpuBarWidth   = std::max(5, wCols - cpuLabelWidth - 3);

    std::ostringstream tmpMem;
    tmpMem << "Mem Total: " << std::fixed << std::setprecision(2) << totalMem << "% ";
    int memLabelWidth = tmpMem.str().length();
    int memBarWidth   = std::max(5, wCols - memLabelWidth - 3);

    mvwprintw(winStats, 1, 1, "%s%6.2f%% ", "CPU Total:", totalCpu);
    int cpuBarStart = cpuLabelWidth + 1;
    int cpuFilled = static_cast<int>(std::ceil((totalCpu / 100.0) * cpuBarWidth));
    cpuFilled = std::clamp(cpuFilled, 0, cpuBarWidth);

    if (has_colors()) {
        if (totalCpu <= CPU_GREEN_THRESHOLD) {
            wattron(winStats, COLOR_PAIR(CP_COLOR_GREEN));
        } else if (totalCpu <= CPU_YELLOW_THRESHOLD) {
            wattron(winStats, COLOR_PAIR(CP_COLOR_YELLOW));
        } else {
            wattron(winStats, COLOR_PAIR(CP_COLOR_RED));
        }
    }
    mvwprintw(winStats, 1, cpuBarStart, "[");
    for (int i = 0; i < cpuBarWidth; ++i) {
        mvwaddch(winStats, 1, cpuBarStart + 1 + i,
                (i < cpuFilled ? ACS_CKBOARD : ' '));
    }
    mvwaddch(winStats, 1, cpuBarStart + 1 + cpuBarWidth, ']');
    if (has_colors()) {
        wattroff(winStats, COLOR_PAIR(CP_COLOR_GREEN));
        wattroff(winStats, COLOR_PAIR(CP_COLOR_YELLOW));
        wattroff(winStats, COLOR_PAIR(CP_COLOR_RED));
    }

    mvwprintw(winStats, 2, 1, "%s%6.2f%% ", "Mem Total:", totalMem);
    int memBarStart = memLabelWidth + 1;
    int memFilled = static_cast<int>(std::ceil((totalMem / 100.0) * memBarWidth));
    memFilled = std::clamp(memFilled, 0, memBarWidth);

    if (has_colors()) {
        if (totalMem <= CPU_GREEN_THRESHOLD) {
            wattron(winStats, COLOR_PAIR(CP_COLOR_GREEN));
        } else if (totalMem <= CPU_YELLOW_THRESHOLD) {
            wattron(winStats, COLOR_PAIR(CP_COLOR_YELLOW));
        } else {
            wattron(winStats, COLOR_PAIR(CP_COLOR_RED));
        }
    }
    mvwprintw(winStats, 2, memBarStart, "[");
    for (int i = 0; i < memBarWidth; ++i) {
        mvwaddch(winStats, 2, memBarStart + 1 + i,
                (i < memFilled ? ACS_CKBOARD : ' '));
    }
    mvwaddch(winStats, 2, memBarStart + 1 + memBarWidth, ']');
    if (has_colors()) {
        wattroff(winStats, COLOR_PAIR(CP_COLOR_GREEN));
        wattroff(winStats, COLOR_PAIR(CP_COLOR_YELLOW));
        wattroff(winStats, COLOR_PAIR(CP_COLOR_RED));
    }

    wrefresh(winStats);
}

void UI::drawProcHeader() {
    mvwprintw(winProcs, 0, 2, " PROCESS LIST ");
    if (has_colors()) {
        wattron(winProcs, COLOR_PAIR(CP_COLOR_HEADER_BG));
        mvwprintw(winProcs, 1, 1, "%-6s %-20s %6s %6s %8s",
                  "PID", "NAME", "CPU%", "MEM%", "TIME");
        wattroff(winProcs, COLOR_PAIR(CP_COLOR_HEADER_BG));
    } else {
        mvwprintw(winProcs, 1, 1, "%-6s %-20s %6s %6s %8s",
                  "PID", "NAME", "CPU%", "MEM%", "TIME");
    }
}

void UI::drawProcessList() {
    int wRows, wCols;
    getmaxyx(winProcs, wRows, wCols);

    auto procs = filteredProcesses();
    int totalMatches = static_cast<int>(procs.size());
    int maxRows = wRows - 3;

    if (selectedIndex < 0) selectedIndex = 0;
    if (selectedIndex >= totalMatches)
        selectedIndex = std::max(0, totalMatches - 1);

    if (selectedIndex < offset) {
        offset = selectedIndex;
    } else if (selectedIndex >= offset + maxRows) {
        offset = selectedIndex - maxRows + 1;
    }
    if (offset > totalMatches - maxRows) {
        offset = std::max(0, totalMatches - maxRows);
    }

    for (int i = 0; i < maxRows && (offset + i) < totalMatches; ++i) {
        const auto& p = procs[offset + i];
        int screenRow = i + 2;
        bool isSelected = (offset + i) == selectedIndex;
        bool isAltRow   = ((offset + i) % 2) != 0;

        if (has_colors() && !isSelected && isAltRow) {
            wattron(winProcs, COLOR_PAIR(CP_COLOR_ROW_ALT_BG));
        }
        if (isSelected) {
            wattron(winProcs, A_REVERSE);
        }
        if (has_colors() && !isSelected) {
            if (p.getCpuUsage() > CPU_YELLOW_THRESHOLD) {
                wattron(winProcs, COLOR_PAIR(CP_COLOR_RED));
            } else if (p.getCpuUsage() > CPU_GREEN_THRESHOLD) {
                wattron(winProcs, COLOR_PAIR(CP_COLOR_YELLOW));
            } else {
                wattron(winProcs, COLOR_PAIR(CP_COLOR_DEFAULT));
            }
        }

        mvwprintw(winProcs, screenRow, 1,
                  "%-6d %-20.20s %6.2f %6.2f %8ld",
                  p.getPid(),
                  p.getName().c_str(),
                  p.getCpuUsage(),
                  p.getMemUsage(),
                  p.getElapsedTime());

        if (has_colors() && !isSelected) {
            wattroff(winProcs, COLOR_PAIR(CP_COLOR_RED));
            wattroff(winProcs, COLOR_PAIR(CP_COLOR_YELLOW));
            wattroff(winProcs, COLOR_PAIR(CP_COLOR_DEFAULT));
            if (isAltRow) {
                wattroff(winProcs, COLOR_PAIR(CP_COLOR_ROW_ALT_BG));
            }
        }
        if (isSelected) {
            wattroff(winProcs, A_REVERSE);
        }
    }

    drawSpinner(0, 14 + 2);
    wrefresh(winProcs);
}

void UI::drawHelp() {
    werase(winHelp);
    mvwprintw(winHelp, 0, 0,
              "q:quit  p:PID  c:CPU  m:MEM  ↑/↓:navigate  PgUp/PgDn:scroll  k:TERM  K:KILL  /:filter");
    wrefresh(winHelp);
}

void UI::draw() {
    resizeWindowsIfNeeded();

    drawStats();

    werase(winProcs);
    box(winProcs, 0, 0);
    drawProcHeader();
    drawProcessList();

    drawHelp();

    if (filtering) {
        drawFilterPrompt();
    }
}

void UI::run() {
    while (true) {
        int ch = getch();
        if (!filtering) {
            switch (ch) {
                case 'q': case 'Q':
                    return;
                case 'p': case 'P':
                    sortMode = SortMode::PID;
                    offset = selectedIndex = 0;
                    break;
                case 'c': case 'C':
                    sortMode = SortMode::CPU;
                    offset = selectedIndex = 0;
                    break;
                case 'm': case 'M':
                    sortMode = SortMode::MEM;
                    offset = selectedIndex = 0;
                    break;
                case KEY_UP:
                    selectedIndex = std::max(0, selectedIndex - 1);
                    break;
                case KEY_DOWN: {
                    auto procs = filteredProcesses();
                    selectedIndex = std::min(static_cast<int>(procs.size()) - 1,
                                             selectedIndex + 1);
                    break;
                }
                case KEY_NPAGE: {
                    auto procs = filteredProcesses();
                    int totalMatches = static_cast<int>(procs.size());
                    selectedIndex = std::min(totalMatches - 1,
                                             selectedIndex + (LINES - 6));
                    break;
                }
                case KEY_PPAGE:
                    selectedIndex = std::max(0, selectedIndex - (LINES - 6));
                    break;
                case 'k': {
                    auto procs = filteredProcesses();
                    if (selectedIndex < static_cast<int>(procs.size())) {
                        kill(procs[selectedIndex].getPid(), SIGTERM);
                    }
                    break;
                }
                case 'K': {
                    auto procs = filteredProcesses();
                    if (selectedIndex < static_cast<int>(procs.size())) {
                        kill(procs[selectedIndex].getPid(), SIGKILL);
                    }
                    break;
                }
                case '/':
                    filtering = true;
                    filterStr.clear();
                    werase(winFilter);
                    wrefresh(winFilter);
                    continue;
                default:
                    break;
            }
        } else {
            if (ch == '\n' || ch == KEY_ENTER) {
                filtering = false;
                offset = selectedIndex = 0;
            } else if (ch == 27) {
                filtering = false;
                filterStr.clear();
                offset = selectedIndex = 0;
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (!filterStr.empty()) {
                    filterStr.pop_back();
                }
            } else if (ch >= 32 && ch <= 126) {
                filterStr.push_back(static_cast<char>(ch));
            }
            draw();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        pm.refresh();
        switch (sortMode) {
            case SortMode::PID: pm.sortByPid(); break;
            case SortMode::CPU: pm.sortByCpu(); break;
            case SortMode::MEM: pm.sortByMem(); break;
        }
        draw();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
