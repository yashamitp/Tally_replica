# TallyReplica: Native C++20 / Qt 6 Accounting Application

A native, high-performance, keyboard-driven desktop accounting application inspired by TallyPrime, developed in **C++20** and **Qt 6 Widgets**.

---

## Key Highlights

- **Native Desktop Performance:** Built with native C++20 and Qt 6 Widgets; zero JavaScript runtime, zero Electron overhead, instant startup, and minimal memory footprint.
- **Strict Accounting Invariants:** Transactional double-entry accounting engine where $\sum \text{Debits} = \sum \text{Credits}$ is enforced with zero floating-point drift.
- **Exact Fixed-Point Arithmetic:** Arbitrary-precision exact decimal mathematics designed specifically for financial ledgers, currency calculations, and Indian numbering formats (Lakhs & Crores).
- **TallyPrime Keyboard Navigation:** Authentic keyboard-first interface featuring Gateway of Tally, screen-specific function key shortcuts (F1-F12), Alt+G GoTo search, compact tables, and rapid numeric entry.
- **Hierarchical Financial Reports:** Real-time generation of Trial Balance, Balance Sheet ($\text{Assets} = \text{Liabilities} + \text{Equity}$), Profit & Loss Statement, Day Book, and Ledger Statements with running balances.
- **Tally Interoperability:** Support for Tally XML envelopes, JSON exchange formats, and bidirectional port 9000 sync.
- **Robust Persistence:** SQLite with Write-Ahead Logging (WAL) and foreign key enforcement, designed with an abstraction layer for future PostgreSQL server deployment.

---

## Architecture Overview

```
+-----------------------------------------------------------+
|                   Qt 6 Widgets Desktop UI                 |
|   (Gateway, Voucher Entry, Day Book, Reports, Dialogs)    |
+-----------------------------------------------------------+
                              |
+-----------------------------------------------------------+
|                    Application Services                   |
|  (CompanyService, LedgerService, VoucherService, Reports) |
+-----------------------------------------------------------+
                              |
+-----------------------------------------------------------+
|                   Accounting Domain Engine                |
|  (Double-entry invariants, ExactDecimal, Audit, Periods)  |
+-----------------------------------------------------------+
                              |
+-----------------------------------------------------------+
|                 Repository & Data Access Layer            |
|       (Qt SQL, Schema Migrations, SQLite / PostgreSQL)    |
+-----------------------------------------------------------+
```

---

## Building and Running

### Prerequisites
- C++20 compliant compiler (`g++ 13+`, `clang++ 16+`, or MSVC 2022)
- CMake 3.25+
- Qt 6 (Widgets, Core, Gui, Sql, Xml, Network)
- Ninja or Make

### Build Instructions
```bash
# Configure the project
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build executable and test suites
cmake --build build

# Run unit and accounting verification tests
ctest --test-dir build --output-on-failure
```

---

## Documentation

- [Existing Project Analysis](docs/EXISTING_PROJECT_ANALYSIS.md)
- [System Architecture](docs/ARCHITECTURE.md)
- [Database Schema Specification](docs/DATABASE_SCHEMA.md)
- [Accounting Rules & Invariants](docs/ACCOUNTING_RULES.md)
- [Tally Compatibility Specification](docs/TALLY_COMPATIBILITY.md)
- [Development Roadmap](docs/DEVELOPMENT_ROADMAP.md)
- [Testing Strategy](docs/TESTING_STRATEGY.md)
- [UI Implementation Status](docs/UI_IMPLEMENTATION_STATUS.md)
- [Migration Status](docs/MIGRATION_STATUS.md)
- [Known Limitations](docs/KNOWN_LIMITATIONS.md)
- [Feature Development Status](DEVELOPMENT_STATUS.md)
