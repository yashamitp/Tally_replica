#include "CompanyService.hpp"
#include <QUuid>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <map>
#include "../utilities/DateUtils.hpp"
#include "../utilities/Logger.hpp"

namespace tally::services {

CompanyService::CompanyService(database::DatabaseManager& dbManager)
    : m_dbManager(dbManager) {}

domain::Company CompanyService::createCompany(const CreateCompanyParams& params) {
    if (params.name.empty()) {
        throw std::invalid_argument("Company name is required");
    }

    m_dbManager.beginTransaction();

    std::string companyId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    std::string now = utilities::DateUtils::currentIsoDateTime();

    // 1. Insert Company
    QSqlQuery query(m_dbManager.database());
    query.prepare(R"(
        INSERT INTO companies (
            id, name, formal_name, address_line1, address_line2,
            city, state, country, pincode, gstin, pan,
            currency_symbol, currency_formal_name, decimal_places,
            is_active, created_at, updated_at
        ) VALUES (
            :id, :name, :formal_name, :address_line1, :address_line2,
            :city, :state, :country, :pincode, :gstin, :pan,
            :currency_symbol, :currency_formal_name, :decimal_places,
            1, :created_at, :updated_at
        )
    )");

    query.bindValue(":id", QString::fromStdString(companyId));
    query.bindValue(":name", QString::fromStdString(params.name));
    query.bindValue(":formal_name", params.formalName ? QString::fromStdString(*params.formalName) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":address_line1", params.addressLine1 ? QString::fromStdString(*params.addressLine1) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":address_line2", params.addressLine2 ? QString::fromStdString(*params.addressLine2) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":city", params.city ? QString::fromStdString(*params.city) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":state", params.state ? QString::fromStdString(*params.state) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":country", QString::fromStdString(params.country));
    query.bindValue(":pincode", params.pincode ? QString::fromStdString(*params.pincode) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":gstin", params.gstin ? QString::fromStdString(*params.gstin) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":pan", params.pan ? QString::fromStdString(*params.pan) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":currency_symbol", QString::fromStdString(params.currencySymbol));
    query.bindValue(":currency_formal_name", QString::fromStdString(params.currencyFormalName));
    query.bindValue(":decimal_places", params.decimalPlaces);
    query.bindValue(":created_at", QString::fromStdString(now));
    query.bindValue(":updated_at", QString::fromStdString(now));

    if (!query.exec()) {
        m_dbManager.rollbackTransaction();
        throw std::runtime_error("Failed to insert company: " + query.lastError().text().toStdString());
    }

    // 2. Insert Financial Year
    std::string fyId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    std::string endDate = utilities::DateUtils::addFinancialYear(params.financialYearStart);

    QSqlQuery fyQuery(m_dbManager.database());
    fyQuery.prepare(R"(
        INSERT INTO financial_years (
            id, company_id, start_date, end_date, books_beginning_from, is_closed, created_at
        ) VALUES (
            :id, :company_id, :start_date, :end_date, :books_beginning_from, 0, :created_at
        )
    )");
    fyQuery.bindValue(":id", QString::fromStdString(fyId));
    fyQuery.bindValue(":company_id", QString::fromStdString(companyId));
    fyQuery.bindValue(":start_date", QString::fromStdString(params.financialYearStart));
    fyQuery.bindValue(":end_date", QString::fromStdString(endDate));
    fyQuery.bindValue(":books_beginning_from", QString::fromStdString(params.booksBeginningFrom));
    fyQuery.bindValue(":created_at", QString::fromStdString(now));

    if (!fyQuery.exec()) {
        m_dbManager.rollbackTransaction();
        throw std::runtime_error("Failed to insert financial year: " + fyQuery.lastError().text().toStdString());
    }

    // 3. Seed Primary Accounting Groups (12 Groups)
    struct PrimaryGroupDef {
        std::string name;
        domain::GroupNature nature;
        bool affectsGrossProfit;
    };

    const std::vector<PrimaryGroupDef> primaryGroupDefs = {
        {"Capital Account", domain::GroupNature::Equity, false},
        {"Loans (Liability)", domain::GroupNature::Liability, false},
        {"Current Liabilities", domain::GroupNature::Liability, false},
        {"Fixed Assets", domain::GroupNature::Asset, false},
        {"Investments", domain::GroupNature::Asset, false},
        {"Current Assets", domain::GroupNature::Asset, false},
        {"Sales Accounts", domain::GroupNature::Income, true},
        {"Purchase Accounts", domain::GroupNature::Expense, true},
        {"Direct Incomes", domain::GroupNature::Income, true},
        {"Direct Expenses", domain::GroupNature::Expense, true},
        {"Indirect Incomes", domain::GroupNature::Income, false},
        {"Indirect Expenses", domain::GroupNature::Expense, false}
    };

    std::map<std::string, std::string> groupMap;

    for (const auto& grp : primaryGroupDefs) {
        std::string gid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        QSqlQuery grpQuery(m_dbManager.database());
        grpQuery.prepare(R"(
            INSERT INTO accounting_groups (
                id, company_id, parent_id, name, nature, affects_gross_profit, is_primary, is_system, created_at, updated_at
            ) VALUES (
                :id, :company_id, NULL, :name, :nature, :affects_gross_profit, 1, 1, :created_at, :updated_at
            )
        )");
        grpQuery.bindValue(":id", QString::fromStdString(gid));
        grpQuery.bindValue(":company_id", QString::fromStdString(companyId));
        grpQuery.bindValue(":name", QString::fromStdString(grp.name));
        grpQuery.bindValue(":nature", QString::fromStdString(domain::groupNatureToString(grp.nature)));
        grpQuery.bindValue(":affects_gross_profit", grp.affectsGrossProfit ? 1 : 0);
        grpQuery.bindValue(":created_at", QString::fromStdString(now));
        grpQuery.bindValue(":updated_at", QString::fromStdString(now));

        if (!grpQuery.exec()) {
            m_dbManager.rollbackTransaction();
            throw std::runtime_error("Failed to seed primary group " + grp.name + ": " + grpQuery.lastError().text().toStdString());
        }
        groupMap[grp.name] = gid;
    }

    // 4. Seed Standard Sub-groups (8 Groups)
    struct SubGroupDef {
        std::string name;
        std::string parent;
        domain::GroupNature nature;
    };

    const std::vector<SubGroupDef> subGroupDefs = {
        {"Bank Accounts", "Current Assets", domain::GroupNature::Asset},
        {"Cash-in-Hand", "Current Assets", domain::GroupNature::Asset},
        {"Sundry Debtors", "Current Assets", domain::GroupNature::Asset},
        {"Sundry Creditors", "Current Liabilities", domain::GroupNature::Liability},
        {"Duties & Taxes", "Current Liabilities", domain::GroupNature::Liability},
        {"Bank OCC A/c", "Loans (Liability)", domain::GroupNature::Liability},
        {"Secured Loans", "Loans (Liability)", domain::GroupNature::Liability},
        {"Unsecured Loans", "Loans (Liability)", domain::GroupNature::Liability}
    };

    for (const auto& sub : subGroupDefs) {
        std::string gid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        std::string parentId = groupMap[sub.parent];

        QSqlQuery subQuery(m_dbManager.database());
        subQuery.prepare(R"(
            INSERT INTO accounting_groups (
                id, company_id, parent_id, name, nature, affects_gross_profit, is_primary, is_system, created_at, updated_at
            ) VALUES (
                :id, :company_id, :parent_id, :name, :nature, 0, 0, 1, :created_at, :updated_at
            )
        )");
        subQuery.bindValue(":id", QString::fromStdString(gid));
        subQuery.bindValue(":company_id", QString::fromStdString(companyId));
        subQuery.bindValue(":parent_id", QString::fromStdString(parentId));
        subQuery.bindValue(":name", QString::fromStdString(sub.name));
        subQuery.bindValue(":nature", QString::fromStdString(domain::groupNatureToString(sub.nature)));
        subQuery.bindValue(":created_at", QString::fromStdString(now));
        subQuery.bindValue(":updated_at", QString::fromStdString(now));

        if (!subQuery.exec()) {
            m_dbManager.rollbackTransaction();
            throw std::runtime_error("Failed to seed subgroup " + sub.name + ": " + subQuery.lastError().text().toStdString());
        }
        groupMap[sub.name] = gid;
    }

    // 5. Seed Standard Voucher Types (6 Types)
    struct VTypeDef {
        std::string name;
        domain::VoucherCategory category;
    };

    const std::vector<VTypeDef> vtypeDefs = {
        {"Journal", domain::VoucherCategory::Journal},
        {"Payment", domain::VoucherCategory::Payment},
        {"Receipt", domain::VoucherCategory::Receipt},
        {"Contra", domain::VoucherCategory::Contra},
        {"Sales", domain::VoucherCategory::Sales},
        {"Purchase", domain::VoucherCategory::Purchase}
    };

    for (const auto& vt : vtypeDefs) {
        std::string vid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        QSqlQuery vtQuery(m_dbManager.database());
        vtQuery.prepare(R"(
            INSERT INTO voucher_types (
                id, company_id, name, category, numbering_method, is_system, created_at
            ) VALUES (
                :id, :company_id, :name, :category, 'AUTOMATIC', 1, :created_at
            )
        )");
        vtQuery.bindValue(":id", QString::fromStdString(vid));
        vtQuery.bindValue(":company_id", QString::fromStdString(companyId));
        vtQuery.bindValue(":name", QString::fromStdString(vt.name));
        vtQuery.bindValue(":category", QString::fromStdString(domain::voucherCategoryToString(vt.category)));
        vtQuery.bindValue(":created_at", QString::fromStdString(now));

        if (!vtQuery.exec()) {
            m_dbManager.rollbackTransaction();
            throw std::runtime_error("Failed to seed voucher type " + vt.name + ": " + vtQuery.lastError().text().toStdString());
        }
    }

    // 6. Seed Default Cash Ledger
    auto cashGroupIt = groupMap.find("Cash-in-Hand");
    if (cashGroupIt != groupMap.end()) {
        std::string cashLedgerId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        QSqlQuery cashQuery(m_dbManager.database());
        cashQuery.prepare(R"(
            INSERT INTO ledgers (
                id, company_id, group_id, name, opening_balance, opening_balance_type,
                current_balance, is_system, is_active, created_at, updated_at
            ) VALUES (
                :id, :company_id, :group_id, 'Cash', '0.0000', 'DR',
                '0.0000', 1, 1, :created_at, :updated_at
            )
        )");
        cashQuery.bindValue(":id", QString::fromStdString(cashLedgerId));
        cashQuery.bindValue(":company_id", QString::fromStdString(companyId));
        cashQuery.bindValue(":group_id", QString::fromStdString(cashGroupIt->second));
        cashQuery.bindValue(":created_at", QString::fromStdString(now));
        cashQuery.bindValue(":updated_at", QString::fromStdString(now));

        if (!cashQuery.exec()) {
            m_dbManager.rollbackTransaction();
            throw std::runtime_error("Failed to seed default Cash ledger: " + cashQuery.lastError().text().toStdString());
        }
    }

    // 7. Record Audit Event
    std::string auditId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    QSqlQuery auditQuery(m_dbManager.database());
    auditQuery.prepare(R"(
        INSERT INTO audit_events (
            id, company_id, entity_type, entity_id, action, performed_by, changes_json, performed_at
        ) VALUES (
            :id, :company_id, 'COMPANY', :entity_id, 'CREATE', 'SYSTEM', :changes_json, :performed_at
        )
    )");
    auditQuery.bindValue(":id", QString::fromStdString(auditId));
    auditQuery.bindValue(":company_id", QString::fromStdString(companyId));
    auditQuery.bindValue(":entity_id", QString::fromStdString(companyId));
    auditQuery.bindValue(":changes_json", QString("{\"name\":\"%1\"}").arg(QString::fromStdString(params.name)));
    auditQuery.bindValue(":performed_at", QString::fromStdString(now));
    auditQuery.exec();

    m_dbManager.commitTransaction();

    domain::Company comp;
    comp.id = companyId;
    comp.name = params.name;
    comp.formalName = params.formalName;
    comp.addressLine1 = params.addressLine1;
    comp.addressLine2 = params.addressLine2;
    comp.city = params.city;
    comp.state = params.state;
    comp.country = params.country;
    comp.pincode = params.pincode;
    comp.gstin = params.gstin;
    comp.pan = params.pan;
    comp.currencySymbol = params.currencySymbol;
    comp.currencyFormalName = params.currencyFormalName;
    comp.decimalPlaces = params.decimalPlaces;
    comp.isActive = true;
    comp.createdAt = now;
    comp.updatedAt = now;

    utilities::Logger::info("Created and initialized Company: " + comp.name + " (" + comp.id + ")");
    return comp;
}

std::vector<domain::Company> CompanyService::listCompanies() {
    std::vector<domain::Company> result;
    QSqlQuery query(m_dbManager.database());
    query.exec("SELECT id, name, formal_name, address_line1, address_line2, city, state, country, pincode, gstin, pan, currency_symbol, currency_formal_name, decimal_places, is_active, created_at, updated_at FROM companies ORDER BY name");

    while (query.next()) {
        domain::Company c;
        c.id = query.value(0).toString().toStdString();
        c.name = query.value(1).toString().toStdString();
        if (!query.value(2).isNull()) c.formalName = query.value(2).toString().toStdString();
        if (!query.value(3).isNull()) c.addressLine1 = query.value(3).toString().toStdString();
        if (!query.value(4).isNull()) c.addressLine2 = query.value(4).toString().toStdString();
        if (!query.value(5).isNull()) c.city = query.value(5).toString().toStdString();
        if (!query.value(6).isNull()) c.state = query.value(6).toString().toStdString();
        c.country = query.value(7).toString().toStdString();
        if (!query.value(8).isNull()) c.pincode = query.value(8).toString().toStdString();
        if (!query.value(9).isNull()) c.gstin = query.value(9).toString().toStdString();
        if (!query.value(10).isNull()) c.pan = query.value(10).toString().toStdString();
        c.currencySymbol = query.value(11).toString().toStdString();
        c.currencyFormalName = query.value(12).toString().toStdString();
        c.decimalPlaces = query.value(13).toInt();
        c.isActive = query.value(14).toInt() != 0;
        c.createdAt = query.value(15).toString().toStdString();
        c.updatedAt = query.value(16).toString().toStdString();
        result.push_back(c);
    }
    return result;
}

std::optional<domain::Company> CompanyService::getCompany(const std::string& id) {
    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT id, name, formal_name, address_line1, address_line2, city, state, country, pincode, gstin, pan, currency_symbol, currency_formal_name, decimal_places, is_active, created_at, updated_at FROM companies WHERE id = :id");
    query.bindValue(":id", QString::fromStdString(id));

    if (query.exec() && query.next()) {
        domain::Company c;
        c.id = query.value(0).toString().toStdString();
        c.name = query.value(1).toString().toStdString();
        if (!query.value(2).isNull()) c.formalName = query.value(2).toString().toStdString();
        if (!query.value(3).isNull()) c.addressLine1 = query.value(3).toString().toStdString();
        if (!query.value(4).isNull()) c.addressLine2 = query.value(4).toString().toStdString();
        if (!query.value(5).isNull()) c.city = query.value(5).toString().toStdString();
        if (!query.value(6).isNull()) c.state = query.value(6).toString().toStdString();
        c.country = query.value(7).toString().toStdString();
        if (!query.value(8).isNull()) c.pincode = query.value(8).toString().toStdString();
        if (!query.value(9).isNull()) c.gstin = query.value(9).toString().toStdString();
        if (!query.value(10).isNull()) c.pan = query.value(10).toString().toStdString();
        c.currencySymbol = query.value(11).toString().toStdString();
        c.currencyFormalName = query.value(12).toString().toStdString();
        c.decimalPlaces = query.value(13).toInt();
        c.isActive = query.value(14).toInt() != 0;
        c.createdAt = query.value(15).toString().toStdString();
        c.updatedAt = query.value(16).toString().toStdString();
        return c;
    }
    return std::nullopt;
}

std::vector<domain::AccountingGroup> CompanyService::listGroups(const std::string& companyId) {
    std::vector<domain::AccountingGroup> result;
    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT id, company_id, parent_id, name, nature, affects_gross_profit, is_primary, is_system, created_at, updated_at FROM accounting_groups WHERE company_id = :cid ORDER BY name");
    query.bindValue(":cid", QString::fromStdString(companyId));

    if (query.exec()) {
        while (query.next()) {
            domain::AccountingGroup g;
            g.id = query.value(0).toString().toStdString();
            g.companyId = query.value(1).toString().toStdString();
            if (!query.value(2).isNull()) g.parentId = query.value(2).toString().toStdString();
            g.name = query.value(3).toString().toStdString();
            g.nature = domain::groupNatureFromString(query.value(4).toString().toStdString());
            g.affectsGrossProfit = query.value(5).toInt() != 0;
            g.isPrimary = query.value(6).toInt() != 0;
            g.isSystem = query.value(7).toInt() != 0;
            g.createdAt = query.value(8).toString().toStdString();
            g.updatedAt = query.value(9).toString().toStdString();
            result.push_back(g);
        }
    }
    return result;
}

std::vector<domain::VoucherType> CompanyService::listVoucherTypes(const std::string& companyId) {
    std::vector<domain::VoucherType> result;
    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT id, company_id, name, category, numbering_method, prefix, suffix, is_system, created_at FROM voucher_types WHERE company_id = :cid ORDER BY name");
    query.bindValue(":cid", QString::fromStdString(companyId));

    if (query.exec()) {
        while (query.next()) {
            domain::VoucherType vt;
            vt.id = query.value(0).toString().toStdString();
            vt.companyId = query.value(1).toString().toStdString();
            vt.name = query.value(2).toString().toStdString();
            vt.category = domain::voucherCategoryFromString(query.value(3).toString().toStdString());
            vt.numberingMethod = query.value(4).toString().toStdString();
            if (!query.value(5).isNull()) vt.prefix = query.value(5).toString().toStdString();
            if (!query.value(6).isNull()) vt.suffix = query.value(6).toString().toStdString();
            vt.isSystem = query.value(7).toInt() != 0;
            vt.createdAt = query.value(8).toString().toStdString();
            result.push_back(vt);
        }
    }
    return result;
}

} // namespace tally::services
