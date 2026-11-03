cmake_minimum_required(VERSION 3.25)
project(TallyReplica VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Xml Network)

# Core domain, services, database, utilities, integrations library
add_library(TallyCore
    src/domain/decimal/ExactDecimal.cpp
    src/domain/accounting/AccountingEngine.cpp
    src/database/DatabaseManager.cpp
    src/database/migrations/SchemaMigrations.cpp
    src/services/CompanyService.cpp
    src/services/LedgerService.cpp
    src/services/VoucherService.cpp
    src/services/DayBookService.cpp
    src/services/ReportService.cpp
    src/utilities/Logger.cpp
    src/utilities/DateUtils.cpp
    src/application/NavigationManager.cpp
    src/integrations/xml/TallyXmlReader.cpp
    src/integrations/xml/TallyXmlWriter.cpp
    src/integrations/xml/TallyXmlImporter.cpp
    src/integrations/xml/TallyXmlExporter.cpp
    src/integrations/json/JsonExchange.cpp
    src/integrations/tally/TallyHttpClient.cpp
)

target_include_directories(TallyCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(TallyCore PUBLIC
    Qt6::Core
    Qt6::Sql
    Qt6::Xml
    Qt6::Network
)

# UI Library
add_library(TallyUI
    src/ui/theme/TallyTheme.cpp
    src/ui/dialogs/GoToDialog.cpp
    src/ui/gateway/GatewayView.cpp
    src/ui/vouchers/VoucherEntryView.cpp
    src/ui/reports/DayBookView.cpp
    src/ui/reports/TrialBalanceView.cpp
    src/ui/reports/ProfitLossView.cpp
    src/ui/reports/BalanceSheetView.cpp
    src/ui/reports/LedgerStatementView.cpp
    src/ui/shell/MainWindow.cpp
)

target_include_directories(TallyUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(TallyUI PUBLIC
    TallyCore
    Qt6::Widgets
    Qt6::Gui
)

# Main Executable
add_executable(TallyReplica
    src/main.cpp
)

target_link_libraries(TallyReplica PRIVATE
    TallyUI
    TallyCore
    Qt6::Widgets
)

# Test Suite
enable_testing()

add_executable(test_decimal tests/unit/test_decimal.cpp)
target_link_libraries(test_decimal PRIVATE TallyCore)
add_test(NAME test_decimal COMMAND test_decimal)

add_executable(test_validation tests/unit/test_validation.cpp)
target_link_libraries(test_validation PRIVATE TallyCore)
add_test(NAME test_validation COMMAND test_validation)

add_executable(test_accounting_engine tests/accounting/test_accounting_engine.cpp)
target_link_libraries(test_accounting_engine PRIVATE TallyCore)
add_test(NAME test_accounting_engine COMMAND test_accounting_engine)

add_executable(test_reconciliation tests/accounting/test_reconciliation.cpp)
target_link_libraries(test_reconciliation PRIVATE TallyCore)
add_test(NAME test_reconciliation COMMAND test_reconciliation)

add_executable(test_tally_xml tests/integration/test_tally_xml.cpp)
target_link_libraries(test_tally_xml PRIVATE TallyCore)
add_test(NAME test_tally_xml COMMAND test_tally_xml)

add_executable(test_json_exchange tests/integration/test_json_exchange.cpp)
target_link_libraries(test_json_exchange PRIVATE TallyCore)
add_test(NAME test_json_exchange COMMAND test_json_exchange)
