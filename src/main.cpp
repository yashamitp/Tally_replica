#include <QApplication>
#include <QDir>
#include <iostream>
#include "ui/shell/MainWindow.hpp"
#include "utilities/Logger.hpp"

int main(int argc, char* argv[]) {
    if (qEnvironmentVariableIsEmpty("DISPLAY") && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY") && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);
    app.setApplicationName("TallyReplica");
    app.setApplicationDisplayName("Antigravity Accounting - TallyPrime Compatible System");
    app.setOrganizationName("Antigravity");

    tally::utilities::Logger::info("Starting Antigravity Accounting C++20 / Qt 6 Application...");

    std::string dbPath = "tally_replica.sqlite3";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--memory" || arg == "-m") {
            dbPath = ":memory:";
        } else if (arg.rfind("--db=", 0) == 0) {
            dbPath = arg.substr(5);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: TallyReplica [options]\n"
                      << "Options:\n"
                      << "  --db=<path>   Specify SQLite database file path (default: tally_replica.sqlite3)\n"
                      << "  --memory, -m  Use in-memory SQLite database\n"
                      << "  --help, -h    Display this help message\n";
            return 0;
        }
    }

    tally::ui::MainWindow mainWindow;
    if (!mainWindow.initialize(dbPath)) {
        tally::utilities::Logger::error("Failed to initialize MainWindow. Exiting.");
        return 1;
    }

    mainWindow.show();
    tally::utilities::Logger::info("Application main window shown successfully. Entering event loop.");

    return app.exec();
}
