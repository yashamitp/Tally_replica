#include "LedgerService.hpp"
#include <QUuid>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <map>
#include <set>
#include "../utilities/DateUtils.hpp"
#include "../utilities/Logger.hpp"

namespace tally::services {

LedgerService::LedgerService(database::DatabaseManager& dbManager)
    : m_dbManager(dbManager) {}

domain::Ledger LedgerService::createLedger(const CreateLedgerParams& params) {
    if (params.name.empty()) {
        throw std::invalid_argument("Ledger name is required");
    }

    // Verify parent group exists
    QSqlQuery grpQuery(m_dbManager.database());
    grpQuery.prepare("SELECT id FROM accounting_groups WHERE id = :gid AND company_id = :cid");
    grpQuery.bindValue(":gid", QString::fromStdString(params.groupId));
    grpQuery.bindValue(":cid", QString::fromStdString(params.companyId));
    if (!grpQuery.exec() || !grpQuery.next()) {
        throw std::invalid_argument("Accounting group does not exist in this company");
    }

    // Verify name uniqueness within company
    QSqlQuery existQuery(m_dbManager.database());
    existQuery.prepare("SELECT id FROM ledgers WHERE company_id = :cid AND name = :name");
    existQuery.bindValue(":cid", QString::fromStdString(params.companyId));
    existQuery.bindValue(":name", QString::fromStdString(params.name));
    if (existQuery.exec() && existQuery.next()) {
        throw std::invalid_argument("Ledger with name '" + params.name + "' already exists in this company");
    }

    std::string ledgerId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    std::string now = utilities::DateUtils::currentIsoDateTime();

    QSqlQuery insertQuery(m_dbManager.database());
    insertQuery.prepare(R"(
        INSERT INTO ledgers (
            id, company_id, group_id, name, alias, opening_balance, opening_balance_type,
            current_balance, is_system, is_active, gstin, pan, address, state, pincode,
            unmapped_fields, created_at, updated_at
        ) VALUES (
            :id, :company_id, :group_id, :name, :alias, :opening_balance, :opening_balance_type,
            :current_balance, :is_system, 1, :gstin, :pan, :address, :state, :pincode,
            :unmapped_fields, :created_at, :updated_at
        )
    )");

    insertQuery.bindValue(":id", QString::fromStdString(ledgerId));
    insertQuery.bindValue(":company_id", QString::fromStdString(params.companyId));
    insertQuery.bindValue(":group_id", QString::fromStdString(params.groupId));
    insertQuery.bindValue(":name", QString::fromStdString(params.name));
    insertQuery.bindValue(":alias", params.alias ? QString::fromStdString(*params.alias) : QVariant(QMetaType(QMetaType::QString)));
    insertQuery.bindValue(":opening_balance", QString::fromStdString(params.openingBalance.toDbString()));
    insertQuery.bindValue(":opening_balance_type", QString::fromStdString(domain::balanceTypeToString(params.openingBalanceType)));
    insertQuery.bindValue(":current_balance", QString::fromStdString(params.openingBalance.toDbString()));
    insertQuery.bindValue(":is_system", params.isSystem ? 1 : 0);
    insertQuery.bindValue(":gstin", params.gstin ? QString::fromStdString(*params.gstin) : QVariant(QMetaType(QMetaType::QString)));
    insertQuery.bindValue(":pan", params.pan ? QString::fromStdString(*params.pan) : QVariant(QMetaType(QMetaType::QString)));
    insertQuery.bindValue(":address", params.address ? QString::fromStdString(*params.address) : QVariant(QMetaType(QMetaType::QString)));
    insertQuery.bindValue(":state", params.state ? QString::fromStdString(*params.state) : QVariant(QMetaType(QMetaType::QString)));
    insertQuery.bindValue(":pincode", params.pincode ? QString::fromStdString(*params.pincode) : QVariant(QMetaType(QMetaType::QString)));
    insertQuery.bindValue(":unmapped_fields", QString::fromStdString(params.unmappedFields));
    insertQuery.bindValue(":created_at", QString::fromStdString(now));
    insertQuery.bindValue(":updated_at", QString::fromStdString(now));

    if (!insertQuery.exec()) {
        throw std::runtime_error("Failed to insert ledger: " + insertQuery.lastError().text().toStdString());
    }

    domain::Ledger ledger;
    ledger.id = ledgerId;
    ledger.companyId = params.companyId;
    ledger.groupId = params.groupId;
    ledger.name = params.name;
    ledger.alias = params.alias;
    ledger.openingBalance = params.openingBalance;
    ledger.openingBalanceType = params.openingBalanceType;
    ledger.currentBalance = params.openingBalance;
    ledger.isSystem = params.isSystem;
    ledger.isActive = true;
    ledger.gstin = params.gstin;
    ledger.pan = params.pan;
    ledger.address = params.address;
    ledger.state = params.state;
    ledger.pincode = params.pincode;
    ledger.unmappedFields = params.unmappedFields;
    ledger.createdAt = now;
    ledger.updatedAt = now;

    utilities::Logger::info("Created Ledger: " + ledger.name + " (" + ledger.id + ")");
    return ledger;
}

std::optional<domain::Ledger> LedgerService::getLedger(const std::string& id) {
    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT id, company_id, group_id, name, alias, opening_balance, opening_balance_type, current_balance, is_system, is_active, gstin, pan, address, state, pincode, unmapped_fields, created_at, updated_at FROM ledgers WHERE id = :id");
    query.bindValue(":id", QString::fromStdString(id));

    if (query.exec() && query.next()) {
        domain::Ledger l;
        l.id = query.value(0).toString().toStdString();
        l.companyId = query.value(1).toString().toStdString();
        l.groupId = query.value(2).toString().toStdString();
        l.name = query.value(3).toString().toStdString();
        if (!query.value(4).isNull()) l.alias = query.value(4).toString().toStdString();
        l.openingBalance = domain::ExactDecimal::fromString(query.value(5).toString().toStdString());
        l.openingBalanceType = domain::balanceTypeFromString(query.value(6).toString().toStdString());
        l.currentBalance = domain::ExactDecimal::fromString(query.value(7).toString().toStdString());
        l.isSystem = query.value(8).toInt() != 0;
        l.isActive = query.value(9).toInt() != 0;
        if (!query.value(10).isNull()) l.gstin = query.value(10).toString().toStdString();
        if (!query.value(11).isNull()) l.pan = query.value(11).toString().toStdString();
        if (!query.value(12).isNull()) l.address = query.value(12).toString().toStdString();
        if (!query.value(13).isNull()) l.state = query.value(13).toString().toStdString();
        if (!query.value(14).isNull()) l.pincode = query.value(14).toString().toStdString();
        l.unmappedFields = query.value(15).toString().toStdString();
        l.createdAt = query.value(16).toString().toStdString();
        l.updatedAt = query.value(17).toString().toStdString();
        return l;
    }
    return std::nullopt;
}

std::optional<domain::Ledger> LedgerService::getLedgerByName(const std::string& companyId, const std::string& name) {
    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT id, company_id, group_id, name, alias, opening_balance, opening_balance_type, current_balance, is_system, is_active, gstin, pan, address, state, pincode, unmapped_fields, created_at, updated_at FROM ledgers WHERE company_id = :cid AND name = :name");
    query.bindValue(":cid", QString::fromStdString(companyId));
    query.bindValue(":name", QString::fromStdString(name));

    if (query.exec() && query.next()) {
        domain::Ledger l;
        l.id = query.value(0).toString().toStdString();
        l.companyId = query.value(1).toString().toStdString();
        l.groupId = query.value(2).toString().toStdString();
        l.name = query.value(3).toString().toStdString();
        if (!query.value(4).isNull()) l.alias = query.value(4).toString().toStdString();
        l.openingBalance = domain::ExactDecimal::fromString(query.value(5).toString().toStdString());
        l.openingBalanceType = domain::balanceTypeFromString(query.value(6).toString().toStdString());
        l.currentBalance = domain::ExactDecimal::fromString(query.value(7).toString().toStdString());
        l.isSystem = query.value(8).toInt() != 0;
        l.isActive = query.value(9).toInt() != 0;
        if (!query.value(10).isNull()) l.gstin = query.value(10).toString().toStdString();
        if (!query.value(11).isNull()) l.pan = query.value(11).toString().toStdString();
        if (!query.value(12).isNull()) l.address = query.value(12).toString().toStdString();
        if (!query.value(13).isNull()) l.state = query.value(13).toString().toStdString();
        if (!query.value(14).isNull()) l.pincode = query.value(14).toString().toStdString();
        l.unmappedFields = query.value(15).toString().toStdString();
        l.createdAt = query.value(16).toString().toStdString();
        l.updatedAt = query.value(17).toString().toStdString();
        return l;
    }
    return std::nullopt;
}

std::vector<domain::Ledger> LedgerService::listLedgers(const std::string& companyId) {
    std::vector<domain::Ledger> result;
    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT id, company_id, group_id, name, alias, opening_balance, opening_balance_type, current_balance, is_system, is_active, gstin, pan, address, state, pincode, unmapped_fields, created_at, updated_at FROM ledgers WHERE company_id = :cid ORDER BY name");
    query.bindValue(":cid", QString::fromStdString(companyId));

    if (query.exec()) {
        while (query.next()) {
            domain::Ledger l;
            l.id = query.value(0).toString().toStdString();
            l.companyId = query.value(1).toString().toStdString();
            l.groupId = query.value(2).toString().toStdString();
            l.name = query.value(3).toString().toStdString();
            if (!query.value(4).isNull()) l.alias = query.value(4).toString().toStdString();
            l.openingBalance = domain::ExactDecimal::fromString(query.value(5).toString().toStdString());
            l.openingBalanceType = domain::balanceTypeFromString(query.value(6).toString().toStdString());
            l.currentBalance = domain::ExactDecimal::fromString(query.value(7).toString().toStdString());
            l.isSystem = query.value(8).toInt() != 0;
            l.isActive = query.value(9).toInt() != 0;
            if (!query.value(10).isNull()) l.gstin = query.value(10).toString().toStdString();
            if (!query.value(11).isNull()) l.pan = query.value(11).toString().toStdString();
            if (!query.value(12).isNull()) l.address = query.value(12).toString().toStdString();
            if (!query.value(13).isNull()) l.state = query.value(13).toString().toStdString();
            if (!query.value(14).isNull()) l.pincode = query.value(14).toString().toStdString();
            l.unmappedFields = query.value(15).toString().toStdString();
            l.createdAt = query.value(16).toString().toStdString();
            l.updatedAt = query.value(17).toString().toStdString();
            result.push_back(l);
        }
    }
    return result;
}

bool LedgerService::alterLedger(const domain::Ledger& ledger) {
    QSqlQuery query(m_dbManager.database());
    query.prepare(R"(
        UPDATE ledgers SET
            name = :name,
            group_id = :group_id,
            alias = :alias,
            opening_balance = :opening_balance,
            opening_balance_type = :opening_balance_type,
            current_balance = :current_balance,
            gstin = :gstin,
            pan = :pan,
            address = :address,
            state = :state,
            pincode = :pincode,
            updated_at = :updated_at
        WHERE id = :id AND company_id = :company_id
    )");

    std::string now = utilities::DateUtils::currentIsoDateTime();
    query.bindValue(":id", QString::fromStdString(ledger.id));
    query.bindValue(":company_id", QString::fromStdString(ledger.companyId));
    query.bindValue(":name", QString::fromStdString(ledger.name));
    query.bindValue(":group_id", QString::fromStdString(ledger.groupId));
    query.bindValue(":alias", ledger.alias ? QString::fromStdString(*ledger.alias) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":opening_balance", QString::fromStdString(ledger.openingBalance.toDbString()));
    query.bindValue(":opening_balance_type", QString::fromStdString(domain::balanceTypeToString(ledger.openingBalanceType)));
    query.bindValue(":current_balance", QString::fromStdString(ledger.currentBalance.toDbString()));
    query.bindValue(":gstin", ledger.gstin ? QString::fromStdString(*ledger.gstin) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":pan", ledger.pan ? QString::fromStdString(*ledger.pan) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":address", ledger.address ? QString::fromStdString(*ledger.address) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":state", ledger.state ? QString::fromStdString(*ledger.state) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":pincode", ledger.pincode ? QString::fromStdString(*ledger.pincode) : QVariant(QMetaType(QMetaType::QString)));
    query.bindValue(":updated_at", QString::fromStdString(now));

    return query.exec();
}

LedgerStatement LedgerService::getLedgerStatement(
    const std::string& companyId,
    const std::string& ledgerId,
    const std::optional<std::string>& fromDate,
    const std::optional<std::string>& toDate
) {
    // 1. Fetch Ledger & Parent Group
    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare(R"(
        SELECT l.name, l.opening_balance, l.opening_balance_type, g.name, g.nature
        FROM ledgers l
        JOIN accounting_groups g ON l.group_id = g.id
        WHERE l.id = :lid AND l.company_id = :cid
    )");
    lQuery.bindValue(":lid", QString::fromStdString(ledgerId));
    lQuery.bindValue(":cid", QString::fromStdString(companyId));

    if (!lQuery.exec() || !lQuery.next()) {
        throw std::invalid_argument("Ledger not found in company");
    }

    std::string ledgerName = lQuery.value(0).toString().toStdString();
    domain::ExactDecimal initialOpening = domain::ExactDecimal::fromString(lQuery.value(1).toString().toStdString());
    domain::BalanceType initialSide = domain::balanceTypeFromString(lQuery.value(2).toString().toStdString());
    std::string groupName = lQuery.value(3).toString().toStdString();
    domain::GroupNature groupNature = domain::groupNatureFromString(lQuery.value(4).toString().toStdString());

    // Net signed convention: DR is positive, CR is negative
    domain::ExactDecimal netSigned = (initialSide == domain::BalanceType::Dr) ? initialOpening : initialOpening.neg();

    // 2. Pre-period movements (< fromDate)
    if (fromDate && !fromDate->empty()) {
        QSqlQuery preQuery(m_dbManager.database());
        preQuery.prepare(R"(
            SELECT ve.entry_type, ve.amount
            FROM voucher_entries ve
            JOIN vouchers v ON ve.voucher_id = v.id
            WHERE v.company_id = :cid AND ve.ledger_id = :lid
              AND v.is_cancelled = 0 AND v.date < :from_date
        )");
        preQuery.bindValue(":cid", QString::fromStdString(companyId));
        preQuery.bindValue(":lid", QString::fromStdString(ledgerId));
        preQuery.bindValue(":from_date", QString::fromStdString(*fromDate));

        if (preQuery.exec()) {
            while (preQuery.next()) {
                domain::BalanceType etype = domain::balanceTypeFromString(preQuery.value(0).toString().toStdString());
                domain::ExactDecimal amt = domain::ExactDecimal::fromString(preQuery.value(1).toString().toStdString());
                if (etype == domain::BalanceType::Dr) {
                    netSigned = netSigned.add(amt);
                } else {
                    netSigned = netSigned.sub(amt);
                }
            }
        }
    }

    domain::ExactDecimal statementOpening = netSigned.abs();
    domain::BalanceType statementOpeningType = netSigned.isNegative() ? domain::BalanceType::Cr : domain::BalanceType::Dr;

    // 3. Transactions in period
    QString qstr = R"(
        SELECT v.id, v.date, v.voucher_number, vt.name, v.narration, ve.entry_type, ve.amount, ve.narration, ve.line_index
        FROM voucher_entries ve
        JOIN vouchers v ON ve.voucher_id = v.id
        JOIN voucher_types vt ON v.voucher_type_id = vt.id
        WHERE v.company_id = :cid AND ve.ledger_id = :lid AND v.is_cancelled = 0
    )";

