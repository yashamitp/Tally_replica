# Comprehensive Existing Project Analysis

**Reference Implementation:** `@tally-replica` (TypeScript, Node.js 22, Fastify, PGlite/PostgreSQL, React, Electron)  
**Target Architecture:** Native C++20, Qt 6 Widgets, CMake, SQLite (future PostgreSQL)  
**Date of Audit:** November 3, 2026  
**Auditor:** Antigravity AI Pair Programming Agent  

---

## 1. Executive Summary

The existing project located at `/home/yash/projects/tally-replica` is an accounting software prototype designed to provide a keyboard-driven desktop experience replicating TallyPrime and Tally.ERP 9. It is structured as an npm monorepo with three packages:
1. `@tally-replica/core`: Pure domain types, decimal arithmetic, and validation invariants.
2. `@tally-replica/backend`: Headless accounting engine, embedded PostgreSQL database (`@electric-sql/pglite` with Drizzle ORM), Fastify HTTP REST API, Tally XML/JSON exchange adapters, and inventory/backup services.
3. `@tally-replica/desktop`: React 18, Vite, Tailwind CSS desktop frontend, coupled with an Electron shell and Tally-inspired UI components.

This analysis inventories every component, invariant, rule, database relationship, API route, and test suite in the reference TypeScript codebase to establish exact functional parity targets for the C++20 / Qt 6 native application.

---

## 2. Feature Inventory: Implemented, Partially Implemented, and Not Implemented

### 2.1 Implemented Features

1. **Exact Decimal Arithmetic:**
   - Implemented in `packages/core/src/domain/decimal.ts` using `decimal.js`.
   - 28-digit precision, `ROUND_HALF_UP`.
   - Methods: `add`, `sub`, `mul`, `div` (zero-checked), `abs`, `neg`, `equals`, `greaterThan`, `lessThan`, `isZero`, `isPositive`, `isNegative`, `isNonNegative`.
   - String formatters: `toDbString()` (4 decimal places), `toCurrencyString()` (2 decimal places), `formatInr()` (Indian numbering format: lakhs and crores grouping, optional `₹` symbol).

2. **Company Lifecycle & Standard Master Seeding:**
   - Implemented in `packages/backend/src/engine/company-service.ts`.
   - Creation of company records with currency (`₹`, `INR`, decimal places = 2).
   - Automatic seeding of initial Financial Year (from specified start date to March 31 of following year).
   - Automatic seeding of 12 Primary Accounting Groups (`Capital Account`, `Loans (Liability)`, `Current Liabilities`, `Fixed Assets`, `Investments`, `Current Assets`, `Sales Accounts`, `Purchase Accounts`, `Direct Incomes`, `Direct Expenses`, `Indirect Incomes`, `Indirect Expenses`).
   - Automatic seeding of 8 Standard Sub-groups (`Bank Accounts`, `Cash-in-Hand`, `Sundry Debtors`, `Sundry Creditors`, `Duties & Taxes`, `Bank OCC A/c`, `Secured Loans`, `Unsecured Loans`).
   - Automatic seeding of 6 standard Voucher Types (`Journal`, `Payment`, `Receipt`, `Contra`, `Sales`, `Purchase`).
   - Automatic creation of the default `Cash` ledger under `Cash-in-Hand`.
   - Recording of audit events for company and master initialization.

3. **Master Creation & Alteration:**
   - Implemented in `packages/backend/src/engine/ledger-service.ts` and API routes.
   - Creation of custom Ledgers with parent group validation, unique name per company, opening balance, opening balance type (`DR`/`CR`), GSTIN, PAN, address, and metadata.
   - Alteration of existing ledgers with updating of `currentBalance` and audit trails.
   - Group creation supporting hierarchical nesting (`parentId`).

4. **Transactional Voucher Posting & Double-Entry Invariants:**
   - Implemented in `packages/backend/src/engine/accounting-engine.ts`.
   - Strict double-entry balance validation ($\sum \text{Debits} = \sum \text{Credits}$) enforced via `validateDoubleEntryBalance()`.
   - At least 2 entries required (minimum one Debit and one Credit).
   - All amounts strictly positive decimals ($> 0$).
   - Atomic database transactions with rollback on failure.
   - Company isolation validation: all ledgers and voucher types verified to belong to the active company.
   - Duplicate voucher number detection within the same company and voucher type.
   - Real-time running balance adjustments for ledgers based on group nature.

