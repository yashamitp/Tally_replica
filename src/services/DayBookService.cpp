#include "DayBookService.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <map>
#include <sstream>

namespace tally::services {

DayBookService::DayBookService(database::DatabaseManager& dbManager)
    : m_dbManager(dbManager) {}

DayBookResult DayBookService::getDayBook(const DayBookFilter& filter) {
    DayBookResult result;

    QString whereClause = "WHERE v.company_id = :cid";
    if (filter.fromDate && !filter.fromDate->empty()) whereClause += " AND v.date >= :from_date";
    if (filter.toDate && !filter.toDate->empty()) whereClause += " AND v.date <= :to_date";
    if (filter.voucherTypeId && !filter.voucherTypeId->empty()) whereClause += " AND v.voucher_type_id = :vtid";
    if (filter.category) whereClause += " AND vt.category = :category";
    if (filter.status) whereClause += " AND v.status = :status";

    if (filter.ledgerId && !filter.ledgerId->empty()) {
        whereClause += " AND v.id IN (SELECT voucher_id FROM voucher_entries WHERE ledger_id = :lid)";
    }

    // 1. Get total count
    QString countSql = "SELECT COUNT(*) FROM vouchers v JOIN voucher_types vt ON v.voucher_type_id = vt.id " + whereClause;
    QSqlQuery countQuery(m_dbManager.database());
    countQuery.prepare(countSql);
    countQuery.bindValue(":cid", QString::fromStdString(filter.companyId));
    if (filter.fromDate && !filter.fromDate->empty()) countQuery.bindValue(":from_date", QString::fromStdString(*filter.fromDate));
    if (filter.toDate && !filter.toDate->empty()) countQuery.bindValue(":to_date", QString::fromStdString(*filter.toDate));
    if (filter.voucherTypeId && !filter.voucherTypeId->empty()) countQuery.bindValue(":vtid", QString::fromStdString(*filter.voucherTypeId));
    if (filter.category) countQuery.bindValue(":category", QString::fromStdString(domain::voucherCategoryToString(*filter.category)));
    if (filter.status) countQuery.bindValue(":status", QString::fromStdString(domain::voucherStatusToString(*filter.status)));
    if (filter.ledgerId && !filter.ledgerId->empty()) countQuery.bindValue(":lid", QString::fromStdString(*filter.ledgerId));

    if (countQuery.exec() && countQuery.next()) {
        result.totalCount = countQuery.value(0).toInt();
    }

    if (result.totalCount == 0) {
        return result;
    }

    // 2. Fetch Paginated Voucher Headers
    QString sql = R"(
        SELECT v.id, v.date, v.voucher_number, vt.name, vt.category, v.narration, v.total_amount, v.status
        FROM vouchers v
        JOIN voucher_types vt ON v.voucher_type_id = vt.id
    )" + whereClause + " ORDER BY v.date ASC, v.created_at ASC LIMIT :limit OFFSET :offset";

    QSqlQuery q(m_dbManager.database());
    q.prepare(sql);
    q.bindValue(":cid", QString::fromStdString(filter.companyId));
    if (filter.fromDate && !filter.fromDate->empty()) q.bindValue(":from_date", QString::fromStdString(*filter.fromDate));
    if (filter.toDate && !filter.toDate->empty()) q.bindValue(":to_date", QString::fromStdString(*filter.toDate));
    if (filter.voucherTypeId && !filter.voucherTypeId->empty()) q.bindValue(":vtid", QString::fromStdString(*filter.voucherTypeId));
    if (filter.category) q.bindValue(":category", QString::fromStdString(domain::voucherCategoryToString(*filter.category)));
    if (filter.status) q.bindValue(":status", QString::fromStdString(domain::voucherStatusToString(*filter.status)));
    if (filter.ledgerId && !filter.ledgerId->empty()) q.bindValue(":lid", QString::fromStdString(*filter.ledgerId));
    q.bindValue(":limit", filter.limit);
    q.bindValue(":offset", filter.offset);

    std::vector<std::string> vIds;
    std::vector<DayBookItem> items;

    if (q.exec()) {
        while (q.next()) {
            DayBookItem item;
            item.id = q.value(0).toString().toStdString();
            item.date = q.value(1).toString().toStdString();
            item.voucherNumber = q.value(2).toString().toStdString();
            item.voucherTypeName = q.value(3).toString().toStdString();
            item.voucherCategory = domain::voucherCategoryFromString(q.value(4).toString().toStdString());
            if (!q.value(5).isNull()) item.narration = q.value(5).toString().toStdString();
            item.totalAmount = domain::ExactDecimal::fromString(q.value(6).toString().toStdString());
            item.status = domain::voucherStatusFromString(q.value(7).toString().toStdString());

            vIds.push_back(item.id);
            items.push_back(item);
        }
    }

    if (items.empty()) {
        return result;
    }

    // 3. Fetch Entries for these vouchers
    QString inClause;
    for (size_t i = 0; i < vIds.size(); ++i) {
        if (i > 0) inClause += ",";
        inClause += "'" + QString::fromStdString(vIds[i]) + "'";
    }

    QSqlQuery eq(m_dbManager.database());
    eq.exec(R"(
        SELECT ve.voucher_id, ve.ledger_id, l.name, ve.entry_type, ve.amount, ve.narration
        FROM voucher_entries ve
        JOIN ledgers l ON ve.ledger_id = l.id
        WHERE ve.voucher_id IN ()" + inClause + R"()
        ORDER BY ve.line_index ASC
    )");

    std::map<std::string, std::vector<DayBookItem::EntryLine>> entryMap;
    while (eq.next()) {
        std::string vid = eq.value(0).toString().toStdString();
        DayBookItem::EntryLine line;
        line.ledgerId = eq.value(1).toString().toStdString();
        line.ledgerName = eq.value(2).toString().toStdString();
        line.entryType = domain::balanceTypeFromString(eq.value(3).toString().toStdString());
        line.amount = domain::ExactDecimal::fromString(eq.value(4).toString().toStdString());
        if (!eq.value(5).isNull()) line.narration = eq.value(5).toString().toStdString();

        entryMap[vid].push_back(line);
    }

    // 4. Populate entries & particulars
    domain::ExactDecimal sumDr = domain::ExactDecimal::zero();
    domain::ExactDecimal sumCr = domain::ExactDecimal::zero();

    for (auto& item : items) {
        item.entries = entryMap[item.id];

        std::vector<std::string> drNames;
        std::vector<std::string> crNames;
        for (const auto& e : item.entries) {
            if (e.entryType == domain::BalanceType::Dr) {
                drNames.push_back(e.ledgerName);
                sumDr = sumDr.add(e.amount);
            } else {
                crNames.push_back(e.ledgerName);
                sumCr = sumCr.add(e.amount);
            }
        }

        if (drNames.size() == 1 && crNames.size() == 1) {
            item.particulars = drNames[0] + " / " + crNames[0];
        } else if (drNames.size() == 1) {
            item.particulars = "To ";
            for (size_t i = 0; i < crNames.size(); ++i) {
                if (i > 0) item.particulars += ", ";
                item.particulars += crNames[i];
            }
        } else if (crNames.size() == 1) {
            item.particulars = "By ";
            for (size_t i = 0; i < drNames.size(); ++i) {
                if (i > 0) item.particulars += ", ";
                item.particulars += drNames[i];
            }
        } else {
            item.particulars = "As per details";
        }
    }

    result.items = std::move(items);
    result.totalDebitSum = sumDr;
    result.totalCreditSum = sumCr;
    return result;
}

} // namespace tally::services