    if (fromDate && !fromDate->empty()) {
        qstr += " AND v.date >= :from_date";
    }
    if (toDate && !toDate->empty()) {
        qstr += " AND v.date <= :to_date";
    }
    qstr += " ORDER BY v.date ASC, v.created_at ASC, ve.line_index ASC";

    QSqlQuery tQuery(m_dbManager.database());
    tQuery.prepare(qstr);
    tQuery.bindValue(":cid", QString::fromStdString(companyId));
    tQuery.bindValue(":lid", QString::fromStdString(ledgerId));
    if (fromDate && !fromDate->empty()) tQuery.bindValue(":from_date", QString::fromStdString(*fromDate));
    if (toDate && !toDate->empty()) tQuery.bindValue(":to_date", QString::fromStdString(*toDate));

    struct RawPeriodRow {
        std::string voucherId;
        std::string date;
        std::string voucherNumber;
        std::string voucherTypeName;
        std::optional<std::string> vNarration;
        domain::BalanceType entryType;
        domain::ExactDecimal amount;
        std::optional<std::string> eNarration;
    };

    std::vector<RawPeriodRow> periodRows;
    std::set<std::string> voucherIds;

    if (tQuery.exec()) {
        while (tQuery.next()) {
            RawPeriodRow row;
            row.voucherId = tQuery.value(0).toString().toStdString();
            row.date = tQuery.value(1).toString().toStdString();
            row.voucherNumber = tQuery.value(2).toString().toStdString();
            row.voucherTypeName = tQuery.value(3).toString().toStdString();
            if (!tQuery.value(4).isNull()) row.vNarration = tQuery.value(4).toString().toStdString();
            row.entryType = domain::balanceTypeFromString(tQuery.value(5).toString().toStdString());
            row.amount = domain::ExactDecimal::fromString(tQuery.value(6).toString().toStdString());
            if (!tQuery.value(7).isNull()) row.eNarration = tQuery.value(7).toString().toStdString();

            voucherIds.insert(row.voucherId);
            periodRows.push_back(row);
        }
    }