5. **Voucher Reversal & Cancellation:**
   - Reversal: Creates an offsetting counter-voucher (`REV-<VoucherNumber>-<timestamp>`), inverts line items (`DR` $\leftrightarrow$ `CR`), adjusts running ledger balances, updates original voucher status to `REVERSED`, sets `isReversed = true`, preserves link via `reversalVoucherId`.
   - Cancellation: Reverses ledger balance adjustments without deleting rows, marks voucher status as `CANCELLED`, sets `isCancelled = true`, logs cancellation reason in audit events.

6. **Financial Reports:**
   - **Trial Balance:** Hierarchical tree rollup of groups and leaf ledgers, opening debit/credit, period movements, closing balances, zero-balance filtering, mathematical balance verification ($\sum \text{Debits} = \sum \text{Credits}$).
   - **Profit & Loss:** Four-way classification (Direct Income, Direct Expenses, Indirect Income, Indirect Expenses), Trading Account calculation (Gross Profit / Gross Loss), Operating Account calculation (Net Profit / Net Loss).
   - **Balance Sheet:** Assets = Liabilities + Equity ($\text{Capital} \pm \text{Current Period Net Profit/Loss}$), hierarchical group structures, discrepancy detection.
   - **Day Book:** Date-filtered transactions, voucher category filtering, pagination, particulars generation (`Party / Counter-party`), debit/credit totals.
   - **Ledger Statement:** Opening balance calculation as of report start date, sequential period transactions, opposing ledger particulars display, running net balance calculation (`DR`/`CR`).

7. **Tally Data Exchange:**
   - **Tally XML:** Parsing Tally XML payloads (`TALLYMESSAGE`, `VOUCHER`, `ALLLEDGERENTRIES.LIST`), mapping Tally negative/positive conventions to DR/CR, handling `ISDEEMEDPOSITIVE`, mapping date formats (`YYYYMMDD` $\leftrightarrow$ `YYYY-MM-DD`), previewing imports, executing imports with duplicate skipping and transaction safety, exporting Masters and Vouchers in Tally-compatible XML.
   - **JSON Exchange:** Canonical JSON import/export for Masters and Vouchers, preview generation, conflict detection.
   - **Tally HTTP Client:** Port 9000 client with ping check, active company detection, sync cursor tracking via `ALTERID`, pull preview, and push execution.

8. **Inventory Tracking & Stock Valuation:**
   - Implemented in `packages/backend/src/engine/inventory/`.
   - Units of Measurement (e.g., Nos, Pcs, Kgs, Box) with decimal places.
   - Stock Items with HSN code, GST rate, opening quantity, rate, and value.
   - Stock movements (`INWARD` / `OUTWARD`) tied to vouchers.
   - FIFO and Weighted Average stock valuation algorithms.

9. **Backup & Restore:**
   - Implemented in `packages/backend/src/engine/backup-restore-service.ts`.
   - Full JSON snapshot of all 11 database entities.
   - SHA-256 checksum manifest and entity count verification.
   - Safe restoration into original or newly named target company.
   - Post-restore Trial Balance verification.

### 2.2 Partially Implemented Features

1. **Desktop Shell Integration:**
   - The desktop client relies on Electron wrapping a Vite web server. It lacks native OS menus, native printing, OS file dialog integration, and offline single-file installer distribution.
2. **Keyboard Navigation & Hotkey Interception:**
   - The React frontend captures some keys (`F1`, `F12`, `Alt+G`, `Escape`), but input elements capture focus arbitrarily. True Tally keyboard navigation (seamless Enter/Tab across fields, Esc to drill up without losing state, single-key Gateway hotkeys like `V`, `K`, `D`, `B`) is only partially mimicked.
3. **Multi-User / Server Mode:**
   - The Fastify backend can bind to HTTP, but authentication, session management, user roles, and network row locking are not implemented.
