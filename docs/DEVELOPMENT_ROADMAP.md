# Development Roadmap: C++20 / Qt 6 Accounting Application

## Overview of Development Phases

| Phase | Milestone | Deliverables | Status |
| :---: | :--- | :--- | :---: |
| **0** | **Existing Project Analysis** | `docs/EXISTING_PROJECT_ANALYSIS.md`, inventory of features, invariants, and tests. | **Completed** |
| **1** | **C++ Foundation & Core Engine** | CMake project, C++20 build, `ExactDecimal`, Domain models, SQLite database manager, migrations, logging. | **In Progress** |
| **2** | **Native UI Foundation** | Main window, Gateway of Tally, NavigationManager, ShortcutManager, authentic Tally styling. | **Pending** |
| **3** | **Company & Masters Management** | Company creation, group hierarchy, ledger workflows, search & selection dialogs. | **Pending** |
| **4** | **Accounting Domain Engine** | Double-entry invariants, transactional posting, running balances, reversal, cancellation, audit events. | **Pending** |
| **5** | **Voucher Management** | Voucher entry screens (Journal, Payment, Receipt, Contra, Sales, Purchase) with keyboard focus flows. | **Pending** |
| **6** | **Financial Reports** | Day Book, Ledger Statement, Trial Balance, Profit & Loss, Balance Sheet with drill-down navigation. | **Pending** |
| **7** | **Tally Compatibility** | QXmlStreamReader/Writer, JSON exchange, Tally Port 9000 HTTP client. | **Pending** |
| **8** | **Production Readiness** | Backup/Restore, unit & integration test suites, packaging with CPack. | **Pending** |
