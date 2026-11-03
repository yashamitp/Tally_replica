#include "JsonExchange.hpp"
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <map>
#include <set>
#include "../../utilities/DateUtils.hpp"
#include "../../utilities/Logger.hpp"

namespace tally::integrations::json {

JsonExchange::JsonExchange(database::DatabaseManager& dbManager)
    : m_dbManager(dbManager), m_accountingEngine(dbManager) {}

QJsonObject JsonExchange::exportMastersJson(const std::string& companyId) {
    QJsonObject root;
    root["version"] = "1.0";
    root["format"] = "TALLY_COMPATIBLE_JSON_MASTERS";
    root["exportedAt"] = QString::fromStdString(utilities::DateUtils::currentIsoDateTime());

    // Company
    QSqlQuery cQuery(m_dbManager.database());
    cQuery.prepare("SELECT name FROM companies WHERE id = :cid");
    cQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (cQuery.exec() && cQuery.next()) {
        root["companyName"] = cQuery.value(0).toString();
    }

    // Groups
    std::map<std::string, std::string> groupNames;
    QJsonArray groupsArray;
    QSqlQuery gQuery(m_dbManager.database());
    gQuery.prepare("SELECT id, name, parent_id, nature, affects_gross_profit FROM accounting_groups WHERE company_id = :cid ORDER BY name");
    gQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (gQuery.exec()) {
        while (gQuery.next()) {
            std::string gid = gQuery.value(0).toString().toStdString();
            std::string gname = gQuery.value(1).toString().toStdString();
            groupNames[gid] = gname;

            QJsonObject gobj;
            gobj["name"] = QString::fromStdString(gname);
            if (!gQuery.value(2).isNull()) {
                gobj["parentId"] = gQuery.value(2).toString();
            } else {
                gobj["parentId"] = QJsonValue::Null;
            }
            gobj["nature"] = gQuery.value(3).toString();
            gobj["affectsGrossProfit"] = gQuery.value(4).toInt() != 0;
            groupsArray.append(gobj);
        }
    }
    root["groups"] = groupsArray;

    // Ledgers
    QJsonArray ledgersArray;
    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare("SELECT id, group_id, name, opening_balance, opening_balance_type, current_balance, gstin, pan, address, state, pincode FROM ledgers WHERE company_id = :cid ORDER BY name");
    lQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (lQuery.exec()) {
        while (lQuery.next()) {
            QJsonObject lobj;
            lobj["id"] = lQuery.value(0).toString();
            std::string gid = lQuery.value(1).toString().toStdString();
            lobj["groupName"] = QString::fromStdString(groupNames.count(gid) ? groupNames[gid] : "Current Assets");
            lobj["name"] = lQuery.value(2).toString();
            lobj["openingBalance"] = lQuery.value(3).toString();
            lobj["openingBalanceType"] = lQuery.value(4).toString();
            lobj["currentBalance"] = lQuery.value(5).toString();
            if (!lQuery.value(6).isNull()) lobj["gstin"] = lQuery.value(6).toString();
            if (!lQuery.value(7).isNull()) lobj["pan"] = lQuery.value(7).toString();
            if (!lQuery.value(8).isNull()) lobj["address"] = lQuery.value(8).toString();
            if (!lQuery.value(9).isNull()) lobj["state"] = lQuery.value(9).toString();
            if (!lQuery.value(10).isNull()) lobj["pincode"] = lQuery.value(10).toString();
            ledgersArray.append(lobj);
        }
    }
    root["ledgers"] = ledgersArray;

    return root;
}

QJsonObject JsonExchange::exportVouchersJson(const std::string& companyId,
                                             const std::optional<std::string>& fromDate,
                                             const std::optional<std::string>& toDate) {
    QJsonObject root;
    root["version"] = "1.0";
    root["format"] = "TALLY_COMPATIBLE_JSON_VOUCHERS";
    root["exportedAt"] = QString::fromStdString(utilities::DateUtils::currentIsoDateTime());

    // Company
    QSqlQuery cQuery(m_dbManager.database());
    cQuery.prepare("SELECT name FROM companies WHERE id = :cid");
    cQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (cQuery.exec() && cQuery.next()) {
        root["companyName"] = cQuery.value(0).toString();
    }

    // Voucher Types
    std::map<std::string, std::string> vTypeNames;
    QSqlQuery vtQuery(m_dbManager.database());
    vtQuery.prepare("SELECT id, name FROM voucher_types WHERE company_id = :cid");
    vtQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (vtQuery.exec()) {
        while (vtQuery.next()) {
            vTypeNames[vtQuery.value(0).toString().toStdString()] = vtQuery.value(1).toString().toStdString();
        }
    }

    // Ledgers
    std::map<std::string, std::string> ledgerNames;
    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare("SELECT id, name FROM ledgers WHERE company_id = :cid");
    lQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (lQuery.exec()) {
        while (lQuery.next()) {
            ledgerNames[lQuery.value(0).toString().toStdString()] = lQuery.value(1).toString().toStdString();
        }
    }

    // Query Vouchers
    std::string sql = "SELECT id, voucher_type_id, voucher_number, date, reference_number, narration, total_amount "
                      "FROM vouchers WHERE company_id = :cid AND is_cancelled = 0";
    if (fromDate.has_value()) sql += " AND date >= :from_date";
    if (toDate.has_value()) sql += " AND date <= :to_date";
    sql += " ORDER BY date ASC, voucher_number ASC";

    QSqlQuery vQuery(m_dbManager.database());
    vQuery.prepare(QString::fromStdString(sql));
    vQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (fromDate.has_value()) vQuery.bindValue(":from_date", QString::fromStdString(*fromDate));
    if (toDate.has_value()) vQuery.bindValue(":to_date", QString::fromStdString(*toDate));

    QJsonArray vouchersArray;
    if (vQuery.exec()) {
        while (vQuery.next()) {
            QJsonObject vobj;
            std::string vid = vQuery.value(0).toString().toStdString();
            std::string vtid = vQuery.value(1).toString().toStdString();

            vobj["voucherType"] = QString::fromStdString(vTypeNames.count(vtid) ? vTypeNames[vtid] : "Journal");
            vobj["voucherNumber"] = vQuery.value(2).toString();
            vobj["date"] = vQuery.value(3).toString();
            if (!vQuery.value(4).isNull()) vobj["referenceNumber"] = vQuery.value(4).toString();
            if (!vQuery.value(5).isNull()) vobj["narration"] = vQuery.value(5).toString();
            vobj["totalAmount"] = vQuery.value(6).toString();

            // Entries
            QJsonArray entriesArray;
            QSqlQuery eQuery(m_dbManager.database());
            eQuery.prepare("SELECT ledger_id, entry_type, amount, narration FROM voucher_entries WHERE voucher_id = :vid ORDER BY line_index ASC");
            eQuery.bindValue(":vid", QString::fromStdString(vid));
            if (eQuery.exec()) {
                while (eQuery.next()) {
                    QJsonObject eobj;
                    std::string lid = eQuery.value(0).toString().toStdString();
                    eobj["ledgerName"] = QString::fromStdString(ledgerNames.count(lid) ? ledgerNames[lid] : "Unknown");
                    eobj["entryType"] = eQuery.value(1).toString();
                    eobj["amount"] = eQuery.value(2).toString();
                    if (!eQuery.value(3).isNull()) eobj["narration"] = eQuery.value(3).toString();
                    entriesArray.append(eobj);
                }
            }
            vobj["entries"] = entriesArray;
            vouchersArray.append(vobj);
        }
    }
    root["vouchers"] = vouchersArray;

    return root;
}

xml::ImportExecutionResult JsonExchange::importMastersJson(const std::string& companyId, const QJsonObject& rootObj) {
    xml::ImportExecutionResult result;
    m_dbManager.beginTransaction();

    try {
        std::string now = utilities::DateUtils::currentIsoDateTime();

        // 1. Existing groups
        std::map<std::string, std::string> groupNameToId;
        QSqlQuery gQuery(m_dbManager.database());
        gQuery.prepare("SELECT id, LOWER(name) FROM accounting_groups WHERE company_id = :cid");
        gQuery.bindValue(":cid", QString::fromStdString(companyId));
        if (gQuery.exec()) {
            while (gQuery.next()) {
                groupNameToId[gQuery.value(1).toString().toStdString()] = gQuery.value(0).toString().toStdString();
            }
        }

        // Import Groups
        QJsonArray groups = rootObj["groups"].toArray();
        for (const auto& val : groups) {
            QJsonObject g = val.toObject();
            std::string name = g["name"].toString().trimmed().toStdString();
            if (name.empty()) continue;
            std::string lower = QString::fromStdString(name).toLower().toStdString();

            if (groupNameToId.find(lower) == groupNameToId.end()) {
                std::string gid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
                std::string natureStr = g["nature"].toString().toUpper().toStdString();
                if (natureStr.empty()) natureStr = "ASSET";

                QSqlQuery insG(m_dbManager.database());
                insG.prepare(R"(
                    INSERT INTO accounting_groups (
                        id, company_id, name, nature, affects_gross_profit, is_primary, is_system, created_at, updated_at
                    ) VALUES (
                        :id, :cid, :name, :nature, :gp, 0, 0, :now, :now
                    )
                )");
                insG.bindValue(":id", QString::fromStdString(gid));
                insG.bindValue(":cid", QString::fromStdString(companyId));
                insG.bindValue(":name", QString::fromStdString(name));
                insG.bindValue(":nature", QString::fromStdString(natureStr));
                insG.bindValue(":gp", g["affectsGrossProfit"].toBool() ? 1 : 0);
                insG.bindValue(":now", QString::fromStdString(now));
                insG.exec();
                groupNameToId[lower] = gid;
                result.createdGroups++;
            } else {
                result.skippedCount++;
            }
        }

        // 2. Existing ledgers
        std::map<std::string, std::string> ledgerNameToId;
        QSqlQuery lQuery(m_dbManager.database());
        lQuery.prepare("SELECT id, LOWER(name) FROM ledgers WHERE company_id = :cid");
        lQuery.bindValue(":cid", QString::fromStdString(companyId));
        if (lQuery.exec()) {
            while (lQuery.next()) {
                ledgerNameToId[lQuery.value(1).toString().toStdString()] = lQuery.value(0).toString().toStdString();
            }
        }

        std::string fallbackGroupId = groupNameToId.empty() ? "" : groupNameToId.begin()->second;

        // Import Ledgers
        QJsonArray ledgers = rootObj["ledgers"].toArray();
        for (const auto& val : ledgers) {
            QJsonObject l = val.toObject();
            std::string name = l["name"].toString().trimmed().toStdString();
            if (name.empty()) continue;
            std::string lower = QString::fromStdString(name).toLower().toStdString();

            if (ledgerNameToId.find(lower) == ledgerNameToId.end()) {
                std::string lid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
                std::string gname = l["groupName"].toString().trimmed().toLower().toStdString();
                std::string gid = fallbackGroupId;
                if (groupNameToId.count(gname)) gid = groupNameToId[gname];

                std::string opBal = l["openingBalance"].toString("0.0000").toStdString();
                std::string opType = l["openingBalanceType"].toString("DR").toUpper().toStdString();

                QSqlQuery insL(m_dbManager.database());
                insL.prepare(R"(
                    INSERT INTO ledgers (
                        id, company_id, group_id, name, opening_balance, opening_balance_type,
                        current_balance, is_active, created_at, updated_at
                    ) VALUES (
                        :id, :cid, :gid, :name, :op_bal, :op_type, :cur_bal, 1, :now, :now
                    )
                )");
                insL.bindValue(":id", QString::fromStdString(lid));
                insL.bindValue(":cid", QString::fromStdString(companyId));
                insL.bindValue(":gid", QString::fromStdString(gid));
                insL.bindValue(":name", QString::fromStdString(name));
                insL.bindValue(":op_bal", QString::fromStdString(opBal));
                insL.bindValue(":op_type", QString::fromStdString(opType));
                insL.bindValue(":cur_bal", QString::fromStdString(opBal));
                insL.bindValue(":now", QString::fromStdString(now));
                insL.exec();
                ledgerNameToId[lower] = lid;
                result.createdLedgers++;
            } else {
                result.skippedCount++;
            }
        }

        m_dbManager.commitTransaction();
        result.success = true;
    } catch (const std::exception& e) {
        m_dbManager.rollbackTransaction();
        result.success = false;
        result.errors.push_back(e.what());
    }

    return result;
}

xml::ImportExecutionResult JsonExchange::importVouchersJson(const std::string& companyId, const QJsonObject& rootObj) {
    xml::ImportExecutionResult result;

    // Ledgers map
    std::map<std::string, std::string> ledgerNameToId;
    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare("SELECT id, LOWER(name) FROM ledgers WHERE company_id = :cid");
    lQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (lQuery.exec()) {
        while (lQuery.next()) {
            ledgerNameToId[lQuery.value(1).toString().toStdString()] = lQuery.value(0).toString().toStdString();
        }
    }

    // Voucher Types map
    std::map<std::string, std::string> vTypeNames;
    QSqlQuery vtQuery(m_dbManager.database());
    vtQuery.prepare("SELECT id, LOWER(name) FROM voucher_types WHERE company_id = :cid");
    vtQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (vtQuery.exec()) {
        while (vtQuery.next()) {
            vTypeNames[vtQuery.value(1).toString().toStdString()] = vtQuery.value(0).toString().toStdString();
        }
    }
    std::string defaultVtId = vTypeNames.count("journal") ? vTypeNames["journal"] : (vTypeNames.empty() ? "" : vTypeNames.begin()->second);

    QJsonArray vouchers = rootObj["vouchers"].toArray();
    for (const auto& val : vouchers) {
        QJsonObject v = val.toObject();
        std::string vtName = v["voucherType"].toString().toLower().toStdString();
        std::string vtid = vTypeNames.count(vtName) ? vTypeNames[vtName] : defaultVtId;

        domain::Voucher voucher;
        voucher.companyId = companyId;
        voucher.voucherTypeId = vtid;
        voucher.voucherNumber = v["voucherNumber"].toString().toStdString();
        voucher.date = v["date"].toString().toStdString();
        if (v.contains("referenceNumber") && !v["referenceNumber"].isNull()) {
            voucher.referenceNumber = v["referenceNumber"].toString().toStdString();
        }
        if (v.contains("narration") && !v["narration"].isNull()) {
            voucher.narration = v["narration"].toString().toStdString();
        }

        QJsonArray entries = v["entries"].toArray();
        int lineIdx = 0;
        for (const auto& eval : entries) {
            QJsonObject e = eval.toObject();
            std::string lname = e["ledgerName"].toString().toLower().toStdString();
            if (ledgerNameToId.find(lname) == ledgerNameToId.end()) {
                result.failedCount++;
                result.errors.push_back("Voucher " + voucher.voucherNumber + ": Ledger not found: " + e["ledgerName"].toString().toStdString());
                continue;
            }

            domain::VoucherEntry entry;
            entry.ledgerId = ledgerNameToId[lname];
            entry.entryType = (e["entryType"].toString().toUpper() == "DR") ? domain::BalanceType::Dr : domain::BalanceType::Cr;
            entry.amount = domain::ExactDecimal::fromString(e["amount"].toString().toStdString());
            if (e.contains("narration") && !e["narration"].isNull()) {
                entry.narration = e["narration"].toString().toStdString();
            }
            entry.lineIndex = lineIdx++;
            voucher.entries.push_back(entry);
        }

        try {
            m_accountingEngine.postVoucher(voucher);
            result.createdVouchers++;
        } catch (const std::exception& ex) {
            result.failedCount++;
            result.errors.push_back(ex.what());
        }
    }

    result.success = (result.failedCount == 0);
    return result;
}

} // namespace tally::integrations::json