4. **Voucher Alteration in UI:**
   - The backend supports voucher retrieval and ledger update; the UI can populate the voucher entry form with an existing voucher, but an edit currently posts or cancels rather than cleanly updating or re-verifying consecutive voucher numbers.

### 2.3 Not Implemented Features

1. **GST Return Forms (GSTR-1, GSTR-3B):** Only basic GSTIN and HSN fields exist; statutory return computation is not present.
2. **Cheque Printing & Bank Reconciliation Statement (BRS):** Bank ledgers exist, but BRS reconciliation dates and cheque layout printing are not implemented.
3. **Bill-wise Accounting (Outstanding Reference Tracking):** In Tally, debtors and creditors can have bill-by-bill references (`New Ref`, `Agst Ref`, `Advance`, `On Account`). In the TypeScript version, outstandings are tracked purely on a net ledger balance basis.
4. **Cost Centers & Cost Categories:** Not implemented.
5. **Multi-Currency Transactions:** Only a single company-level currency is supported.

---

## 3. Existing Accounting Rules and Invariants

### 3.1 Double-Entry Balance Invariant
Every voucher must satisfy:
$$\sum_{i \in \text{Debits}} \text{Amount}_i = \sum_{j \in \text{Credits}} \text{Amount}_j$$
- Minimum of 2 entries (at least 1 `DR` and 1 `CR`).
- Every entry amount must be strictly positive ($> 0$).
- Discrepancy threshold: strictly zero ($0.0000$).

### 3.2 Running Balance Update Rule
Ledger balance updates are computed according to group nature:
- For **ASSET** and **EXPENSE**:
  $$\text{NewBalance} = \begin{cases} \text{CurrentBalance} + \text{Amount} & \text{if Entry is } \mathbf{DR} \\ \text{CurrentBalance} - \text{Amount} & \text{if Entry is } \mathbf{CR} \end{cases}$$
- For **LIABILITY**, **EQUITY**, and **INCOME**:
  $$\text{NewBalance} = \begin{cases} \text{CurrentBalance} + \text{Amount} & \text{if Entry is } \mathbf{CR} \\ \text{CurrentBalance} - \text{Amount} & \text{if Entry is } \mathbf{DR} \end{cases}$$

### 3.3 Sign Conventions in Reports
- **Trial Balance:**
  - Net positive on normal side is Debit (`DR`), net negative is Credit (`CR`).
  - Total Debits across all leaf ledgers must equal Total Credits across all leaf ledgers.
- **Profit & Loss:**
  - Direct Income: Income groups with `affectsGrossProfit = true`
  - Direct Expense: Expense groups with `affectsGrossProfit = true`
  - $\text{Gross Profit} = \sum \text{Direct Income} - \sum \text{Direct Expenses}$
  - $\text{Net Profit} = \text{Gross Profit} + \sum \text{Indirect Income} - \sum \text{Indirect Expenses}$
- **Balance Sheet:**
  - Assets: Sum of all Asset groups.
  - Liabilities: Sum of all Liability groups.
  - Equity: $\sum \text{Capital Accounts} + \text{Current Period Net Profit}$ (or minus Net Loss).
  - Equation: $\text{Total Assets} = \text{Total Liabilities} + \text{Total Equity}$.

### 3.4 Reversal & Cancellation Rules
- A posted voucher cannot be deleted.
- **Reversal:** Leaves the original voucher intact, marks status as `REVERSED`, creates a new voucher with inverted entries (`DR` $\leftrightarrow$ `CR`), and adjusts running balances.
- **Cancellation:** Marks status as `CANCELLED`, `isCancelled = true`, subtracts the posted entries from current ledger balances, and retains the voucher for Day Book and audit history.

---

## 4. Existing Database Relationships & Schema

The reference implementation uses 11 tables with foreign keys and uniqueness constraints:

