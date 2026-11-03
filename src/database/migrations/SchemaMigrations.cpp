#include "SchemaMigrations.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
#include "../../utilities/Logger.hpp"

namespace tally::database {

bool SchemaMigrations::applyMigrations(QSqlDatabase& db, std::string& errorOut) {
    if (!db.isOpen()) {
        errorOut = "Database is not open";
        return false;
    }

    const QStringList ddlStatements = {
        // 1. Companies
        R"(
        CREATE TABLE IF NOT EXISTS companies (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            formal_name TEXT,
            address_line1 TEXT,
            address_line2 TEXT,
            city TEXT,
            state TEXT,
            country TEXT NOT NULL DEFAULT 'India',
            pincode TEXT,
            gstin TEXT,
            pan TEXT,
            currency_symbol TEXT NOT NULL DEFAULT '₹',
            currency_formal_name TEXT NOT NULL DEFAULT 'INR',
            decimal_places INTEGER NOT NULL DEFAULT 2,
            is_active INTEGER NOT NULL DEFAULT 1,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        );
        )",

        // 2. Financial Years
        R"(
        CREATE TABLE IF NOT EXISTS financial_years (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            start_date TEXT NOT NULL,
            end_date TEXT NOT NULL,
            books_beginning_from TEXT NOT NULL,
            is_closed INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL,
            CONSTRAINT uq_company_financial_year UNIQUE (company_id, start_date, end_date)
        );
        )",

