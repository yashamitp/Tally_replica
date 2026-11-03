# Migration Status: TypeScript to C++20

## 1. Migration Strategy Summary

The TypeScript reference implementation (`/home/yash/projects/tally-replica`) serves as the functional blueprint. It remains completely preserved and untouched. The C++ implementation (`/home/yash/projects/TallyReplica`) is being constructed module-by-module.

| Module | TypeScript Reference | C++ Target | Status |
| :--- | :--- | :--- | :---: |
| **ExactDecimal** | `decimal.js` | `src/domain/decimal/ExactDecimal.hpp` | Planned |
| **Domain Models** | `@tally-replica/core` | `src/domain/` structs & enums | Planned |
| **Database Client** | `@electric-sql/pglite` + Drizzle | `QSqlDatabase` SQLite3 in WAL mode | Planned |
| **Company Service** | `CompanyService.ts` | `src/services/CompanyService.hpp` | Planned |
| **Ledger Service** | `LedgerService.ts` | `src/services/LedgerService.hpp` | Planned |
| **Accounting Engine** | `AccountingEngine.ts` | `src/domain/accounting/AccountingEngine.hpp` | Planned |
| **Voucher Service** | `AccountingEngine.ts` | `src/services/VoucherService.hpp` | Planned |
| **Day Book Service** | `DayBookService.ts` | `src/services/DayBookService.hpp` | Planned |
| **Trial Balance** | `TrialBalanceService.ts` | `src/services/reports/TrialBalanceService.hpp` | Planned |
| **Profit & Loss** | `ProfitLossService.ts` | `src/services/reports/ProfitLossService.hpp` | Planned |
| **Balance Sheet** | `BalanceSheetService.ts`| `src/services/reports/BalanceSheetService.hpp`| Planned |
| **Tally XML Import** | `TallyXmlImportService.ts` | `src/integrations/xml/TallyXmlImporter.hpp` | Planned |
| **Tally XML Export** | `TallyXmlExportService.ts` | `src/integrations/xml/TallyXmlExporter.hpp` | Planned |
| **JSON Exchange** | `JsonExchangeService.ts` | `src/integrations/json/JsonExchange.hpp` | Planned |
| **HTTP Gateway** | `TallyHttpClient.ts` | `src/integrations/tally/TallyHttpClient.hpp` | Planned |
| **Desktop UI Shell**| React / Electron | Qt 6 Widgets (`QMainWindow`) | Planned |