```
+--------------------+       1:N       +-------------------------+
|     companies      | <-------------- |     financial_years     |
+--------------------+                 +-------------------------+
   |              |
   | 1:N          | 1:N
   v              v
+-------------+  +--------------------+  1:N  +--------------------+
| voucher_typ |  |  accounting_groups | <---  |  accounting_groups | (Self-ref: parent_id)
+-------------+  +--------------------+       +--------------------+
   |               |
   |               | 1:N
   |               v
   | 1:N         +--------------------+  1:N  +--------------------+
   |  +--------> |      ledgers       | <---  |   voucher_entries  |
   |  |          +--------------------+       +--------------------+
   v  |                                                 ^
+-------------+         1:N                             |
|  vouchers   | ----------------------------------------+
+-------------+
   |  |
   |  | 1:N
   |  +----------------------------+
   |                               v
   | 1:N                 +--------------------+
   +-------------------> |  stock_movements   |
                         +--------------------+
                                   ^
                                   | 1:N
+--------------------+   1:N     +--------------------+
| units_of_measure   | --------> |  inventory_items   |
+--------------------+           +--------------------+
```

### Table Summary:
1. `companies`: Primary metadata, currency settings, timestamps.
2. `financial_years`: Dates, `company_id` foreign key with CASCADE, uniqueness on `(company_id, start_date, end_date)`.
3. `accounting_groups`: Hierarchy (`parent_id`), nature (`ASSET`, `LIABILITY`, `EQUITY`, `INCOME`, `EXPENSE`), `affects_gross_profit`, uniqueness on `(company_id, name)`.
4. `ledgers`: Belongs to `company_id` and `group_id` (RESTRICT on delete), `opening_balance`, `current_balance` as `NUMERIC(18, 4)`.
5. `voucher_types`: Belongs to `company_id`, category (`JOURNAL`, `PAYMENT`, etc.), uniqueness on `(company_id, name)`.
6. `vouchers`: Header containing `voucher_number`, `date`, `total_amount`, `status`, `reversal_voucher_id`, uniqueness on `(company_id, voucher_type_id, voucher_number)`.
7. `voucher_entries`: Line items referencing `voucher_id` (CASCADE) and `ledger_id` (RESTRICT), `entry_type` (`DR`/`CR`), `amount` as `NUMERIC(18, 4)`, `line_index`.
8. `units_of_measurement`: Measurement units (`Nos`, `Box`) with decimal places.
9. `inventory_items`: Stock items with `unit_id`, opening stock, rates.
10. `stock_movements`: Stock transaction lines linked optionally to `voucher_id`.
11. `external_identifiers`: Tally GUIDs, `MASTERID`, `ALTERID` mapping for incremental synchronization.
12. `audit_events`: Immutable audit trail for all operations.

---

## 5. Existing API Routes (Fastify REST)

