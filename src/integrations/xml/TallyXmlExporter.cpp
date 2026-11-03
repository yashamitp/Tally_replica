#include "TallyXmlExporter.hpp"
#include "TallyXmlWriter.hpp"
#include <QSqlQuery>
#include <QVariant>
#include <map>
#include <vector>
#include "../../utilities/Logger.hpp"

namespace tally::integrations::xml {

TallyXmlExporter::TallyXmlExporter(database::DatabaseManager& dbManager)
    : m_dbManager(dbManager) {}

std::string TallyXmlExporter::exportMasters(const std::string& companyId, int* outRecordCount) {
    // 1. Get Company Name
    std::string companyName;
    QSqlQuery cQuery(m_dbManager.database());
    cQuery.prepare("SELECT name FROM companies WHERE id = :cid");
    cQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (cQuery.exec() && cQuery.next()) {
        companyName = cQuery.value(0).toString().toStdString();
    }

    // 2. Fetch Groups
    std::vector<domain::AccountingGroup> groups;
    std::map<std::string, std::string> groupNames; // id -> name
    QSqlQuery gQuery(m_dbManager.database());
    gQuery.prepare("SELECT id, name, parent_id, nature, affects_gross_profit, is_system FROM accounting_groups WHERE company_id = :cid ORDER BY name");
    gQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (gQuery.exec()) {
        while (gQuery.next()) {
            domain::AccountingGroup g;
            g.id = gQuery.value(0).toString().toStdString();
            g.companyId = companyId;
            g.name = gQuery.value(1).toString().toStdString();
            if (!gQuery.value(2).isNull()) {
                g.parentId = gQuery.value(2).toString().toStdString();
            }
            g.nature = domain::groupNatureFromString(gQuery.value(3).toString().toStdString());
            g.affectsGrossProfit = gQuery.value(4).toInt() != 0;
            g.isSystem = gQuery.value(5).toInt() != 0;
            groupNames[g.id] = g.name;
            groups.push_back(g);
        }
    }

    // 3. Fetch Ledgers
    std::vector<domain::Ledger> ledgers;
    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare("SELECT id, group_id, name, opening_balance, opening_balance_type, gstin, pan, address, state, pincode FROM ledgers WHERE company_id = :cid ORDER BY name");
    lQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (lQuery.exec()) {
        while (lQuery.next()) {
            domain::Ledger l;
            l.id = lQuery.value(0).toString().toStdString();
            l.companyId = companyId;
            l.groupId = lQuery.value(1).toString().toStdString();
            l.name = lQuery.value(2).toString().toStdString();
            l.openingBalance = domain::ExactDecimal::fromString(lQuery.value(3).toString().toStdString());
            l.openingBalanceType = (lQuery.value(4).toString().toUpper() == "DR") ? domain::BalanceType::Dr : domain::BalanceType::Cr;
            if (!lQuery.value(5).isNull()) l.gstin = lQuery.value(5).toString().toStdString();
            if (!lQuery.value(6).isNull()) l.pan = lQuery.value(6).toString().toStdString();
            if (!lQuery.value(7).isNull()) l.address = lQuery.value(7).toString().toStdString();
            if (!lQuery.value(8).isNull()) l.state = lQuery.value(8).toString().toStdString();
            if (!lQuery.value(9).isNull()) l.pincode = lQuery.value(9).toString().toStdString();
            ledgers.push_back(l);
        }
    }

    std::string innerXml;
    for (const auto& g : groups) {
        std::string parentName;
        if (g.parentId.has_value() && !g.parentId->empty()) {
            auto it = groupNames.find(*g.parentId);
            if (it != groupNames.end()) parentName = it->second;
        }
        innerXml += TallyXmlWriter::groupToXml(g, parentName) + "\n";
    }

    for (const auto& l : ledgers) {
        std::string pName = "Current Assets";
        auto it = groupNames.find(l.groupId);
        if (it != groupNames.end()) pName = it->second;
        innerXml += TallyXmlWriter::ledgerToXml(l, pName) + "\n";
    }

    if (outRecordCount) {
        *outRecordCount = static_cast<int>(groups.size() + ledgers.size());
    }

    return TallyXmlWriter::wrapEnvelope("Export", "All Masters", companyName, innerXml);
}

std::string TallyXmlExporter::exportVouchers(const ExportVouchersFilter& filter, int* outRecordCount) {
    // 1. Get Company Name
    std::string companyName;
    QSqlQuery cQuery(m_dbManager.database());
    cQuery.prepare("SELECT name FROM companies WHERE id = :cid");
    cQuery.bindValue(":cid", QString::fromStdString(filter.companyId));
    if (cQuery.exec() && cQuery.next()) {
        companyName = cQuery.value(0).toString().toStdString();
    }

    // 2. Fetch Voucher Types map
    std::map<std::string, std::string> vTypeNames;
    QSqlQuery vtQuery(m_dbManager.database());
    vtQuery.prepare("SELECT id, name FROM voucher_types WHERE company_id = :cid");
    vtQuery.bindValue(":cid", QString::fromStdString(filter.companyId));
    if (vtQuery.exec()) {
        while (vtQuery.next()) {
            vTypeNames[vtQuery.value(0).toString().toStdString()] = vtQuery.value(1).toString().toStdString();
        }
    }

    // 3. Fetch Ledgers map
    std::map<std::string, std::string> ledgerNames;
    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare("SELECT id, name FROM ledgers WHERE company_id = :cid");
    lQuery.bindValue(":cid", QString::fromStdString(filter.companyId));
    if (lQuery.exec()) {
        while (lQuery.next()) {
            ledgerNames[lQuery.value(0).toString().toStdString()] = lQuery.value(1).toString().toStdString();
        }
    }

    // 4. Fetch Vouchers
    std::string sql = "SELECT id, voucher_type_id, voucher_number, date, reference_number, narration, total_amount "
                      "FROM vouchers WHERE company_id = :cid AND is_cancelled = 0";
    if (filter.fromDate.has_value()) sql += " AND date >= :from_date";
    if (filter.toDate.has_value()) sql += " AND date <= :to_date";
    if (filter.voucherTypeId.has_value()) sql += " AND voucher_type_id = :vtid";
    sql += " ORDER BY date ASC, voucher_number ASC";

    QSqlQuery vQuery(m_dbManager.database());
    vQuery.prepare(QString::fromStdString(sql));
    vQuery.bindValue(":cid", QString::fromStdString(filter.companyId));
    if (filter.fromDate.has_value()) vQuery.bindValue(":from_date", QString::fromStdString(*filter.fromDate));
    if (filter.toDate.has_value()) vQuery.bindValue(":to_date", QString::fromStdString(*filter.toDate));
    if (filter.voucherTypeId.has_value()) vQuery.bindValue(":vtid", QString::fromStdString(*filter.voucherTypeId));

    std::vector<domain::Voucher> vouchers;
    if (vQuery.exec()) {
        while (vQuery.next()) {
            domain::Voucher v;
            v.id = vQuery.value(0).toString().toStdString();
            v.companyId = filter.companyId;
            v.voucherTypeId = vQuery.value(1).toString().toStdString();
            v.voucherNumber = vQuery.value(2).toString().toStdString();
            v.date = vQuery.value(3).toString().toStdString();
            if (!vQuery.value(4).isNull()) v.referenceNumber = vQuery.value(4).toString().toStdString();
            if (!vQuery.value(5).isNull()) v.narration = vQuery.value(5).toString().toStdString();
            v.totalAmount = domain::ExactDecimal::fromString(vQuery.value(6).toString().toStdString());
            vouchers.push_back(v);
        }
    }

    // 5. Fetch Entries for each voucher
    std::string innerXml;
    for (auto& v : vouchers) {
        QSqlQuery eQuery(m_dbManager.database());
        eQuery.prepare("SELECT ledger_id, entry_type, amount, narration FROM voucher_entries WHERE voucher_id = :vid ORDER BY line_index ASC");
        eQuery.bindValue(":vid", QString::fromStdString(v.id));
        if (eQuery.exec()) {
            while (eQuery.next()) {
                domain::VoucherEntry e;
                e.ledgerId = eQuery.value(0).toString().toStdString();
                e.entryType = (eQuery.value(1).toString().toUpper() == "DR") ? domain::BalanceType::Dr : domain::BalanceType::Cr;
                e.amount = domain::ExactDecimal::fromString(eQuery.value(2).toString().toStdString());
                if (!eQuery.value(3).isNull()) e.narration = eQuery.value(3).toString().toStdString();
                v.entries.push_back(e);
            }
        }

        std::string vtName = "Journal";
        auto it = vTypeNames.find(v.voucherTypeId);
        if (it != vTypeNames.end()) vtName = it->second;

        innerXml += TallyXmlWriter::voucherToXml(v, vtName, ledgerNames) + "\n";
    }

    if (outRecordCount) {
        *outRecordCount = static_cast<int>(vouchers.size());
    }

    return TallyXmlWriter::wrapEnvelope("Export", "Day Book", companyName, innerXml);
}

} // namespace tally::integrations::xml
