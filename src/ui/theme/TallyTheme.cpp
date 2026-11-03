#include "TallyTheme.hpp"

namespace tally::ui {

QString TallyTheme::getApplicationStyleSheet() {
    return R"(
        QMainWindow {
            background-color: #EDF4FA;
        }

        QWidget {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            font-size: 10pt;
            color: #112233;
        }

        /* Top Header Bar */
        #topBar {
            background-color: #1B4977;
            color: #FFFFFF;
            border-bottom: 2px solid #123252;
        }

        #topBar QLabel {
            color: #FFFFFF;
            font-weight: bold;
        }

        #topBar QPushButton {
            background-color: transparent;
            color: #FFFFFF;
            border: none;
            padding: 4px 8px;
            font-size: 9pt;
        }

        #topBar QPushButton:hover {
            background-color: #2A6592;
            border-radius: 2px;
        }

        /* Right Function-Key Sidebar */
        #rightSidebar {
            background-color: #E2ECF5;
            border-left: 1px solid #B8D0E5;
        }

        #rightSidebar QPushButton {
            background-color: #FFFFFF;
            color: #112233;
            border: 1px solid #B8D0E5;
            border-radius: 3px;
            padding: 5px;
            font-size: 9pt;
            text-align: left;
            margin-bottom: 2px;
        }

        #rightSidebar QPushButton:hover {
            background-color: #FFE082;
            border-color: #F6C343;
            font-weight: bold;
        }

        #rightSidebar QPushButton:pressed {
            background-color: #F6C343;
        }

        /* Gateway of Tally */
        #gatewayBox {
            background-color: #FFFFFF;
            border: 2px solid #1B4977;
        }

        #gatewayHeader {
            background-color: #235B8C;
            color: #FFFFFF;
            font-weight: bold;
            padding: 6px;
            font-size: 11pt;
            text-align: center;
        }

        #gatewaySectionTitle {
            color: #1B4977;
            font-weight: bold;
            font-size: 9pt;
            padding-left: 8px;
            padding-top: 6px;
        }

        #gatewayList {
            background-color: #FFFFFF;
            border: none;
            outline: none;
        }

        #gatewayList::item {
            padding: 4px 12px;
            height: 24px;
        }

        #gatewayList::item:selected {
            background-color: #F6C343;
            color: #000000;
            font-weight: bold;
        }

        #gatewayList::item:hover {
            background-color: #FFE082;
            color: #000000;
        }

        /* Tables & Financial Views */
        QTableView {
            background-color: #FFFFFF;
            border: 1px solid #B8D0E5;
            gridline-color: #E2ECF5;
            selection-background-color: #F6C343;
            selection-color: #000000;
            font-size: 9.5pt;
        }

        QHeaderView::section {
            background-color: #1B4977;
            color: #FFFFFF;
            font-weight: bold;
            padding: 4px;
            border: 1px solid #123252;
            font-size: 9pt;
        }

        QTableView::item:selected {
            background-color: #F6C343;
            color: #000000;
            font-weight: bold;
        }

        /* Input Controls */
        QLineEdit {
            background-color: #FFFFFF;
            border: 1px solid #1B4977;
            padding: 3px 6px;
            selection-background-color: #F6C343;
            selection-color: #000000;
        }

        QLineEdit:focus {
            background-color: #FFFFE0;
            border: 2px solid #1B4977;
        }

        /* Bottom Status Bar */
        #bottomBar {
            background-color: #1B4977;
            color: #FFFFFF;
            border-top: 1px solid #123252;
            font-size: 8.5pt;
        }

        #bottomBar QLabel {
            color: #FFFFFF;
        }
    )";
}

} // namespace tally::ui
