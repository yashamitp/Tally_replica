# Testing Strategy

## 1. Test Levels

### 1.1 Unit Tests (GoogleTest)
- **`ExactDecimal` Tests:**
  - Invariant: `0.1 + 0.2 == 0.3` (zero binary floating-point drift).
  - Arithmetic operations: addition, subtraction, multiplication, division.
  - Division by zero error handling.
  - Rounding rules: `ROUND_HALF_UP`.
  - Indian numbering formatting (`1,50,000.00`).
- **Validation Tests:**
  - Double-entry balance check ($\sum \text{Dr} = \sum \text{Cr}$).
  - Rejection of single-line entries.
  - Rejection of negative amounts.
  - Company isolation checks.

### 1.2 Integration & Accounting Tests
- **Transactional Posting:**
  - Atomic voucher persistence.
  - Transaction rollback on simulated error.
  - Rejection of duplicate voucher numbers within the same company and voucher type.
- **Reconciliation Invariants:**
  - Trial Balance reconciles: $\sum \text{Debits} == \sum \text{Credits}$.
  - Profit & Loss calculations: $\text{Gross Profit} = \text{Direct Income} - \text{Direct Expense}$, $\text{Net Profit} = \text{Gross Profit} + \text{Indirect Income} - \text{Indirect Expense}$.
  - Balance Sheet equation: $\text{Total Assets} == \text{Total Liabilities} + \text{Total Equity}$.
  - Reversals: counter-voucher restores initial balances with zero distortion.
  - Cancellations: transactions excluded from financial totals while preserving audit trail.

### 1.3 UI & Navigation Tests (Qt Test)
- Hotkey routing (F1-F12, Alt+G).
- Escape key hierarchical back navigation.
- Dense table selection and arrow key movement.
