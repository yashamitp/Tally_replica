# System Architecture: Native C++20 / Qt 6 Accounting Application

## 1. Architectural Philosophy

The application is structured into strictly isolated architectural layers:
1. **Presentation Layer (Qt 6 Widgets):** Manages user interaction, keyboard shortcuts, high-density tabular presentation, and view models. Contains zero business calculations.
2. **Application Services Layer:** Orchestrates business operations, transaction boundaries, and coordinates between domain services and repositories.
3. **Accounting Domain Engine:** Pure domain models, accounting rules, double-entry mathematical invariants, exact decimal arithmetic, and validation. Has zero dependencies on Qt GUI widgets.
4. **Data Access & Repository Layer:** Encapsulates persistence using SQLite via Qt SQL (with interfaces designed for future PostgreSQL drop-in replacement).
5. **Integration Layer:** Handles external Tally XML streaming, canonical JSON exchange, and Tally HTTP synchronization.

```
+-------------------------------------------------------------------+
|                     Presentation Layer (Qt 6)                     |
|  MainWindow, GatewayView, VoucherEntryView, ReportsView, Dialogs  |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
|                    Application Services Layer                     |
|  CompanyService, LedgerService, VoucherService, ReportService    |
+-------------------------------------------------------------------+
             |                                    |
             v                                    v
+-----------------------------+      +------------------------------+
|   Accounting Domain Engine  |      |      Integration Layer       |
| Double-entry, ExactDecimal, |      | Tally XML, JSON, HTTP Client |
| Invariants, Account Trees   |      +------------------------------+
+-----------------------------+                     |
             |                                      |
             +------------------+-------------------+
                                |
                                v
+-------------------------------------------------------------------+
|                    Repository / Data Access                       |
|   DatabaseManager, CompanyRepo, LedgerRepo, VoucherRepo, Qt SQL   |
+-------------------------------------------------------------------+
                                |
                                v
+-------------------------------------------------------------------+
|                       SQLite3 / WAL Mode                          |
|         (Future: PostgreSQL Multi-User Server Backend)            |
+-------------------------------------------------------------------+
```

---

## 2. Component Design

### 2.1 Accounting Domain Engine (`src/domain/`)
- **`ExactDecimal`:** Arbitrary/fixed-point decimal implementation enforcing 4 decimal places for internal storage and 2 decimal places for display. Guarantees zero IEEE-754 binary floating-point drift.
- **`DoubleEntryValidator`:** Enforces $\sum \text{Debits} = \sum \text{Credits}$ at voucher submission time.
- **`AccountTree`:** Recursive tree structure for group rollups in financial statements.

### 2.2 Application Services (`src/services/`)
- **`CompanyService`:** Creation, selection, alteration, and automated chart of accounts seeding.
- **`LedgerService`:** Management of ledgers, opening balances, and point-in-time statement generation.
- **`VoucherService`:** Atomic posting, validation of duplicate voucher numbers, reversal, and cancellation.
- **`ReportService`:** Trial Balance, Profit & Loss, Balance Sheet, and Day Book generators.

### 2.3 Data Access Layer (`src/database/`)
- **`DatabaseManager`:** Manages database connection pool, WAL configuration, foreign key enforcement (`PRAGMA foreign_keys = ON;`), and schema versioning/migrations.
- **Repositories:** Abstract interfaces (`ILedgerRepository`, `IVoucherRepository`) allowing switching from SQLite to PostgreSQL without touching domain engine code.

### 2.4 User Interface Layer (`src/ui/`)
- Built entirely on **Qt 6 Widgets**.
- Uses `QTableView` with custom `QAbstractTableModel` implementations for large accounting datasets.
- Custom `ShortcutManager` and `NavigationManager` for keyboard-first Tally workflow (hotkeys, Enter to accept, Esc to back out).
