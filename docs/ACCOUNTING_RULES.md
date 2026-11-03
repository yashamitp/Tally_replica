# Accounting Rules & Invariants

## 1. Core Invariants

### 1.1 Fundamental Accounting Equation
At all times across any accounting period:
$$\text{Total Assets} = \text{Total Liabilities} + \text{Total Equity}$$
Where:
$$\text{Total Equity} = \text{Opening Capital} + \sum \text{Capital Injections} - \sum \text{Drawings} + \text{Net Profit (or } - \text{Net Loss)}$$

### 1.2 Double-Entry Posting Invariant
Every voucher must balance strictly to zero discrepancy before being persisted:
$$\sum \text{Debits} - \sum \text{Credits} = 0.0000$$
Rules:
- Line entries per voucher $\ge 2$.
- Must contain at least one Debit (`DR`) entry and at least one Credit (`CR`) entry.
- Entry amount $> 0.0000$. Negative amounts in line entries are prohibited; counter entries must be posted as the opposing type (`DR` or `CR`).

---

## 2. Classification of Accounts (5 Natures)

| Account Nature | Debit Effect (`DR`) | Credit Effect (`CR`) | Normal Balance Side | Appears On |
| :--- | :---: | :---: | :---: | :--- |
| **ASSET** | Increase (+) | Decrease (-) | Debit (`DR`) | Balance Sheet |
| **LIABILITY** | Decrease (-) | Increase (+) | Credit (`CR`) | Balance Sheet |
| **EQUITY** | Decrease (-) | Increase (+) | Credit (`CR`) | Balance Sheet |
| **INCOME** | Decrease (-) | Increase (+) | Credit (`CR`) | Profit & Loss |
| **EXPENSE** | Increase (+) | Decrease (-) | Debit (`DR`) | Profit & Loss |

---

## 3. Financial Statement Rules

### 3.1 Trial Balance
1. Opening balances are calculated point-in-time from company inception up to `FromDate - 1`.
2. Transactions between `FromDate` and `ToDate` (where `is_cancelled = 0`) are aggregated.
3. Closing balance for each ledger:
   $$\text{Closing} = \text{Opening} + \text{Period Debits} - \text{Period Credits}$$
4. Rollup occurs hierarchically through sub-groups to primary groups.
5. Invariant check:
   $$\sum_{\text{leaf ledgers}} \text{Closing Debits} = \sum_{\text{leaf ledgers}} \text{Closing Credits}$$

### 3.2 Profit & Loss Statement
1. **Trading Account:**
   - $\text{Direct Income} = \sum \text{Income Groups with } \texttt{affects\_gross\_profit} = \text{true}$
   - $\text{Direct Expenses} = \sum \text{Expense Groups with } \texttt{affects\_gross\_profit} = \text{true}$
   - $\text{Gross Profit} = \text{Direct Income} - \text{Direct Expenses}$ (if negative, it is Gross Loss).
2. **Operating Account:**
   - $\text{Indirect Income} = \sum \text{Income Groups with } \texttt{affects\_gross\_profit} = \text{false}$
   - $\text{Indirect Expenses} = \sum \text{Expense Groups with } \texttt{affects\_gross\_profit} = \text{false}$
   - $\text{Net Profit} = \text{Gross Profit} + \text{Indirect Income} - \text{Indirect Expenses}$ (if negative, it is Net Loss).

### 3.3 Balance Sheet
1. Assets are aggregated across all groups with `nature = 'ASSET'`.
2. Liabilities are aggregated across all groups with `nature = 'LIABILITY'`.
3. Equity is aggregated across `nature = 'EQUITY'` plus current period Net Profit (or minus Net Loss).
4. If $\text{Total Assets} \ne \text{Total Liabilities} + \text{Total Equity}$, an explicit diagnostic out-of-balance error is triggered.

---

## 4. Reversal and Cancellation Policies

### 4.1 Reversal Policy
- Original voucher remains unchanged in the database with status updated to `REVERSED`.
- A counter-voucher is automatically generated with voucher number prefixed by `REV-` and entries swapped (`DR` $\leftrightarrow$ `CR`).
- Running ledger balances are adjusted symmetrically.
- The link between original and counter-voucher is maintained via `reversal_voucher_id`.

### 4.2 Cancellation Policy
- A cancelled voucher is never deleted.
- Status is changed to `CANCELLED` and `is_cancelled` is set to `1`.
- Any historical ledger balance effects are rolled back.
- Cancelled vouchers continue to appear on Day Book and audit reports for transparency.
