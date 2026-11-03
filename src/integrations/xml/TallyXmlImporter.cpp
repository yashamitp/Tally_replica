#include "TallyXmlImporter.hpp"
#include "TallyXmlReader.hpp"
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <map>
#include <set>
#include "../../utilities/Logger.hpp"
#include "../../utilities/DateUtils.hpp"

namespace tally::integrations::xml {

TallyXmlImporter::TallyXmlImporter(database::DatabaseManager& dbManager)
    : m_dbManager(dbManager), m_accountingEngine(dbManager) {}

ImportPreviewResult TallyXmlImporter::preview(const std::string& companyId, const std::string& xmlContent) {
    ImportPreviewResult result;
    result.sourceFormat = "TALLY_XML";

    ParsedTallyPayload payload = TallyXmlReader::parse(xmlContent);
    result.companiesDetected = payload.companyNames;
    result.groupsCount = static_cast<int>(payload.groups.size());
    result.ledgersCount = static_cast<int>(payload.ledgers.size());
    result.vouchersCount = static_cast<int>(payload.vouchers.size());

    // 1. Existing groups
    std::map<std::string, std::string> existingGroups; // lower(name) -> id
    QSqlQuery gQuery(m_dbManager.database());
    gQuery.prepare("SELECT id, LOWER(name) FROM accounting_groups WHERE company_id = :cid");
    gQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (gQuery.exec()) {
        while (gQuery.next()) {
            existingGroups[gQuery.value(1).toString().toStdString()] = gQuery.value(0).toString().toStdString();
        }
    }

    // 2. Existing ledgers
    std::map<std::string, std::string> existingLedgers; // lower(name) -> id
    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare("SELECT id, LOWER(name) FROM ledgers WHERE company_id = :cid");
    lQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (lQuery.exec()) {
        while (lQuery.next()) {
            existingLedgers[lQuery.value(1).toString().toStdString()] = lQuery.value(0).toString().toStdString();
        }
    }

    // 3. Existing vouchers
    std::set<std::pair<std::string, std::string>> existingVouchers; // (voucherTypeId, voucherNumber)
    QSqlQuery vQuery(m_dbManager.database());
    vQuery.prepare("SELECT voucher_type_id, voucher_number FROM vouchers WHERE company_id = :cid");
    vQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (vQuery.exec()) {
        while (vQuery.next()) {
            existingVouchers.insert({vQuery.value(0).toString().toStdString(), vQuery.value(1).toString().toStdString()});
        }
    }

    // 4. Voucher Types
    std::map<std::string, std::string> vTypeNames; // lower(name) -> id
    QSqlQuery vtQuery(m_dbManager.database());
    vtQuery.prepare("SELECT id, LOWER(name) FROM voucher_types WHERE company_id = :cid");
    vtQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (vtQuery.exec()) {
        while (vtQuery.next()) {
            vTypeNames[vtQuery.value(1).toString().toStdString()] = vtQuery.value(0).toString().toStdString();
        }
    }

    // Evaluate Groups
    for (const auto& grp : payload.groups) {
        ImportPreviewItem item;
        item.type = "GROUP";
        item.identifier = grp.name;
        std::string lowerName = QString::fromStdString(grp.name).toLower().toStdString();
        if (existingGroups.find(lowerName) != existingGroups.end()) {
            item.action = PreviewAction::Skip;
            item.reason = "Group already exists in company";
            result.skippedCount++;
        } else {
            item.action = PreviewAction::Create;
            item.reason = "New group will be created";
            result.acceptedCount++;
            existingGroups[lowerName] = "pending";
        }
        result.items.push_back(item);
    }

    // Evaluate Ledgers
    for (const auto& led : payload.ledgers) {
        ImportPreviewItem item;
        item.type = "LEDGER";
        item.identifier = led.name;
        std::string lowerName = QString::fromStdString(led.name).toLower().toStdString();
        if (existingLedgers.find(lowerName) != existingLedgers.end()) {
            item.action = PreviewAction::Skip;
            item.reason = "Ledger already exists in company";
            result.skippedCount++;
        } else {
            item.action = PreviewAction::Create;
            item.reason = "New ledger will be created";
            result.acceptedCount++;
            existingLedgers[lowerName] = "pending";
        }
        result.items.push_back(item);
    }

    // Evaluate Vouchers
    for (const auto& vch : payload.vouchers) {
        ImportPreviewItem item;
        item.type = "VOUCHER";
        item.identifier = vch.voucherNumber.empty() ? "(Auto)" : vch.voucherNumber;

        if (!vch.isBalanced) {
            item.action = PreviewAction::Reject;
            item.reason = "Double-entry out of balance: Total Dr (" + vch.totalDebit.toCurrencyString() +
                          ") != Total Cr (" + vch.totalCredit.toCurrencyString() + ")";
            result.rejectedCount++;
            result.errors.push_back("Voucher " + item.identifier + ": " + item.reason);
        } else {
            std::string vtLower = QString::fromStdString(vch.voucherTypeName).toLower().toStdString();
            std::string vtId;
            auto itVt = vTypeNames.find(vtLower);
            if (itVt != vTypeNames.end()) {
                vtId = itVt->second;
            }

            if (!vtId.empty() && existingVouchers.find({vtId, vch.voucherNumber}) != existingVouchers.end()) {
                item.action = PreviewAction::Skip;
                item.reason = "Voucher number already exists for type " + vch.voucherTypeName;
                result.skippedCount++;
            } else {
                item.action = PreviewAction::Create;
                item.reason = "Valid balanced voucher ready to post";
                result.acceptedCount++;
                result.expectedDebitsSum = result.expectedDebitsSum.add(vch.totalDebit);
                result.expectedCreditsSum = result.expectedCreditsSum.add(vch.totalCredit);
            }
        }
        result.items.push_back(item);
    }

    result.canCommit = (result.rejectedCount == 0 && result.acceptedCount > 0);
    return result;
}

ImportExecutionResult TallyXmlImporter::execute(const std::string& companyId, const std::string& xmlContent) {
    ImportExecutionResult execResult;
    ParsedTallyPayload payload = TallyXmlReader::parse(xmlContent);

    m_dbManager.beginTransaction();

    try {
        // 1. Group resolution
        std::map<std::string, std::string> groupNameToId; // lower(name) -> id
        QSqlQuery gQuery(m_dbManager.database());
        gQuery.prepare("SELECT id, LOWER(name) FROM accounting_groups WHERE company_id = :cid");
        gQuery.bindValue(":cid", QString::fromStdString(companyId));
        if (gQuery.exec()) {
            while (gQuery.next()) {
                groupNameToId[gQuery.value(1).toString().toStdString()] = gQuery.value(0).toString().toStdString();
            }
        }

        std::string now = utilities::DateUtils::currentIsoDateTime();

        for (const auto& grp : payload.groups) {
            std::string lower = QString::fromStdString(grp.name).toLower().toStdString();
            if (groupNameToId.find(lower) == groupNameToId.end()) {
                std::string newId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
                std::string parentId;
                if (!grp.parentName.empty()) {
                    auto itP = groupNameToId.find(QString::fromStdString(grp.parentName).toLower().toStdString());
                    if (itP != groupNameToId.end()) parentId = itP->second;
                }

                domain::GroupNature nature = domain::GroupNature::Asset;
                std::string lowerParent = QString::fromStdString(grp.parentName).toLower().toStdString();
                if (lower.find("liability") != std::string::npos || lowerParent.find("liability") != std::string::npos) {
                    nature = domain::GroupNature::Liability;
                } else if (lower.find("equity") != std::string::npos || lower.find("capital") != std::string::npos) {
                    nature = domain::GroupNature::Equity;
                } else if (lower.find("income") != std::string::npos || lower.find("sale") != std::string::npos) {
                    nature = domain::GroupNature::Income;
                } else if (lower.find("expense") != std::string::npos || lower.find("purchase") != std::string::npos) {
                    nature = domain::GroupNature::Expense;
                }

                QSqlQuery insertG(m_dbManager.database());
                insertG.prepare(R"(
                    INSERT INTO accounting_groups (
                        id, company_id, name, parent_id, nature, affects_gross_profit, is_primary, is_system, created_at, updated_at
                    ) VALUES (
                        :id, :cid, :name, :pid, :nature, 0, 0, 0, :now, :now
                    )
                )");
                insertG.bindValue(":id", QString::fromStdString(newId));
                insertG.bindValue(":cid", QString::fromStdString(companyId));
                insertG.bindValue(":name", QString::fromStdString(grp.name));
                insertG.bindValue(":pid", parentId.empty() ? QVariant(QMetaType(QMetaType::QString)) : QString::fromStdString(parentId));
                insertG.bindValue(":nature", QString::fromStdString(domain::groupNatureToString(nature)));
                insertG.bindValue(":now", QString::fromStdString(now));
                if (!insertG.exec()) {
                    throw std::runtime_error("Failed to insert group: " + grp.name);
                }
                groupNameToId[lower] = newId;
                execResult.createdGroups++;
            } else {
                execResult.skippedCount++;
            }
        }

        // 2. Ledger resolution
        std::map<std::string, std::string> ledgerNameToId; // lower(name) -> id
        QSqlQuery lQuery(m_dbManager.database());
        lQuery.prepare("SELECT id, LOWER(name) FROM ledgers WHERE company_id = :cid");
        lQuery.bindValue(":cid", QString::fromStdString(companyId));
        if (lQuery.exec()) {
            while (lQuery.next()) {
                ledgerNameToId[lQuery.value(1).toString().toStdString()] = lQuery.value(0).toString().toStdString();
            }
        }

        // Find or fallback to a group for ledgers
        std::string fallbackGroupId;
        if (!groupNameToId.empty()) {
            fallbackGroupId = groupNameToId.begin()->second;
        }

        for (const auto& led : payload.ledgers) {
            std::string lower = QString::fromStdString(led.name).toLower().toStdString();
            if (ledgerNameToId.find(lower) == ledgerNameToId.end()) {
                std::string newId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
                std::string grpId = fallbackGroupId;
                if (!led.parentGroupName.empty()) {
                    auto itG = groupNameToId.find(QString::fromStdString(led.parentGroupName).toLower().toStdString());
                    if (itG != groupNameToId.end()) grpId = itG->second;
                }

                domain::ExactDecimal opBal = domain::ExactDecimal::zero();
                domain::BalanceType opType = domain::BalanceType::Dr;
                if (!led.openingBalance.empty()) {
                    std::string clean = led.openingBalance;
                    if (clean[0] == '-') clean = clean.substr(1);
                    try { opBal = domain::ExactDecimal::fromString(clean); } catch(...) {}
                }
                if (QString::fromStdString(led.isDeemedPositive).compare("No", Qt::CaseInsensitive) == 0) {
                    opType = domain::BalanceType::Cr;
                }

                QSqlQuery insertL(m_dbManager.database());
                insertL.prepare(R"(
                    INSERT INTO ledgers (
                        id, company_id, group_id, name, opening_balance, opening_balance_type,
                        current_balance, is_active, gstin, pan, address, state, pincode, created_at, updated_at
                    ) VALUES (
                        :id, :cid, :gid, :name, :op_bal, :op_type, :cur_bal, 1, :gstin, :pan, :addr, :state, :pin, :now, :now
                    )
                )");
                insertL.bindValue(":id", QString::fromStdString(newId));
                insertL.bindValue(":cid", QString::fromStdString(companyId));
                insertL.bindValue(":gid", QString::fromStdString(grpId));
                insertL.bindValue(":name", QString::fromStdString(led.name));
                insertL.bindValue(":op_bal", QString::fromStdString(opBal.toDbString()));
                insertL.bindValue(":op_type", QString::fromStdString(opType == domain::BalanceType::Dr ? "DR" : "CR"));
                insertL.bindValue(":cur_bal", QString::fromStdString(opBal.toDbString()));
                insertL.bindValue(":gstin", led.gstin.empty() ? QVariant(QMetaType(QMetaType::QString)) : QString::fromStdString(led.gstin));
                insertL.bindValue(":pan", led.pan.empty() ? QVariant(QMetaType(QMetaType::QString)) : QString::fromStdString(led.pan));
                insertL.bindValue(":addr", led.address.empty() ? QVariant(QMetaType(QMetaType::QString)) : QString::fromStdString(led.address));
                insertL.bindValue(":state", led.state.empty() ? QVariant(QMetaType(QMetaType::QString)) : QString::fromStdString(led.state));
                insertL.bindValue(":pin", led.pincode.empty() ? QVariant(QMetaType(QMetaType::QString)) : QString::fromStdString(led.pincode));
                insertL.bindValue(":now", QString::fromStdString(now));
                if (!insertL.exec()) {
                    throw std::runtime_error("Failed to insert ledger: " + led.name);
                }
                ledgerNameToId[lower] = newId;
                execResult.createdLedgers++;
            } else {
                execResult.skippedCount++;
            }
        }

        // 3. Voucher Types Map
        std::map<std::string, std::string> vTypeNames;
        QSqlQuery vtQuery(m_dbManager.database());
        vtQuery.prepare("SELECT id, LOWER(name) FROM voucher_types WHERE company_id = :cid");
        vtQuery.bindValue(":cid", QString::fromStdString(companyId));
        if (vtQuery.exec()) {
            while (vtQuery.next()) {
                vTypeNames[vtQuery.value(1).toString().toStdString()] = vtQuery.value(0).toString().toStdString();
            }
        }
        std::string defaultVtId;
        if (!vTypeNames.empty()) {
            auto itJ = vTypeNames.find("journal");
            defaultVtId = (itJ != vTypeNames.end()) ? itJ->second : vTypeNames.begin()->second;
        }

        // 4. Post Vouchers
        for (const auto& vch : payload.vouchers) {
            if (!vch.isBalanced) {
                throw std::runtime_error("Cannot post unbalanced voucher: " + vch.voucherNumber);
            }

            std::string vtId = defaultVtId;
            auto itVt = vTypeNames.find(QString::fromStdString(vch.voucherTypeName).toLower().toStdString());
            if (itVt != vTypeNames.end()) {
                vtId = itVt->second;
            }

            domain::Voucher v;
            v.companyId = companyId;
            v.voucherTypeId = vtId;
            v.voucherNumber = vch.voucherNumber;
            v.date = vch.date.empty() ? utilities::DateUtils::currentIsoDate() : vch.date;
            if (!vch.referenceNumber.empty()) v.referenceNumber = vch.referenceNumber;
            if (!vch.narration.empty()) v.narration = vch.narration;

            int lineIndex = 0;
            for (const auto& entry : vch.entries) {
                std::string lNameLower = QString::fromStdString(entry.ledgerName).toLower().toStdString();
                auto itL = ledgerNameToId.find(lNameLower);
                if (itL == ledgerNameToId.end()) {
                    // Auto-create ledger if it was not in master list
                    std::string newLedId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
                    QSqlQuery autoL(m_dbManager.database());
                    autoL.prepare(R"(
                        INSERT INTO ledgers (
                            id, company_id, group_id, name, opening_balance, opening_balance_type,
                            current_balance, is_active, created_at, updated_at
                        ) VALUES (
                            :id, :cid, :gid, :name, '0.0000', 'DR', '0.0000', 1, :now, :now
                        )
                    )");
                    autoL.bindValue(":id", QString::fromStdString(newLedId));
                    autoL.bindValue(":cid", QString::fromStdString(companyId));
                    autoL.bindValue(":gid", QString::fromStdString(fallbackGroupId));
                    autoL.bindValue(":name", QString::fromStdString(entry.ledgerName));
                    autoL.bindValue(":now", QString::fromStdString(now));
                    autoL.exec();
                    ledgerNameToId[lNameLower] = newLedId;
                    itL = ledgerNameToId.find(lNameLower);
                    execResult.createdLedgers++;
                }

                domain::VoucherEntry ve;
                ve.ledgerId = itL->second;
                ve.entryType = entry.entryType;
                ve.amount = entry.amount;
                if (!entry.narration.empty()) ve.narration = entry.narration;
                ve.lineIndex = lineIndex++;
                v.entries.push_back(ve);
            }

            // Post voucher via AccountingEngine (note: we are already in transaction,
            // so we commit and allow AccountingEngine to do its transactional posting)
        }

        m_dbManager.commitTransaction();

        // Now post each voucher through AccountingEngine to guarantee double-entry ledger balance updates & audit logs
        for (const auto& vch : payload.vouchers) {
            std::string vtId = defaultVtId;
            auto itVt = vTypeNames.find(QString::fromStdString(vch.voucherTypeName).toLower().toStdString());
            if (itVt != vTypeNames.end()) vtId = itVt->second;

            domain::Voucher v;
            v.companyId = companyId;
            v.voucherTypeId = vtId;
            v.voucherNumber = vch.voucherNumber;
            v.date = vch.date.empty() ? utilities::DateUtils::currentIsoDate() : vch.date;
            if (!vch.referenceNumber.empty()) v.referenceNumber = vch.referenceNumber;
            if (!vch.narration.empty()) v.narration = vch.narration;

            int lineIdx = 0;
            for (const auto& entry : vch.entries) {
                std::string lNameLower = QString::fromStdString(entry.ledgerName).toLower().toStdString();
                std::string lId = ledgerNameToId[lNameLower];
                domain::VoucherEntry ve;
                ve.ledgerId = lId;
                ve.entryType = entry.entryType;
                ve.amount = entry.amount;
                if (!entry.narration.empty()) ve.narration = entry.narration;
                ve.lineIndex = lineIdx++;
                v.entries.push_back(ve);
            }

            try {
                m_accountingEngine.postVoucher(v);
                execResult.createdVouchers++;
            } catch (const std::exception& ex) {
                execResult.failedCount++;
                execResult.errors.push_back("Voucher " + v.voucherNumber + " failed: " + ex.what());
            }
        }

        execResult.success = (execResult.failedCount == 0);
    } catch (const std::exception& e) {
        m_dbManager.rollbackTransaction();
        execResult.success = false;
        execResult.errors.push_back(std::string("Execution failed: ") + e.what());
    }

    return execResult;
}

} // namespace tally::integrations::xml
