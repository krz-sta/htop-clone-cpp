# htop-clone

A lightweight clone of the popular **htop**, written in C++17 with ncurses for Linux.

---

## Overview

This program replicates the core functionality of **htop**:
- Real-time CPU and memory usage bars
- Interactive, colored process list with sorting (PID/CPU/MEM)
- Process selection (arrows and PageUp/PageDown)
- Send SIGTERM (`k`) or SIGKILL (`K`) to the selected process
- Case-insensitive filtering by process name (`/` → type substring → Enter to apply, Esc to cancel)
- On-screen help bar and bottom-line filter prompt

---

## Requirements

- **Linux** (relies on the `/proc` filesystem)
- **C++17** compiler (e.g. GCC ≥ 7.0 or Clang ≥ 5.0)
- **ncurses** development library

---

## Build Instructions

1. Clone or download the repository:
   ```bash
   git clone https://github.com/krz-sta/htop-clone-cpp.git
   cd htop-clone

2. Create a build directory and compile:
   mkdir build
   cd build
   cmake ..
   make

3. Run the executable:
   ./htop_clone
