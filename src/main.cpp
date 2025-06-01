#include "core/ProcessManager.hpp"
#include "ui/UI.hpp"

int main() {
    ProcessManager pm;
    UI ui(pm);
    ui.run();
    return 0;
}