| Route | Method | Purpose |
| :--- | :--- | :--- |
| `/health` | GET | System health & timestamp |
| `/api/companies` | POST | Create new company & seed chart of accounts |
| `/api/companies` | GET | List all registered companies |
| `/api/companies/:id` | GET | Get company details |
| `/api/companies/:companyId/groups` | GET/POST | List and create accounting groups |
| `/api/companies/:companyId/ledgers` | GET/POST | List and create ledgers |
| `/api/companies/:companyId/ledgers/:id` | PUT | Alter existing ledger details |
| `/api/companies/:companyId/ledgers/:id/statement` | GET | Ledger statement with running balance |
| `/api/companies/:companyId/voucher-types` | GET | List standard & custom voucher types |
| `/api/companies/:companyId/vouchers` | POST | Transactionally post a new voucher |
| `/api/companies/:companyId/vouchers/:id` | GET | Retrieve voucher with line entries |
| `/api/companies/:companyId/vouchers/:id/reverse` | POST | Create counter-voucher & reverse |
| `/api/companies/:companyId/vouchers/:id/cancel` | POST | Cancel voucher and roll back balances |
| `/api/companies/:companyId/daybook` | GET | Paginated Day Book report |
| `/api/companies/:companyId/reports/trial-balance` | GET | Hierarchical Trial Balance report |
| `/api/companies/:companyId/reports/profit-loss` | GET | Trading & P&L Statement |
| `/api/companies/:companyId/reports/balance-sheet` | GET | Balance Sheet with Net Profit integration |
| `/api/companies/:companyId/import/tally-xml/preview` | POST | Preview Tally XML import |
| `/api/companies/:companyId/import/tally-xml/execute` | POST | Execute Tally XML import |
| `/api/companies/:companyId/export/tally-xml/masters` | GET | Export Masters in Tally XML format |
| `/api/companies/:companyId/export/tally-xml/vouchers` | GET | Export Vouchers in Tally XML format |
| `/api/companies/:companyId/export/json/masters` | GET | Export Masters in canonical JSON |
| `/api/companies/:companyId/export/json/vouchers` | GET | Export Vouchers in canonical JSON |
| `/api/companies/:companyId/import/json/preview` | POST | Preview canonical JSON import |
| `/api/companies/:companyId/import/json/execute` | POST | Execute canonical JSON import |
| `/api/companies/:companyId/sync/tally/test-connection`| POST | Probe running Tally instance on port 9000 |
| `/api/companies/:companyId/sync/tally/cursor` | GET | Get highest synced `ALTERID` |
| `/api/companies/:companyId/sync/tally/pull-preview` | POST | Preview delta sync from Tally |
| `/api/companies/:companyId/sync/tally/pull-execute` | POST | Execute delta sync from Tally |
| `/api/companies/:companyId/sync/tally/push` | POST | Push posted vouchers into Tally |
| `/api/companies/:companyId/inventory/units` | GET/POST | List and create measurement units |
| `/api/companies/:companyId/inventory/items` | GET/POST | List and create stock items |
| `/api/companies/:companyId/inventory/movements` | GET/POST | List and record stock movements |
| `/api/companies/:companyId/inventory/stock-summary` | GET | FIFO/Weighted Average stock valuation report |
| `/api/companies/:companyId/backup` | GET | Export full company backup payload |
| `/api/companies/backup/verify` | POST | Verify backup checksum & entity counts |
| `/api/companies/restore` | POST | Restore company from backup payload |

---

## 6. Inventory of Existing Automated Tests

The reference test suite comprises 18 test files using Vitest:

| Test File | Focus Area | Scenarios Verified |
| :--- | :--- | :--- |
| `decimal.test.ts` | Unit | Eliminates floating-point drift (`0.1 + 0.2 = 0.3`), division by zero check, rounding half-up. |
| `validation.test.ts` | Unit | Zod schema bounds, double-entry mathematical balancing, rejection of single-sided entries. |
| `accounting-engine.test.ts` | Integration | Transactional posting, balance updates for all 5 group natures, atomic rollback on error, unique voucher numbers. |
| `company-service.test.ts` | Integration | Company creation, automatic seeding of 12 primary groups, sub-groups, voucher types, default Cash ledger. |
| `ledger-statement.test.ts` | Integration | Statement generation, opening balance calculation across date filters, running balances, opposing particulars. |
| `day-book.test.ts` | Integration | Day Book filtering by date, voucher type, ledger, status, pagination, debit/credit summations. |
| `voucher-reversal.test.ts` | Integration | Counter-voucher generation, entry inversion (`DR` $\leftrightarrow$ `CR`), original voucher status preservation, cancellation. |
| `trial-balance.test.ts` | Financial | Mathematical reconciliation ($\text{Total Debits} = \text{Total Credits}$), mid-year start dates, reversed voucher handling. |
| `profit-loss.test.ts` | Financial | Direct vs Indirect split, Trading Account gross profit/loss, Net Profit calculation. |
| `balance-sheet-and-reconciliation.test.ts` | Financial | Fundamental equation ($\text{Assets} = \text{Liabilities} + \text{Equity}$), current period profit linkage, triple reconciliation. |
| `tally-xml.test.ts` | Compatibility | Tally XML parsing, date mapping, `ISDEEMEDPOSITIVE` handling, negative amounts, preview/execute import. |
| `json-data-exchange.test.ts` | Compatibility | Canonical JSON export, preview, duplicate detection, round-trip export-import with reconciliation. |
| `tally-http-gateway.test.ts` | Compatibility | Port 9000 client communication, ping, cursor tracking, push/pull workflows. |
| `conflict-sync.test.ts` | Compatibility | Conflict detection, fingerprinting, three-way merge resolution. |
| `multi-version-compatibility.test.ts`| Compatibility | Handling varied Tally XML schemas across Tally 9, ERP 9, and TallyPrime. |
| `inventory-engine.test.ts` | Inventory | Unit creation, item opening values, inward/outward movements, FIFO and Weighted Average valuations. |
| `backup-restore.test.ts` | System | Full backup generation, SHA-256 manifest check, company restoration, post-restore Trial Balance verification. |
| `api-server.test.ts` | E2E | Fastify HTTP endpoints testing, status codes, error payloads. |

