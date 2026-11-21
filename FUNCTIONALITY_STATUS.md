# Functionality Matrix & Status Checklist

**Application:** Antigravity Accounting (Native C++20 / Qt 6)  
**Tracking File:** `FUNCTIONALITY_STATUS.md`  
**Last Updated:** November 21, 2026  

---

## 1. Company Management

- [x] **Create company**: Full creation with standard chart of accounts seeding.
- [x] **Select company**: Select active company via `CompanyDialog` (Alt+K / F3) or menu.
- [x] **Alter company**: Edit company trade name, address, GSTIN, PAN, and contact details.
- [x] **Shut company**: Close active company session cleanly.
- [x] **Configure financial year**: Specify FY beginning date with automatic 12-month period.
- [x] **Configure books beginning date**: Support books beginning date on or after FY start date.
- [x] **Configure currency**: Currency symbol (`₹`), formal name (`INR`), and decimal places (`2`).
- [x] **Save company information**: Atomically persisted in SQLite `companies` table.
- [x] **Reopen application and verify persistence**: Active company remembered and reloaded on launch.
- [x] **Switch between companies without mixing their data**: Strict database isolation on `company_id`.

---

## 2. Groups and Ledgers

- [x] **Create primary group**: Create root accounting groups with defined nature (`Asset`, `Liability`, `Income`, `Expense`, `Equity`).
- [x] **Create subgroup**: Create nested child groups linked to parent groups.
- [x] **Alter group**: Modify group name, parent group, nature, and gross profit flag.
- [x] **Display group**: View group hierarchy in Chart of Accounts and Group Summary.
- [x] **Create ledger**: Create ledger accounts under any valid group with opening balance.
- [x] **Alter ledger**: Modify ledger name, group, address, GSTIN, PAN, and opening balances.
- [x] **Display ledger**: View ledger accounts and statements.
- [x] **Select parent group**: Dynamic searchable group selection in ledger creation.
- [x] **Enter opening balance**: Exact decimal opening balance input.
- [x] **Select debit or credit balance**: Explicit `Dr` or `Cr` balance type selection.
- [x] **Search ledgers**: Incremental search and filtering in selection dialogs.
- [x] **Prevent invalid duplicate records**: Duplicate group and ledger names rejected within company.
- [x] **Validate ledger references**: Foreign key integrity prevents orphan ledgers or invalid groups.
- [x] **Display ledger hierarchy**: Hierarchical rollup in Trial Balance and Group Summary.
- [x] **Verify group and ledger balances**: Exact decimal balance calculations without rounding errors.

---

## 3. Voucher Management

- [x] **Payment voucher**: F5 shortcut and sidebar entry for cash/bank payments.
- [x] **Receipt voucher**: F6 shortcut and sidebar entry for incoming funds.
- [x] **Contra voucher**: F4 shortcut and sidebar entry for bank/cash inter-transfers.
- [x] **Journal voucher**: F7 shortcut for general adjustment entries.
- [x] **Sales voucher**: F8 shortcut for revenue entries.
- [x] **Purchase voucher**: F9 shortcut for procurement entries.
- [x] **Create voucher**: Multi-line entry with real-time debit and credit balancing.
- [x] **Alter supported vouchers**: Reversal and replacement workflows with audit trail.
- [x] **Display voucher**: Drill-down from Day Book and Ledger Statement to Voucher Details.
- [x] **Cancel voucher**: Cancel voucher without deleting database records; reverses ledger balances.
- [x] **Reverse voucher**: Creates linked offsetting counter-voucher (`REV-...`).
- [x] **Search vouchers**: Filter vouchers by date range, voucher type, category, or ledger in Day Book.
- [x] **Navigate between voucher entries**: Keyboard Tab/Enter navigation across line items.
- [x] **Validate voucher numbering**: Auto-incremented voucher numbers with duplicate detection.
- [x] **Validate debit and credit**: Strict invariant enforcement ($\sum \text{Dr} = \sum \text{Cr}$).
- [x] **Validate ledger selection**: All line items must specify valid ledgers belonging to the active company.
- [x] **Save narration**: Persist voucher header and line-level narration.
- [x] **Persist all voucher entries**: Atomic database insertion across `vouchers` and `voucher_entries`.

---

## 4. Reports

- [x] **Day Book**: Chronological journal of all vouchers with date filtering and drill-down.
- [x] **Ledger Statement**: Detailed statement for any ledger with opening balance, line items, and closing balance.
- [x] **Ledger Vouchers**: Direct drill-down from statement into individual voucher entries.
- [x] **Group Summary**: Aggregate performance of all subgroups and ledgers under a parent group.
- [x] **Cash Book**: Specialized ledger statement for default Cash-in-Hand.
- [x] **Bank Book**: Specialized ledger statement for Bank Accounts.
- [x] **Trial Balance**: Complete hierarchical chart of accounts rollup verifying debits equal credits.
- [x] **Profit & Loss**: Trading account gross profit calculation + operating net profit calculation.
- [x] **Balance Sheet**: Dual-pane T-format report reconciling Assets = Liabilities + Capital + Current Net Profit.

---

## 5. Application Operations

- [x] **All menu items work**: Gateway, Top Bar, and Context menus execute verified actions.
- [x] **All dropdowns work**: Voucher type, Dr/Cr, group, and ledger dropdowns respond smoothly.
- [x] **All selection dialogs work**: Company, master, date, and import/export dialogs.
- [x] **Search works**: Real-time filtering in Go To, Master Selection, and Reports.
- [x] **Back navigation works**: Back buttons and `Esc` return smoothly up the navigation hierarchy.
- [x] **Escape works**: Context-sensitive `Esc` handles dialog cancellation, back navigation, and quit confirmation.
- [x] **Keyboard shortcuts work**: `Alt+G`, `Alt+K`, `Alt+Y`, `Alt+Z`, `Alt+O`, `Alt+E`, `F1`-`F9`, `Ctrl+A`.
- [x] **Date selection works**: `F2` updates active voucher entry date.
- [x] **Report period selection works**: `Alt+F2` configures from/to reporting periods.
- [x] **Error messages work**: Clear, non-crashing dialogs explain validation or constraint failures.
- [x] **Confirmation dialogs work**: Standard Tally-style "Accept? (Yes/No)" prompts on saving.
- [x] **Application restart preserves data**: SQLite database persists all transactions and masters across sessions.
