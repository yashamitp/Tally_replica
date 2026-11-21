# Known Limitations

This document tracks known architectural, platform, and accounting limitations across current and planned phases.

## 1. Single-User SQLite Database Architecture
- **Current Behavior**: The C++ application utilizes SQLite with Write-Ahead Logging (WAL) and foreign keys enabled. It is optimized for ultra-fast, local desktop responsiveness and single-workstation use.
- **Limitation**: SQLite is designed for single-process write locks. Multi-user concurrent networked access (e.g., multiple accountants posting vouchers simultaneously over a LAN) will require the planned client-server PostgreSQL backend.
- **Workaround**: Automated atomic backup/restore is provided via `BackupService` for data portability between workstations.

## 2. Proprietary Tally Binary Files (`.900`)
- **Current Behavior**: Data exchange is performed via standard, documented Tally XML envelopes (`ENVELOPE/BODY/DATA/TALLYMESSAGE`) and JSON export/import formats.
- **Limitation**: The application does not reverse-engineer or parse proprietary, encrypted `.900` binary database files from Tally Solutions.
- **Workaround**: Standard XML/JSON export/import is fully functional and tested for masters and transactions.

## 3. Statutory Return Filing & Direct Portal API
- **Current Behavior**: Basic GSTIN validation, state codes, and tax rates (CGST, SGST, IGST) are calculated and reflected in voucher lines and ledgers.
- **Limitation**: Direct real-time e-way bill generation, e-invoicing JSON upload, and direct GST portal API filing (via GSP/ASP) are not implemented.

## 4. Bill-by-Bill Allocation (`Agst Ref`, `New Ref`)
- **Current Behavior**: Debtor and creditor accounts operate on a running net ledger balance basis.
- **Limitation**: Discrete FIFO bill tracking (allocating a receipt voucher specifically against invoice number X with payment terms and overdue aging buckets) is scheduled for the next major milestone.

## 5. Operating System Platform Differences
- **Current Behavior**: The codebase is written in standard cross-platform C++20 and Qt 6. Key bindings (`F1` through `F10`, `Alt+F1`, `Alt+F2`, `Ctrl+A`, `Esc`) follow standard Tally keyboard ergonomics.
- **Limitation**: On some Linux window managers (such as GNOME or XFCE), system-level shortcuts (such as `Alt+F1` for application menu or `F10` for menu bar) may intercept keys before they reach the Qt application unless configured or run on Windows.

## 6. Voucher Reversal vs. Cancellation Policy
- **Reversal**: Used when correcting a previously finalized voucher while retaining historical audit records. Generates an immutable offsetting counter-voucher (`REV-<VoucherNumber>-<Id>`) that reverses ledger balances.
- **Cancellation**: Marks the voucher status as `CANCELLED`. In accordance with statutory rules, the voucher record and voucher number are preserved in the database and audit trail, but its debit and credit amounts are omitted from financial statements (Trial Balance, P&L, Balance Sheet).
