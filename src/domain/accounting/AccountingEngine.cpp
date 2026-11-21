#include "AccountingEngine.hpp"
#include <QUuid>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <map>
#include <set>
#include "../../utilities/DateUtils.hpp"
#include "../../utilities/Logger.hpp"

namespace tally::domain {

AccountingEngine::AccountingEngine(database::DatabaseManager& dbManager)
    : m_dbManager(dbManager) {}

Voucher AccountingEngine::postVoucher(Voucher voucher) {
    // 1. Strict mathematical double-entry balance check
    DoubleEntryValidationResult balanceCheck = DoubleEntryValidator::validate(voucher.entries);
    if (!balanceCheck.isBalanced) {
        throw std::invalid_argument(balanceCheck.errorMessage);
    }

    voucher.totalAmount = balanceCheck.totalDebit;

    // 2. Execute inside an ACID database transaction
    m_dbManager.beginTransaction();

    // Verify Company exists and is active
    QSqlQuery compQuery(m_dbManager.database());
    compQuery.prepare("SELECT is_active FROM companies WHERE id = :cid");
    compQuery.bindValue(":cid", QString::fromStdString(voucher.companyId));
    if (!compQuery.exec() || !compQuery.next()) {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Company not found: " + voucher.companyId);
    }
    if (compQuery.value(0).toInt() == 0) {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Company is inactive");
    }

    // Verify VoucherType exists and belongs to company
    QSqlQuery vtQuery(m_dbManager.database());
    vtQuery.prepare("SELECT name FROM voucher_types WHERE id = :vtid AND company_id = :cid");
    vtQuery.bindValue(":vtid", QString::fromStdString(voucher.voucherTypeId));
    vtQuery.bindValue(":cid", QString::fromStdString(voucher.companyId));
    if (!vtQuery.exec() || !vtQuery.next()) {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Voucher type does not exist in this company");
    }
    std::string vTypeName = vtQuery.value(0).toString().toStdString();

    // Check for duplicate voucher number in this company & voucher type
    QSqlQuery dupQuery(m_dbManager.database());
    dupQuery.prepare("SELECT id FROM vouchers WHERE company_id = :cid AND voucher_type_id = :vtid AND voucher_number = :vno");
    dupQuery.bindValue(":cid", QString::fromStdString(voucher.companyId));
    dupQuery.bindValue(":vtid", QString::fromStdString(voucher.voucherTypeId));
    dupQuery.bindValue(":vno", QString::fromStdString(voucher.voucherNumber));
    if (dupQuery.exec() && dupQuery.next()) {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Voucher number '" + voucher.voucherNumber + "' already exists for voucher type '" + vTypeName + "' in this company");
    }

    // Fetch all referenced ledgers along with group nature
    std::set<std::string> ledgerIds;
    for (const auto& e : voucher.entries) {
        ledgerIds.insert(e.ledgerId);
    }

    struct LedgerInfo {
        std::string id;
        std::string name;
        ExactDecimal currentBalance;
        GroupNature nature;
    };
    std::map<std::string, LedgerInfo> ledgerMap;

    for (const auto& lid : ledgerIds) {
        QSqlQuery lQuery(m_dbManager.database());
        lQuery.prepare("SELECT l.id, l.name, l.current_balance, g.nature FROM ledgers l JOIN accounting_groups g ON l.group_id = g.id WHERE l.id = :lid AND l.company_id = :cid");
        lQuery.bindValue(":lid", QString::fromStdString(lid));
        lQuery.bindValue(":cid", QString::fromStdString(voucher.companyId));

        if (!lQuery.exec() || !lQuery.next()) {
            m_dbManager.rollbackTransaction();
            throw std::invalid_argument("One or more ledger accounts do not exist or belong to a different company");
        }

        LedgerInfo info;
        info.id = lQuery.value(0).toString().toStdString();
        info.name = lQuery.value(1).toString().toStdString();
        info.currentBalance = ExactDecimal::fromString(lQuery.value(2).toString().toStdString());
        info.nature = groupNatureFromString(lQuery.value(3).toString().toStdString());
        ledgerMap[info.id] = info;
    }

    // Insert Voucher Header
    std::string voucherId = voucher.id.empty() ? QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString() : voucher.id;
    std::string now = utilities::DateUtils::currentIsoDateTime();

    QSqlQuery insertVch(m_dbManager.database());
    insertVch.prepare(R"(
        INSERT INTO vouchers (
            id, company_id, voucher_type_id, voucher_number, date, reference_number,
            reference_date, narration, total_amount, status, is_cancelled, is_reversed,
            reversal_voucher_id, unmapped_fields, created_at, updated_at
        ) VALUES (
            :id, :company_id, :voucher_type_id, :voucher_number, :date, :reference_number,
            :reference_date, :narration, :total_amount, 'POSTED', 0, 0,
            NULL, :unmapped_fields, :created_at, :updated_at
        )
    )");

    insertVch.bindValue(":id", QString::fromStdString(voucherId));
    insertVch.bindValue(":company_id", QString::fromStdString(voucher.companyId));
    insertVch.bindValue(":voucher_type_id", QString::fromStdString(voucher.voucherTypeId));
    insertVch.bindValue(":voucher_number", QString::fromStdString(voucher.voucherNumber));
    insertVch.bindValue(":date", QString::fromStdString(voucher.date));
    insertVch.bindValue(":reference_number", voucher.referenceNumber ? QString::fromStdString(*voucher.referenceNumber) : QVariant(QMetaType(QMetaType::QString)));
    insertVch.bindValue(":reference_date", voucher.referenceDate ? QString::fromStdString(*voucher.referenceDate) : QVariant(QMetaType(QMetaType::QString)));
    insertVch.bindValue(":narration", voucher.narration ? QString::fromStdString(*voucher.narration) : QVariant(QMetaType(QMetaType::QString)));
    insertVch.bindValue(":total_amount", QString::fromStdString(voucher.totalAmount.toDbString()));
    insertVch.bindValue(":unmapped_fields", QString::fromStdString(voucher.unmappedFields));
    insertVch.bindValue(":created_at", QString::fromStdString(now));
    insertVch.bindValue(":updated_at", QString::fromStdString(now));

    if (!insertVch.exec()) {
        m_dbManager.rollbackTransaction();
        throw std::runtime_error("Failed to insert voucher header: " + insertVch.lastError().text().toStdString());
    }

    // Insert line items & update running ledger balances
    std::vector<VoucherEntry> insertedEntries;
    int lineIdx = 1;

    for (auto& entry : voucher.entries) {
        std::string entryId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        QSqlQuery insertEntry(m_dbManager.database());
        insertEntry.prepare(R"(
            INSERT INTO voucher_entries (
                id, voucher_id, ledger_id, entry_type, amount, narration, line_index, created_at
            ) VALUES (
                :id, :voucher_id, :ledger_id, :entry_type, :amount, :narration, :line_index, :created_at
            )
        )");

        insertEntry.bindValue(":id", QString::fromStdString(entryId));
        insertEntry.bindValue(":voucher_id", QString::fromStdString(voucherId));
        insertEntry.bindValue(":ledger_id", QString::fromStdString(entry.ledgerId));
        insertEntry.bindValue(":entry_type", QString::fromStdString(balanceTypeToString(entry.entryType)));
        insertEntry.bindValue(":amount", QString::fromStdString(entry.amount.toDbString()));
        insertEntry.bindValue(":narration", entry.narration ? QString::fromStdString(*entry.narration) : QVariant(QMetaType(QMetaType::QString)));
        insertEntry.bindValue(":line_index", lineIdx);
        insertEntry.bindValue(":created_at", QString::fromStdString(now));

        if (!insertEntry.exec()) {
            m_dbManager.rollbackTransaction();
            throw std::runtime_error("Failed to insert voucher entry line: " + insertEntry.lastError().text().toStdString());
        }

        entry.id = entryId;
        entry.voucherId = voucherId;
        entry.lineIndex = lineIdx;
        entry.createdAt = now;
        insertedEntries.push_back(entry);
        lineIdx++;

        // Calculate running balance adjustment:
        // ASSET & EXPENSE: DR increases (+), CR decreases (-)
        // LIABILITY, EQUITY, INCOME: CR increases (+), DR decreases (-)
        auto& ledgerInfo = ledgerMap[entry.ledgerId];
        ExactDecimal updatedBal;

        if (ledgerInfo.nature == GroupNature::Asset || ledgerInfo.nature == GroupNature::Expense) {
            updatedBal = (entry.entryType == BalanceType::Dr)
                ? ledgerInfo.currentBalance.add(entry.amount)
                : ledgerInfo.currentBalance.sub(entry.amount);
        } else {
            updatedBal = (entry.entryType == BalanceType::Cr)
                ? ledgerInfo.currentBalance.add(entry.amount)
                : ledgerInfo.currentBalance.sub(entry.amount);
        }

        ledgerInfo.currentBalance = updatedBal;

        QSqlQuery updateBal(m_dbManager.database());
        updateBal.prepare("UPDATE ledgers SET current_balance = :bal, updated_at = :upd WHERE id = :id");
        updateBal.bindValue(":bal", QString::fromStdString(updatedBal.toDbString()));
        updateBal.bindValue(":upd", QString::fromStdString(now));
        updateBal.bindValue(":id", QString::fromStdString(entry.ledgerId));

        if (!updateBal.exec()) {
            m_dbManager.rollbackTransaction();
            throw std::runtime_error("Failed to update ledger balance: " + updateBal.lastError().text().toStdString());
        }
    }

    // Record Audit Event
    std::string auditId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    QSqlQuery auditQuery(m_dbManager.database());
    auditQuery.prepare(R"(
        INSERT INTO audit_events (
            id, company_id, entity_type, entity_id, action, performed_by, changes_json, performed_at
        ) VALUES (
            :id, :company_id, 'VOUCHER', :entity_id, 'POST', 'SYSTEM', :changes_json, :performed_at
        )
    )");
    auditQuery.bindValue(":id", QString::fromStdString(auditId));
    auditQuery.bindValue(":company_id", QString::fromStdString(voucher.companyId));
    auditQuery.bindValue(":entity_id", QString::fromStdString(voucherId));
    auditQuery.bindValue(":changes_json", QString("{\"voucherNumber\":\"%1\",\"totalAmount\":\"%2\",\"entries\":%3}")
        .arg(QString::fromStdString(voucher.voucherNumber))
        .arg(QString::fromStdString(voucher.totalAmount.toDbString()))
        .arg(insertedEntries.size()));
    auditQuery.bindValue(":performed_at", QString::fromStdString(now));
    auditQuery.exec();

    m_dbManager.commitTransaction();

    voucher.id = voucherId;
    voucher.status = VoucherStatus::Posted;
    voucher.entries = std::move(insertedEntries);
    voucher.createdAt = now;
    voucher.updatedAt = now;

    utilities::Logger::info("Successfully posted Voucher " + voucher.voucherNumber + " (" + voucher.id + ")");
    return voucher;
}

Voucher AccountingEngine::reverseVoucher(const ReverseVoucherParams& params) {
    m_dbManager.beginTransaction();

    // 1. Fetch original voucher
    QSqlQuery vQuery(m_dbManager.database());
    vQuery.prepare("SELECT id, voucher_type_id, voucher_number, date, total_amount, status, is_cancelled, is_reversed FROM vouchers WHERE id = :vid AND company_id = :cid");
    vQuery.bindValue(":vid", QString::fromStdString(params.voucherId));
    vQuery.bindValue(":cid", QString::fromStdString(params.companyId));

    if (!vQuery.exec() || !vQuery.next()) {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Voucher not found in company: " + params.voucherId);
    }

    std::string origId = vQuery.value(0).toString().toStdString();
    std::string vTypeId = vQuery.value(1).toString().toStdString();
    std::string origNumber = vQuery.value(2).toString().toStdString();
    ExactDecimal totalAmt = ExactDecimal::fromString(vQuery.value(4).toString().toStdString());
    std::string status = vQuery.value(5).toString().toStdString();
    bool isCancelled = vQuery.value(6).toInt() != 0;
    bool isReversed = vQuery.value(7).toInt() != 0;

    if (isReversed) {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Voucher '" + origNumber + "' has already been reversed");
    }
    if (isCancelled) {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Voucher '" + origNumber + "' has been cancelled");
    }
    if (status != "POSTED") {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Cannot reverse voucher with status '" + status + "'");
    }

    // 2. Fetch original voucher entries
    QSqlQuery eQuery(m_dbManager.database());
    eQuery.prepare("SELECT ledger_id, entry_type, amount, narration, line_index FROM voucher_entries WHERE voucher_id = :vid ORDER BY line_index ASC");
    eQuery.bindValue(":vid", QString::fromStdString(origId));

    struct OrigEntry {
        std::string ledgerId;
        BalanceType entryType;
        ExactDecimal amount;
        std::optional<std::string> narration;
        int lineIndex;
    };
    std::vector<OrigEntry> origEntries;

    if (eQuery.exec()) {
        while (eQuery.next()) {
            OrigEntry e;
            e.ledgerId = eQuery.value(0).toString().toStdString();
            e.entryType = balanceTypeFromString(eQuery.value(1).toString().toStdString());
            e.amount = ExactDecimal::fromString(eQuery.value(2).toString().toStdString());
            if (!eQuery.value(3).isNull()) e.narration = eQuery.value(3).toString().toStdString();
            e.lineIndex = eQuery.value(4).toInt();
            origEntries.push_back(e);
        }
    }

    if (origEntries.empty()) {
        m_dbManager.rollbackTransaction();
        throw std::runtime_error("Cannot reverse voucher with no line entries");
    }

    // 3. Fetch ledgers info
    std::set<std::string> ledgerIds;
    for (const auto& e : origEntries) ledgerIds.insert(e.ledgerId);

    struct LedgerInfo {
        std::string id;
        ExactDecimal currentBalance;
        GroupNature nature;
    };
    std::map<std::string, LedgerInfo> ledgerMap;

    for (const auto& lid : ledgerIds) {
        QSqlQuery lQuery(m_dbManager.database());
        lQuery.prepare("SELECT l.id, l.current_balance, g.nature FROM ledgers l JOIN accounting_groups g ON l.group_id = g.id WHERE l.id = :lid");
        lQuery.bindValue(":lid", QString::fromStdString(lid));
        if (lQuery.exec() && lQuery.next()) {
            LedgerInfo info;
            info.id = lid;
            info.currentBalance = ExactDecimal::fromString(lQuery.value(1).toString().toStdString());
            info.nature = groupNatureFromString(lQuery.value(2).toString().toStdString());
            ledgerMap[lid] = info;
        }
    }

    // 4. Create Reversal Counter-Voucher
    std::string revId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    std::string revNumber = "REV-" + origNumber + "-" + std::to_string(std::time(nullptr) % 10000);
    std::string now = utilities::DateUtils::currentIsoDateTime();

    QSqlQuery insertRev(m_dbManager.database());
    insertRev.prepare(R"(
        INSERT INTO vouchers (
            id, company_id, voucher_type_id, voucher_number, date, reference_number,
            narration, total_amount, status, is_cancelled, is_reversed, reversal_voucher_id,
            unmapped_fields, created_at, updated_at
        ) VALUES (
            :id, :company_id, :voucher_type_id, :voucher_number, :date, :reference_number,
            :narration, :total_amount, 'POSTED', 0, 0, NULL, '{}', :created_at, :updated_at
        )
    )");
    insertRev.bindValue(":id", QString::fromStdString(revId));
    insertRev.bindValue(":company_id", QString::fromStdString(params.companyId));
    insertRev.bindValue(":voucher_type_id", QString::fromStdString(vTypeId));
    insertRev.bindValue(":voucher_number", QString::fromStdString(revNumber));
    insertRev.bindValue(":date", QString::fromStdString(params.reversalDate));
    insertRev.bindValue(":reference_number", QString::fromStdString(origNumber));
    insertRev.bindValue(":narration", QString::fromStdString("Reversal of " + origNumber + ": " + params.reason));
    insertRev.bindValue(":total_amount", QString::fromStdString(totalAmt.toDbString()));
    insertRev.bindValue(":created_at", QString::fromStdString(now));
    insertRev.bindValue(":updated_at", QString::fromStdString(now));

    if (!insertRev.exec()) {
        m_dbManager.rollbackTransaction();
        throw std::runtime_error("Failed to insert reversal voucher header: " + insertRev.lastError().text().toStdString());
    }

    // 5. Invert line items (DR <-> CR) and adjust balances
    std::vector<VoucherEntry> revEntries;
    int lineIdx = 1;

    for (const auto& oe : origEntries) {
        BalanceType invertedType = (oe.entryType == BalanceType::Dr) ? BalanceType::Cr : BalanceType::Dr;
        std::string entryId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();

        QSqlQuery insertEntry(m_dbManager.database());
        insertEntry.prepare(R"(
            INSERT INTO voucher_entries (
                id, voucher_id, ledger_id, entry_type, amount, narration, line_index, created_at
            ) VALUES (
                :id, :voucher_id, :ledger_id, :entry_type, :amount, :narration, :line_index, :created_at
            )
        )");
        insertEntry.bindValue(":id", QString::fromStdString(entryId));
        insertEntry.bindValue(":voucher_id", QString::fromStdString(revId));
        insertEntry.bindValue(":ledger_id", QString::fromStdString(oe.ledgerId));
        insertEntry.bindValue(":entry_type", QString::fromStdString(balanceTypeToString(invertedType)));
        insertEntry.bindValue(":amount", QString::fromStdString(oe.amount.toDbString()));
        insertEntry.bindValue(":narration", QString::fromStdString("Offsetting line " + std::to_string(oe.lineIndex)));
        insertEntry.bindValue(":line_index", lineIdx);
        insertEntry.bindValue(":created_at", QString::fromStdString(now));

        if (!insertEntry.exec()) {
            m_dbManager.rollbackTransaction();
            throw std::runtime_error("Failed to insert reversal entry: " + insertEntry.lastError().text().toStdString());
        }

        VoucherEntry ve;
        ve.id = entryId;
        ve.voucherId = revId;
        ve.ledgerId = oe.ledgerId;
        ve.entryType = invertedType;
        ve.amount = oe.amount;
        ve.narration = "Offsetting line " + std::to_string(oe.lineIndex);
        ve.lineIndex = lineIdx;
        ve.createdAt = now;
        revEntries.push_back(ve);
        lineIdx++;

        // Adjust running balance using inverted entry
        auto& lInfo = ledgerMap[oe.ledgerId];
        ExactDecimal updatedBal;
        if (lInfo.nature == GroupNature::Asset || lInfo.nature == GroupNature::Expense) {
            updatedBal = (invertedType == BalanceType::Dr)
                ? lInfo.currentBalance.add(oe.amount)
                : lInfo.currentBalance.sub(oe.amount);
        } else {
            updatedBal = (invertedType == BalanceType::Cr)
                ? lInfo.currentBalance.add(oe.amount)
                : lInfo.currentBalance.sub(oe.amount);
        }
        lInfo.currentBalance = updatedBal;

        QSqlQuery updateBal(m_dbManager.database());
        updateBal.prepare("UPDATE ledgers SET current_balance = :bal, updated_at = :upd WHERE id = :id");
        updateBal.bindValue(":bal", QString::fromStdString(updatedBal.toDbString()));
        updateBal.bindValue(":upd", QString::fromStdString(now));
        updateBal.bindValue(":id", QString::fromStdString(oe.ledgerId));
        updateBal.exec();
    }

    // 6. Update original voucher state
    QSqlQuery updOrig(m_dbManager.database());
    updOrig.prepare("UPDATE vouchers SET is_reversed = 1, status = 'REVERSED', reversal_voucher_id = :revid, updated_at = :upd WHERE id = :id");
    updOrig.bindValue(":revid", QString::fromStdString(revId));
    updOrig.bindValue(":upd", QString::fromStdString(now));
    updOrig.bindValue(":id", QString::fromStdString(origId));
    updOrig.exec();

    // 7. Audit event
    std::string auditId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    QSqlQuery auditQuery(m_dbManager.database());
    auditQuery.prepare(R"(
        INSERT INTO audit_events (
            id, company_id, entity_type, entity_id, action, performed_by, changes_json, performed_at
        ) VALUES (
            :id, :company_id, 'VOUCHER', :entity_id, 'REVERSE', 'SYSTEM', :changes_json, :performed_at
        )
    )");
    auditQuery.bindValue(":id", QString::fromStdString(auditId));
    auditQuery.bindValue(":company_id", QString::fromStdString(params.companyId));
    auditQuery.bindValue(":entity_id", QString::fromStdString(origId));
    auditQuery.bindValue(":changes_json", QString("{\"original\":\"%1\",\"reversal\":\"%2\",\"reason\":\"%3\"}")
        .arg(QString::fromStdString(origNumber))
        .arg(QString::fromStdString(revNumber))
        .arg(QString::fromStdString(params.reason)));
    auditQuery.bindValue(":performed_at", QString::fromStdString(now));
    auditQuery.exec();

    m_dbManager.commitTransaction();

    Voucher result;
    result.id = revId;
    result.companyId = params.companyId;
    result.voucherTypeId = vTypeId;
    result.voucherNumber = revNumber;
    result.date = params.reversalDate;
    result.referenceNumber = origNumber;
    result.narration = "Reversal of " + origNumber + ": " + params.reason;
    result.totalAmount = totalAmt;
    result.status = VoucherStatus::Posted;
    result.entries = std::move(revEntries);
    result.createdAt = now;
    result.updatedAt = now;

    utilities::Logger::info("Created Reversal Counter-Voucher: " + revNumber + " for " + origNumber);
    return result;
}

bool AccountingEngine::cancelVoucher(const std::string& companyId, const std::string& voucherId, const std::string& reason) {
    m_dbManager.beginTransaction();

    QSqlQuery vQuery(m_dbManager.database());
    vQuery.prepare("SELECT id, voucher_number, status, is_cancelled FROM vouchers WHERE id = :vid AND company_id = :cid");
    vQuery.bindValue(":vid", QString::fromStdString(voucherId));
    vQuery.bindValue(":cid", QString::fromStdString(companyId));

    if (!vQuery.exec() || !vQuery.next()) {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Voucher not found: " + voucherId);
    }

    std::string vNumber = vQuery.value(1).toString().toStdString();
    std::string status = vQuery.value(2).toString().toStdString();
    bool isCancelled = vQuery.value(3).toInt() != 0;

    if (isCancelled || status != "POSTED") {
        m_dbManager.rollbackTransaction();
        throw std::invalid_argument("Cannot cancel voucher with status: " + status);
    }

    // Fetch entries
    QSqlQuery eQuery(m_dbManager.database());
    eQuery.prepare("SELECT ledger_id, entry_type, amount FROM voucher_entries WHERE voucher_id = :vid");
    eQuery.bindValue(":vid", QString::fromStdString(voucherId));

    std::string now = utilities::DateUtils::currentIsoDateTime();

    if (eQuery.exec()) {
        while (eQuery.next()) {
            std::string lid = eQuery.value(0).toString().toStdString();
            BalanceType etype = balanceTypeFromString(eQuery.value(1).toString().toStdString());
            ExactDecimal amt = ExactDecimal::fromString(eQuery.value(2).toString().toStdString());

            // Fetch ledger info
            QSqlQuery lQuery(m_dbManager.database());
            lQuery.prepare("SELECT l.current_balance, g.nature FROM ledgers l JOIN accounting_groups g ON l.group_id = g.id WHERE l.id = :lid");
            lQuery.bindValue(":lid", QString::fromStdString(lid));
            if (lQuery.exec() && lQuery.next()) {
                ExactDecimal currentBal = ExactDecimal::fromString(lQuery.value(0).toString().toStdString());
                GroupNature nature = groupNatureFromString(lQuery.value(1).toString().toStdString());

                // Roll back entry impact:
                ExactDecimal updatedBal;
                if (nature == GroupNature::Asset || nature == GroupNature::Expense) {
                    updatedBal = (etype == BalanceType::Dr)
                        ? currentBal.sub(amt)
                        : currentBal.add(amt);
                } else {
                    updatedBal = (etype == BalanceType::Cr)
                        ? currentBal.sub(amt)
                        : currentBal.add(amt);
                }

                QSqlQuery upd(m_dbManager.database());
                upd.prepare("UPDATE ledgers SET current_balance = :bal, updated_at = :upd WHERE id = :id");
                upd.bindValue(":bal", QString::fromStdString(updatedBal.toDbString()));
                upd.bindValue(":upd", QString::fromStdString(now));
                upd.bindValue(":id", QString::fromStdString(lid));
                upd.exec();
            }
        }
    }

    // Update voucher status
    QSqlQuery updV(m_dbManager.database());
    updV.prepare("UPDATE vouchers SET is_cancelled = 1, status = 'CANCELLED', updated_at = :upd WHERE id = :id");
    updV.bindValue(":upd", QString::fromStdString(now));
    updV.bindValue(":id", QString::fromStdString(voucherId));
    updV.exec();

    // Audit event
    std::string auditId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    QSqlQuery auditQuery(m_dbManager.database());
    auditQuery.prepare(R"(
        INSERT INTO audit_events (
            id, company_id, entity_type, entity_id, action, performed_by, changes_json, performed_at
        ) VALUES (
            :id, :company_id, 'VOUCHER', :entity_id, 'CANCEL', 'SYSTEM', :changes_json, :performed_at
        )
    )");
    auditQuery.bindValue(":id", QString::fromStdString(auditId));
    auditQuery.bindValue(":company_id", QString::fromStdString(companyId));
    auditQuery.bindValue(":entity_id", QString::fromStdString(voucherId));
    auditQuery.bindValue(":changes_json", QString("{\"voucherNumber\":\"%1\",\"reason\":\"%2\"}")
        .arg(QString::fromStdString(vNumber))
        .arg(QString::fromStdString(reason)));
    auditQuery.bindValue(":performed_at", QString::fromStdString(now));
    auditQuery.exec();

    m_dbManager.commitTransaction();
    utilities::Logger::info("Cancelled Voucher: " + vNumber + " (" + voucherId + ")");
    return true;
}

std::optional<Voucher> AccountingEngine::getVoucherWithEntries(const std::string& voucherId) {
    QSqlQuery vQuery(m_dbManager.database());
    vQuery.prepare("SELECT id, company_id, voucher_type_id, voucher_number, date, reference_number, reference_date, narration, total_amount, status, is_cancelled, is_reversed, reversal_voucher_id, unmapped_fields, created_at, updated_at FROM vouchers WHERE id = :id");
    vQuery.bindValue(":id", QString::fromStdString(voucherId));

    if (!vQuery.exec() || !vQuery.next()) {
        return std::nullopt;
    }

    Voucher v;
    v.id = vQuery.value(0).toString().toStdString();
    v.companyId = vQuery.value(1).toString().toStdString();
    v.voucherTypeId = vQuery.value(2).toString().toStdString();
    v.voucherNumber = vQuery.value(3).toString().toStdString();
    v.date = vQuery.value(4).toString().toStdString();
    if (!vQuery.value(5).isNull()) v.referenceNumber = vQuery.value(5).toString().toStdString();
    if (!vQuery.value(6).isNull()) v.referenceDate = vQuery.value(6).toString().toStdString();
    if (!vQuery.value(7).isNull()) v.narration = vQuery.value(7).toString().toStdString();
    v.totalAmount = ExactDecimal::fromString(vQuery.value(8).toString().toStdString());
    v.status = voucherStatusFromString(vQuery.value(9).toString().toStdString());
    v.isCancelled = vQuery.value(10).toInt() != 0;
    v.isReversed = vQuery.value(11).toInt() != 0;
    if (!vQuery.value(12).isNull()) v.reversalVoucherId = vQuery.value(12).toString().toStdString();
    v.unmappedFields = vQuery.value(13).toString().toStdString();
    v.createdAt = vQuery.value(14).toString().toStdString();
    v.updatedAt = vQuery.value(15).toString().toStdString();

    QSqlQuery eQuery(m_dbManager.database());
    eQuery.prepare("SELECT id, voucher_id, ledger_id, entry_type, amount, narration, line_index, created_at FROM voucher_entries WHERE voucher_id = :vid ORDER BY line_index ASC");
    eQuery.bindValue(":vid", QString::fromStdString(voucherId));

    if (eQuery.exec()) {
        while (eQuery.next()) {
            VoucherEntry ve;
            ve.id = eQuery.value(0).toString().toStdString();
            ve.voucherId = eQuery.value(1).toString().toStdString();
            ve.ledgerId = eQuery.value(2).toString().toStdString();
            ve.entryType = balanceTypeFromString(eQuery.value(3).toString().toStdString());
            ve.amount = ExactDecimal::fromString(eQuery.value(4).toString().toStdString());
            if (!eQuery.value(5).isNull()) ve.narration = eQuery.value(5).toString().toStdString();
            ve.lineIndex = eQuery.value(6).toInt();
            ve.createdAt = eQuery.value(7).toString().toStdString();
            v.entries.push_back(ve);
        }
    }

    return v;
}

std::string AccountingEngine::getNextVoucherNumber(const std::string& companyId, const std::string& voucherTypeId) {
    QSqlQuery q(m_dbManager.database());
    q.prepare("SELECT MAX(CAST(voucher_number AS INTEGER)) FROM vouchers WHERE company_id = :cid AND voucher_type_id = :vtid");
    q.bindValue(":cid", QString::fromStdString(companyId));
    q.bindValue(":vtid", QString::fromStdString(voucherTypeId));
    int maxNo = 0;
    if (q.exec() && q.next() && !q.value(0).isNull()) {
        maxNo = q.value(0).toInt();
    }
    return std::to_string(maxNo + 1);
}

} // namespace tally::domain