    // 4. Opposing particulars map
    std::map<std::string, std::vector<std::string>> opposingMap;
    if (!voucherIds.empty()) {
        QSqlQuery oppQuery(m_dbManager.database());
        oppQuery.exec("SELECT ve.voucher_id, l.name FROM voucher_entries ve JOIN ledgers l ON ve.ledger_id = l.id WHERE ve.ledger_id != '" + QString::fromStdString(ledgerId) + "'");
        while (oppQuery.next()) {
            std::string vid = oppQuery.value(0).toString().toStdString();
            std::string lname = oppQuery.value(1).toString().toStdString();
            auto& list = opposingMap[vid];
            if (std::find(list.begin(), list.end(), lname) == list.end()) {
                list.push_back(lname);
            }
        }
    }

    // 5. Build statement lines & running balance
    domain::ExactDecimal runningNet = netSigned;
    domain::ExactDecimal totalDebits = domain::ExactDecimal::zero();
    domain::ExactDecimal totalCredits = domain::ExactDecimal::zero();
    std::vector<LedgerStatementLine> lines;

    for (const auto& r : periodRows) {
        if (r.entryType == domain::BalanceType::Dr) {
            totalDebits = totalDebits.add(r.amount);
            runningNet = runningNet.add(r.amount);
        } else {
            totalCredits = totalCredits.add(r.amount);
            runningNet = runningNet.sub(r.amount);
        }

        std::string particulars;
        auto oppIt = opposingMap.find(r.voucherId);
        if (oppIt != opposingMap.end() && !oppIt->second.empty()) {
            for (size_t i = 0; i < oppIt->second.size(); ++i) {
                if (i > 0) particulars += ", ";
                particulars += oppIt->second[i];
            }
        } else {
            particulars = "Self/Direct";
        }

        LedgerStatementLine line;
        line.date = r.date;
        line.voucherId = r.voucherId;
        line.voucherNumber = r.voucherNumber;
        line.voucherTypeName = r.voucherTypeName;
        line.particulars = particulars;
        line.narration = r.eNarration ? r.eNarration : r.vNarration;
        if (r.entryType == domain::BalanceType::Dr) {
            line.debitAmount = r.amount;
        } else {
            line.creditAmount = r.amount;
        }
        line.runningBalance = runningNet.abs();
        line.runningBalanceType = runningNet.isNegative() ? domain::BalanceType::Cr : domain::BalanceType::Dr;

        lines.push_back(line);
    }

    LedgerStatement stmt;
    stmt.companyId = companyId;
    stmt.ledgerId = ledgerId;
    stmt.ledgerName = ledgerName;
    stmt.groupName = groupName;
    stmt.groupNature = groupNature;
    stmt.fromDate = fromDate.value_or("Beginning");
    stmt.toDate = toDate.value_or("Present");
    stmt.openingBalance = statementOpening;
    stmt.openingBalanceType = statementOpeningType;
    stmt.totalDebits = totalDebits;
    stmt.totalCredits = totalCredits;
    stmt.closingBalance = runningNet.abs();
    stmt.closingBalanceType = runningNet.isNegative() ? domain::BalanceType::Cr : domain::BalanceType::Dr;
    stmt.transactions = std::move(lines);

    return stmt;
}

} // namespace tally::services