        // 3. Accounting Groups
        R"(
        CREATE TABLE IF NOT EXISTS accounting_groups (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            parent_id TEXT REFERENCES accounting_groups(id) ON DELETE RESTRICT,
            name TEXT NOT NULL,
            nature TEXT NOT NULL,
            affects_gross_profit INTEGER NOT NULL DEFAULT 0,
            is_primary INTEGER NOT NULL DEFAULT 0,
            is_system INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            CONSTRAINT uq_company_group_name UNIQUE (company_id, name)
        );
        )",

        // 4. Ledgers
        R"(
        CREATE TABLE IF NOT EXISTS ledgers (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            group_id TEXT NOT NULL REFERENCES accounting_groups(id) ON DELETE RESTRICT,
            name TEXT NOT NULL,
            alias TEXT,
            opening_balance TEXT NOT NULL DEFAULT '0.0000',
            opening_balance_type TEXT NOT NULL DEFAULT 'DR',
            current_balance TEXT NOT NULL DEFAULT '0.0000',
            is_system INTEGER NOT NULL DEFAULT 0,
            is_active INTEGER NOT NULL DEFAULT 1,
            gstin TEXT,
            pan TEXT,
            address TEXT,
            state TEXT,
            pincode TEXT,
            unmapped_fields TEXT DEFAULT '{}',
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            CONSTRAINT uq_company_ledger_name UNIQUE (company_id, name)
        );
        )",
        "CREATE INDEX IF NOT EXISTS idx_ledgers_company_group ON ledgers(company_id, group_id);",

        // 5. Voucher Types
        R"(
        CREATE TABLE IF NOT EXISTS voucher_types (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            name TEXT NOT NULL,
            category TEXT NOT NULL,
            numbering_method TEXT NOT NULL DEFAULT 'AUTOMATIC',
            prefix TEXT,
            suffix TEXT,
            is_system INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL,
            CONSTRAINT uq_company_voucher_type UNIQUE (company_id, name)
        );
        )",

        // 6. Vouchers
        R"(
        CREATE TABLE IF NOT EXISTS vouchers (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            voucher_type_id TEXT NOT NULL REFERENCES voucher_types(id) ON DELETE RESTRICT,
            voucher_number TEXT NOT NULL,
            date TEXT NOT NULL,
            reference_number TEXT,
            reference_date TEXT,
            narration TEXT,
            total_amount TEXT NOT NULL,
            status TEXT NOT NULL DEFAULT 'POSTED',
            is_cancelled INTEGER NOT NULL DEFAULT 0,
            is_reversed INTEGER NOT NULL DEFAULT 0,
            reversal_voucher_id TEXT REFERENCES vouchers(id),
            unmapped_fields TEXT DEFAULT '{}',
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            CONSTRAINT uq_company_voucher_no UNIQUE (company_id, voucher_type_id, voucher_number)
        );
        )",
        "CREATE INDEX IF NOT EXISTS idx_vouchers_company_date ON vouchers(company_id, date);",

        // 7. Voucher Entries
        R"(
        CREATE TABLE IF NOT EXISTS voucher_entries (
            id TEXT PRIMARY KEY,
            voucher_id TEXT NOT NULL REFERENCES vouchers(id) ON DELETE CASCADE,
            ledger_id TEXT NOT NULL REFERENCES ledgers(id) ON DELETE RESTRICT,
            entry_type TEXT NOT NULL,
            amount TEXT NOT NULL,
            narration TEXT,
            line_index INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            CONSTRAINT uq_voucher_line UNIQUE (voucher_id, line_index)
        );
        )",
        "CREATE INDEX IF NOT EXISTS idx_entries_voucher ON voucher_entries(voucher_id);",
        "CREATE INDEX IF NOT EXISTS idx_entries_ledger ON voucher_entries(ledger_id);",

        // 8. Units of Measurement
        R"(
        CREATE TABLE IF NOT EXISTS units_of_measurement (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            name TEXT NOT NULL,
            formal_name TEXT,
            uqc_code TEXT,
            decimal_places INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL,
            CONSTRAINT uq_company_unit UNIQUE (company_id, name)
        );
        )",

        // 9. Inventory Items
        R"(
        CREATE TABLE IF NOT EXISTS inventory_items (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            unit_id TEXT NOT NULL REFERENCES units_of_measurement(id) ON DELETE RESTRICT,
            name TEXT NOT NULL,
            alias TEXT,
            hsn_code TEXT,
            gst_rate TEXT NOT NULL DEFAULT '0.00',
            opening_qty TEXT NOT NULL DEFAULT '0.0000',
            opening_rate TEXT NOT NULL DEFAULT '0.0000',
            opening_value TEXT NOT NULL DEFAULT '0.0000',
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            CONSTRAINT uq_company_item UNIQUE (company_id, name)
        );
        )",

        // 10. Stock Movements
        R"(
        CREATE TABLE IF NOT EXISTS stock_movements (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            voucher_id TEXT REFERENCES vouchers(id) ON DELETE CASCADE,
            item_id TEXT NOT NULL REFERENCES inventory_items(id) ON DELETE RESTRICT,
            movement_type TEXT NOT NULL,
            date TEXT NOT NULL,
            quantity TEXT NOT NULL,
            rate TEXT NOT NULL,
            amount TEXT NOT NULL,
            godown_name TEXT NOT NULL DEFAULT 'Main Location',
            created_at TEXT NOT NULL
        );
        )",
        "CREATE INDEX IF NOT EXISTS idx_stock_company_item ON stock_movements(company_id, item_id);",
        "CREATE INDEX IF NOT EXISTS idx_stock_date ON stock_movements(date);",

        // 11. External Identifiers
        R"(
        CREATE TABLE IF NOT EXISTS external_identifiers (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            source_system TEXT NOT NULL DEFAULT 'TALLY',
            entity_type TEXT NOT NULL,
            internal_id TEXT NOT NULL,
            external_guid TEXT,
            external_id TEXT,
            master_id TEXT,
            alter_id TEXT,
            fingerprint TEXT,
            last_synced_at TEXT NOT NULL,
            CONSTRAINT uq_source_external_entity UNIQUE (company_id, source_system, entity_type, external_guid)
        );
        )",
        "CREATE INDEX IF NOT EXISTS idx_ext_internal ON external_identifiers(internal_id);",

        // 12. Audit Events
        R"(
        CREATE TABLE IF NOT EXISTS audit_events (
            id TEXT PRIMARY KEY,
            company_id TEXT NOT NULL REFERENCES companies(id) ON DELETE CASCADE,
            entity_type TEXT NOT NULL,
            entity_id TEXT NOT NULL,
            action TEXT NOT NULL,
            performed_by TEXT NOT NULL DEFAULT 'SYSTEM',
            changes_json TEXT,
            performed_at TEXT NOT NULL
        );
        )",
        "CREATE INDEX IF NOT EXISTS idx_audit_company_date ON audit_events(company_id, performed_at);"
    };

    QSqlQuery query(db);
    for (const QString& ddl : ddlStatements) {
        if (!query.exec(ddl.trimmed())) {
            errorOut = "DDL execution failed: " + query.lastError().text().toStdString() + "\nSQL: " + ddl.toStdString();
            tally::utilities::Logger::error(errorOut);
            return false;
        }
    }

    tally::utilities::Logger::info("Database schema initialized and migrations successfully applied.");
    return true;
}

} // namespace tally::database