---

## 7. Known Limitations & Potential Bugs in Reference Code

1. **In-Memory Running Balance Drift Risk:**
   - In `AccountingEngine`, `ledgers.currentBalance` is updated incrementally upon each voucher post. If an out-of-order date voucher is inserted or backdated, `currentBalance` reflects the cumulative state but not point-in-time state. Point-in-time balance calculations in `LedgerService` and `TrialBalanceService` correctly re-calculate from transactions, which is superior. The C++ design should treat ledger current balance as an indexed cached aggregate or compute dynamically.
2. **PostgreSQL/PGlite Specifics:**
   - The reference relies on PostgreSQL dialect (`gen_random_uuid()`, `JSONB`, `TIMESTAMPTZ`, `NUMERIC(18, 4)`). SQLite in C++ does not have native `UUID` or `NUMERIC(18, 4)` column types. The C++ implementation must use string-based or 64-bit integer fixed-point representation for SQLite columns while maintaining exact decimal precision in C++.
3. **Floating Point Fallbacks in UI:**
   - In `packages/desktop/src/components/VoucherEntryScreen.tsx` (lines 123-137), `Number(row.amount)` is used for live totals calculation in the browser rather than `ExactDecimal`. This creates minor floating-point display inaccuracies for certain fractions before posting. The C++ Qt UI will bind directly to C++ `ExactDecimal` models, eliminating this defect.
4. **Cancellation Balance Logic:**
   - In `cancelVoucher()` (`accounting-engine.ts`, line 426), cancelling reverses the balance impact on the ledger's `currentBalance` column directly. If subsequent transactions occurred between the date of the voucher and the cancellation date, historical point-in-time statements must ensure the cancelled voucher's exclusion does not disrupt historical sequences. The C++ application must ensure that queries filter on `is_cancelled = 0` consistently.

---

## 8. C++20 / Qt 6 Functional Parity Targets

| Capability | Reference TypeScript Module | Native C++20 / Qt 6 Target |
| :--- | :--- | :--- |
| **Decimal Arithmetic** | `decimal.js` wrapper (`ExactDecimal`) | Custom `ExactDecimal` class with 64-bit/128-bit fixed-point (or `boost::multiprecision::cpp_dec_float_50`) |
| **Domain Models & Validation** | `zod` schemas & TypeScript interfaces | C++20 structs (`Company`, `Ledger`, `Voucher`, `VoucherEntry`) with validation methods |
| **Database Persistence** | PGlite / Drizzle ORM | SQLite3 with WAL mode, accessed via `QSqlDatabase` / Qt SQL and repository pattern |
| **Application Layer** | Fastify REST controllers & services | C++ Application Services (`CompanyService`, `LedgerService`, `VoucherService`, `ReportService`) |
| **Presentation Layer** | React, Tailwind, Vite, Electron | Qt 6 Widgets (`QMainWindow`, `QStackedWidget`, `QTableView`, custom QSS styling) |
| **Tally XML Parsing/Writing** | `fast-xml-parser` | `QXmlStreamReader` and `QXmlStreamWriter` |
| **JSON Exchange** | Native JavaScript `JSON` | `QJsonDocument`, `QJsonObject`, `QJsonArray` |
| **HTTP Tally Port 9000** | Node.js `fetch` / `http` | `QNetworkAccessManager`, `QNetworkRequest`, `QNetworkReply` |
| **Unit & Accounting Testing** | Vitest | GoogleTest (`gtest`) and Qt Test framework |

---
*Analysis completed and verified against all source files in `/home/yash/projects/tally-replica`.*
