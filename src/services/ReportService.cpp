#include "ReportService.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <map>
#include <set>

namespace tally::services {

ReportService::ReportService(database::DatabaseManager& dbManager)
    : m_dbManager(dbManager) {}

ReportPeriod ReportService::resolvePeriod(
    const std::string& companyId,
    const std::optional<std::string>& fromDate,
    const std::optional<std::string>& toDate
) {
    ReportPeriod p;
    p.startDate = fromDate ? *fromDate : "";
    p.endDate = toDate ? *toDate : "";

    if (p.startDate.empty() || p.endDate.empty()) {
        QSqlQuery fyQuery(m_dbManager.database());
        fyQuery.prepare("SELECT id, start_date, end_date FROM financial_years WHERE company_id = :cid ORDER BY start_date DESC LIMIT 1");
        fyQuery.bindValue(":cid", QString::fromStdString(companyId));
        if (fyQuery.exec() && fyQuery.next()) {
            p.financialYearId = fyQuery.value(0).toString().toStdString();
            if (p.startDate.empty()) p.startDate = fyQuery.value(1).toString().toStdString();
            if (p.endDate.empty()) p.endDate = fyQuery.value(2).toString().toStdString();
        } else {
            if (p.startDate.empty()) p.startDate = "2026-04-01";
            if (p.endDate.empty()) p.endDate = "2027-03-31";
        }
    }
    return p;
}

TrialBalanceReport ReportService::generateTrialBalance(
    const std::string& companyId,
    const std::optional<std::string>& fromDate,
    const std::optional<std::string>& toDate,
    bool includeZeroBalances
) {
    TrialBalanceReport report;

    // Fetch company name
    QSqlQuery cQuery(m_dbManager.database());
    cQuery.prepare("SELECT name FROM companies WHERE id = :cid");
    cQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (cQuery.exec() && cQuery.next()) {
        report.companyName = cQuery.value(0).toString().toStdString();
    }

    report.period = resolvePeriod(companyId, fromDate, toDate);

    // Fetch groups
    struct GroupRec {
        std::string id;
        std::string name;
        domain::GroupNature nature;
        std::optional<std::string> parentId;
    };
    std::vector<GroupRec> groupsList;
    std::map<std::string, GroupRec> groupMap;

    QSqlQuery gQuery(m_dbManager.database());
    gQuery.prepare("SELECT id, name, nature, parent_id FROM accounting_groups WHERE company_id = :cid ORDER BY name");
    gQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (gQuery.exec()) {
        while (gQuery.next()) {
            GroupRec g;
            g.id = gQuery.value(0).toString().toStdString();
            g.name = gQuery.value(1).toString().toStdString();
            g.nature = domain::groupNatureFromString(gQuery.value(2).toString().toStdString());
            if (!gQuery.value(3).isNull()) g.parentId = gQuery.value(3).toString().toStdString();
            groupsList.push_back(g);
            groupMap[g.id] = g;
        }
    }

    // Fetch Ledgers
    struct LedgerRec {
        std::string id;
        std::string groupId;
        std::string name;
        domain::ExactDecimal openingBalance;
        domain::BalanceType openingType;
    };
    std::vector<LedgerRec> ledgersList;
    std::vector<std::string> ledgerIds;

    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare("SELECT id, group_id, name, opening_balance, opening_balance_type FROM ledgers WHERE company_id = :cid ORDER BY name");
    lQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (lQuery.exec()) {
        while (lQuery.next()) {
            LedgerRec l;
            l.id = lQuery.value(0).toString().toStdString();
            l.groupId = lQuery.value(1).toString().toStdString();
            l.name = lQuery.value(2).toString().toStdString();
            l.openingBalance = domain::ExactDecimal::fromString(lQuery.value(3).toString().toStdString());
            l.openingType = domain::balanceTypeFromString(lQuery.value(4).toString().toStdString());
            ledgersList.push_back(l);
            ledgerIds.push_back(l.id);
        }
    }

    if (ledgersList.empty()) {
        report.isReconciled = true;
        return report;
    }

    // Pre-period movements (< report.period.startDate, is_cancelled = 0)
    std::map<std::string, std::pair<domain::ExactDecimal, domain::ExactDecimal>> preMap; // ledgerId -> {dr, cr}
    QSqlQuery preQuery(m_dbManager.database());
    preQuery.prepare(R"(
        SELECT ve.ledger_id, ve.entry_type, ve.amount
        FROM voucher_entries ve
        JOIN vouchers v ON ve.voucher_id = v.id
        WHERE v.company_id = :cid AND v.is_cancelled = 0 AND v.date < :start_date
    )");
    preQuery.bindValue(":cid", QString::fromStdString(companyId));
    preQuery.bindValue(":start_date", QString::fromStdString(report.period.startDate));
    if (preQuery.exec()) {
        while (preQuery.next()) {
            std::string lid = preQuery.value(0).toString().toStdString();
            std::string etype = preQuery.value(1).toString().toStdString();
            domain::ExactDecimal amt = domain::ExactDecimal::fromString(preQuery.value(2).toString().toStdString());
            if (etype == "DR") preMap[lid].first = preMap[lid].first.add(amt);
            else preMap[lid].second = preMap[lid].second.add(amt);
        }
    }

    // In-period movements (startDate <= date <= endDate, is_cancelled = 0)
    std::map<std::string, std::pair<domain::ExactDecimal, domain::ExactDecimal>> inMap; // ledgerId -> {dr, cr}
    QSqlQuery inQuery(m_dbManager.database());
    inQuery.prepare(R"(
        SELECT ve.ledger_id, ve.entry_type, ve.amount
        FROM voucher_entries ve
        JOIN vouchers v ON ve.voucher_id = v.id
        WHERE v.company_id = :cid AND v.is_cancelled = 0
          AND v.date >= :start_date AND v.date <= :end_date
    )");
    inQuery.bindValue(":cid", QString::fromStdString(companyId));
    inQuery.bindValue(":start_date", QString::fromStdString(report.period.startDate));
    inQuery.bindValue(":end_date", QString::fromStdString(report.period.endDate));
    if (inQuery.exec()) {
        while (inQuery.next()) {
            std::string lid = inQuery.value(0).toString().toStdString();
            std::string etype = inQuery.value(1).toString().toStdString();
            domain::ExactDecimal amt = domain::ExactDecimal::fromString(inQuery.value(2).toString().toStdString());
            if (etype == "DR") inMap[lid].first = inMap[lid].first.add(amt);
            else inMap[lid].second = inMap[lid].second.add(amt);
        }
    }

    // Process leaf ledgers
    std::map<std::string, std::vector<TrialBalanceLedgerRow>> ledgersByGroup;
    domain::ExactDecimal totOpenDr = domain::ExactDecimal::zero();
    domain::ExactDecimal totOpenCr = domain::ExactDecimal::zero();
    domain::ExactDecimal totPerDr = domain::ExactDecimal::zero();
    domain::ExactDecimal totPerCr = domain::ExactDecimal::zero();
    domain::ExactDecimal totCloseDr = domain::ExactDecimal::zero();
    domain::ExactDecimal totCloseCr = domain::ExactDecimal::zero();

    for (const auto& l : ledgersList) {
        auto gIt = groupMap.find(l.groupId);
        std::string grpName = (gIt != groupMap.end()) ? gIt->second.name : "Unassigned";
        domain::GroupNature grpNature = (gIt != groupMap.end()) ? gIt->second.nature : domain::GroupNature::Asset;

        // Opening signed (DR is +, CR is -)
        domain::ExactDecimal initSigned = (l.openingType == domain::BalanceType::Dr) ? l.openingBalance : l.openingBalance.neg();
        auto preMv = preMap[l.id];
        domain::ExactDecimal periodOpeningSigned = initSigned.add(preMv.first).sub(preMv.second);

        domain::ExactDecimal openDr = periodOpeningSigned.isPositive() ? periodOpeningSigned : domain::ExactDecimal::zero();
        domain::ExactDecimal openCr = periodOpeningSigned.isNegative() ? periodOpeningSigned.abs() : domain::ExactDecimal::zero();

        // In-period
        auto inMv = inMap[l.id];
        domain::ExactDecimal perDr = inMv.first;
        domain::ExactDecimal perCr = inMv.second;

        // Closing
        domain::ExactDecimal closingSigned = periodOpeningSigned.add(perDr).sub(perCr);
        domain::ExactDecimal closeDr = closingSigned.isPositive() ? closingSigned : domain::ExactDecimal::zero();
        domain::ExactDecimal closeCr = closingSigned.isNegative() ? closingSigned.abs() : domain::ExactDecimal::zero();
        domain::BalanceType closeSide = closingSigned.isNegative() ? domain::BalanceType::Cr : domain::BalanceType::Dr;

        bool isZero = periodOpeningSigned.isZero() && perDr.isZero() && perCr.isZero() && closingSigned.isZero();
        if (!includeZeroBalances && isZero) {
            continue;
        }

        TrialBalanceLedgerRow row;
        row.ledgerId = l.id;
        row.ledgerName = l.name;
        row.groupId = l.groupId;
        row.groupName = grpName;
        row.groupNature = grpNature;
        row.openingDebit = openDr;
        row.openingCredit = openCr;
        row.periodDebit = perDr;
        row.periodCredit = perCr;
        row.closingDebit = closeDr;
        row.closingCredit = closeCr;
        row.closingBalance = closingSigned.abs();
        row.closingSide = closeSide;

        ledgersByGroup[l.groupId].push_back(row);

        // Accumulate grand totals across leaf ledgers directly!
        totOpenDr = totOpenDr.add(openDr);
        totOpenCr = totOpenCr.add(openCr);
        totPerDr = totPerDr.add(perDr);
        totPerCr = totPerCr.add(perCr);
        totCloseDr = totCloseDr.add(closeDr);
        totCloseCr = totCloseCr.add(closeCr);
    }

    // Build hierarchical tree
    std::map<std::string, TrialBalanceGroupNode> nodeMap;
    for (const auto& g : groupsList) {
        TrialBalanceGroupNode node;
        node.groupId = g.id;
        node.groupName = g.name;
        node.groupNature = g.nature;
        node.parentId = g.parentId;
        node.ledgers = ledgersByGroup[g.id];
        nodeMap[g.id] = node;
    }

    std::vector<TrialBalanceGroupNode> rootNodes;
    for (const auto& g : groupsList) {
        if (g.parentId && nodeMap.find(*g.parentId) != nodeMap.end()) {
            nodeMap[*g.parentId].subGroups.push_back(nodeMap[g.id]);
        } else {
            rootNodes.push_back(nodeMap[g.id]);
        }
    }

    // Rollup helper
    auto rollupNode = [](auto& self, TrialBalanceGroupNode& n) -> void {
        domain::ExactDecimal cDr = domain::ExactDecimal::zero();
        domain::ExactDecimal cCr = domain::ExactDecimal::zero();

        for (const auto& l : n.ledgers) {
            cDr = cDr.add(l.closingDebit);
            cCr = cCr.add(l.closingCredit);
        }
        for (auto& sg : n.subGroups) {
            self(self, sg);
            cDr = cDr.add(sg.closingDebit);
            cCr = cCr.add(sg.closingCredit);
        }
        n.closingDebit = cDr;
        n.closingCredit = cCr;
        if (cDr >= cCr) {
            n.netClosingBalance = cDr.sub(cCr);
            n.closingSide = domain::BalanceType::Dr;
        } else {
            n.netClosingBalance = cCr.sub(cDr);
            n.closingSide = domain::BalanceType::Cr;
        }
    };

    for (auto& r : rootNodes) {
        rollupNode(rollupNode, r);
    }

    report.groups = std::move(rootNodes);
    report.totalOpeningDebit = totOpenDr;
    report.totalOpeningCredit = totOpenCr;
    report.totalPeriodDebit = totPerDr;
    report.totalPeriodCredit = totPerCr;
    report.totalClosingDebit = totCloseDr;
    report.totalClosingCredit = totCloseCr;
    report.discrepancy = (totCloseDr.sub(totCloseCr)).abs();
    report.isReconciled = report.discrepancy.isZero();

    return report;
}

ProfitAndLossReport ReportService::generateProfitAndLoss(
    const std::string& companyId,
    const std::optional<std::string>& fromDate,
    const std::optional<std::string>& toDate
) {
    ProfitAndLossReport report;
    report.period = resolvePeriod(companyId, fromDate, toDate);

    // Fetch company name
    QSqlQuery cQuery(m_dbManager.database());
    cQuery.prepare("SELECT name FROM companies WHERE id = :cid");
    cQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (cQuery.exec() && cQuery.next()) {
        report.companyName = cQuery.value(0).toString().toStdString();
    }

    // Fetch Income & Expense groups
    struct GroupDef {
        std::string id;
        std::string name;
        domain::GroupNature nature;
        bool affectsGrossProfit;
        std::optional<std::string> parentId;
    };
    std::vector<GroupDef> pnlGroups;
    std::map<std::string, GroupDef> groupMap;

    QSqlQuery gQuery(m_dbManager.database());
    gQuery.prepare("SELECT id, name, nature, affects_gross_profit, parent_id FROM accounting_groups WHERE company_id = :cid AND nature IN ('INCOME', 'EXPENSE') ORDER BY name");
    gQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (gQuery.exec()) {
        while (gQuery.next()) {
            GroupDef g;
            g.id = gQuery.value(0).toString().toStdString();
            g.name = gQuery.value(1).toString().toStdString();
            g.nature = domain::groupNatureFromString(gQuery.value(2).toString().toStdString());
            g.affectsGrossProfit = gQuery.value(3).toInt() != 0;
            if (!gQuery.value(4).isNull()) g.parentId = gQuery.value(4).toString().toStdString();
            pnlGroups.push_back(g);
            groupMap[g.id] = g;
        }
    }

    // Fetch ledgers under these groups
    struct LedgerDef {
        std::string id;
        std::string groupId;
        std::string name;
    };
    std::vector<LedgerDef> pnlLedgers;
    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare(R"(
        SELECT l.id, l.group_id, l.name
        FROM ledgers l
        JOIN accounting_groups g ON l.group_id = g.id
        WHERE l.company_id = :cid AND g.nature IN ('INCOME', 'EXPENSE')
        ORDER BY l.name
    )");
    lQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (lQuery.exec()) {
        while (lQuery.next()) {
            LedgerDef l;
            l.id = lQuery.value(0).toString().toStdString();
            l.groupId = lQuery.value(1).toString().toStdString();
            l.name = lQuery.value(2).toString().toStdString();
            pnlLedgers.push_back(l);
        }
    }

    // Aggregate movements in period
    std::map<std::string, std::pair<domain::ExactDecimal, domain::ExactDecimal>> mvMap;
    QSqlQuery mvQuery(m_dbManager.database());
    mvQuery.prepare(R"(
        SELECT ve.ledger_id, ve.entry_type, ve.amount
        FROM voucher_entries ve
        JOIN vouchers v ON ve.voucher_id = v.id
        JOIN ledgers l ON ve.ledger_id = l.id
        JOIN accounting_groups g ON l.group_id = g.id
        WHERE v.company_id = :cid AND v.is_cancelled = 0
          AND v.date >= :start_date AND v.date <= :end_date
          AND g.nature IN ('INCOME', 'EXPENSE')
    )");
    mvQuery.bindValue(":cid", QString::fromStdString(companyId));
    mvQuery.bindValue(":start_date", QString::fromStdString(report.period.startDate));
    mvQuery.bindValue(":end_date", QString::fromStdString(report.period.endDate));
    if (mvQuery.exec()) {
        while (mvQuery.next()) {
            std::string lid = mvQuery.value(0).toString().toStdString();
            std::string etype = mvQuery.value(1).toString().toStdString();
            domain::ExactDecimal amt = domain::ExactDecimal::fromString(mvQuery.value(2).toString().toStdString());
            if (etype == "DR") mvMap[lid].first = mvMap[lid].first.add(amt);
            else mvMap[lid].second = mvMap[lid].second.add(amt);
        }
    }

    // Map ledger details:
    // INCOME: CR - DR
    // EXPENSE: DR - CR
    std::map<std::string, std::vector<PnLLedgerDetail>> ledgersByGroup;
    for (const auto& l : pnlLedgers) {
        auto gIt = groupMap.find(l.groupId);
        if (gIt == groupMap.end()) continue;

        auto mv = mvMap[l.id];
        domain::ExactDecimal netAmt;
        if (gIt->second.nature == domain::GroupNature::Income) {
            netAmt = mv.second.sub(mv.first);
        } else {
            netAmt = mv.first.sub(mv.second);
        }

        PnLLedgerDetail d;
        d.ledgerId = l.id;
        d.ledgerName = l.name;
        d.amount = netAmt;
        ledgersByGroup[l.groupId].push_back(d);
    }

    auto buildSections = [&](const std::vector<GroupDef>& groups) -> std::vector<PnLGroupSection> {
        std::map<std::string, PnLGroupSection> sMap;
        for (const auto& g : groups) {
            PnLGroupSection s;
            s.groupId = g.id;
            s.groupName = g.name;
            s.ledgers = ledgersByGroup[g.id];
            sMap[g.id] = s;
        }

        std::vector<PnLGroupSection> roots;
        for (const auto& g : groups) {
            if (g.parentId && sMap.find(*g.parentId) != sMap.end()) {
                sMap[*g.parentId].subGroups.push_back(sMap[g.id]);
            } else {
                roots.push_back(sMap[g.id]);
            }
        }

        auto calcTotal = [](auto& self, PnLGroupSection& sec) -> void {
            domain::ExactDecimal tot = domain::ExactDecimal::zero();
            for (const auto& l : sec.ledgers) tot = tot.add(l.amount);
            for (auto& sg : sec.subGroups) {
                self(self, sg);
                tot = tot.add(sg.totalAmount);
            }
            sec.totalAmount = tot;
        };

        for (auto& r : roots) calcTotal(calcTotal, r);
        return roots;
    };

    auto sumSections = [](const std::vector<PnLGroupSection>& secs) -> domain::ExactDecimal {
        domain::ExactDecimal sum = domain::ExactDecimal::zero();
        for (const auto& s : secs) sum = sum.add(s.totalAmount);
        return sum;
    };

    // 1. Direct Income
    std::vector<GroupDef> dirIncGroups;
    for (const auto& g : pnlGroups) if (g.nature == domain::GroupNature::Income && g.affectsGrossProfit) dirIncGroups.push_back(g);
    report.tradingAccount.directIncomeSections = buildSections(dirIncGroups);
    report.tradingAccount.directIncomeTotal = sumSections(report.tradingAccount.directIncomeSections);

    // 2. Direct Expense
    std::vector<GroupDef> dirExpGroups;
    for (const auto& g : pnlGroups) if (g.nature == domain::GroupNature::Expense && g.affectsGrossProfit) dirExpGroups.push_back(g);
    report.tradingAccount.directExpenseSections = buildSections(dirExpGroups);
    report.tradingAccount.directExpensesTotal = sumSections(report.tradingAccount.directExpenseSections);

    // Gross Profit = Direct Income - Direct Expense
    domain::ExactDecimal gpVal = report.tradingAccount.directIncomeTotal.sub(report.tradingAccount.directExpensesTotal);
    report.tradingAccount.isGrossProfit = !gpVal.isNegative();
    if (report.tradingAccount.isGrossProfit) {
        report.tradingAccount.grossProfit = gpVal;
        report.tradingAccount.grossLoss = domain::ExactDecimal::zero();
    } else {
        report.tradingAccount.grossProfit = domain::ExactDecimal::zero();
        report.tradingAccount.grossLoss = gpVal.abs();
    }

    // 3. Indirect Income
    std::vector<GroupDef> indirIncGroups;
    for (const auto& g : pnlGroups) if (g.nature == domain::GroupNature::Income && !g.affectsGrossProfit) indirIncGroups.push_back(g);
    report.operatingAccount.indirectIncomeSections = buildSections(indirIncGroups);
    report.operatingAccount.indirectIncomeTotal = sumSections(report.operatingAccount.indirectIncomeSections);

    // 4. Indirect Expense
    std::vector<GroupDef> indirExpGroups;
    for (const auto& g : pnlGroups) if (g.nature == domain::GroupNature::Expense && !g.affectsGrossProfit) indirExpGroups.push_back(g);
    report.operatingAccount.indirectExpenseSections = buildSections(indirExpGroups);
    report.operatingAccount.indirectExpensesTotal = sumSections(report.operatingAccount.indirectExpenseSections);

    // Net Profit = Gross Profit + Indirect Income - Indirect Expense
    domain::ExactDecimal npVal = gpVal.add(report.operatingAccount.indirectIncomeTotal).sub(report.operatingAccount.indirectExpensesTotal);
    report.operatingAccount.isNetProfit = !npVal.isNegative();
    if (report.operatingAccount.isNetProfit) {
        report.operatingAccount.netProfit = npVal;
        report.operatingAccount.netLoss = domain::ExactDecimal::zero();
    } else {
        report.operatingAccount.netProfit = domain::ExactDecimal::zero();
        report.operatingAccount.netLoss = npVal.abs();
    }

    return report;
}

BalanceSheetReport ReportService::generateBalanceSheet(
    const std::string& companyId,
    const std::optional<std::string>& asOfDate
) {
    BalanceSheetReport report;
    report.period = resolvePeriod(companyId, std::nullopt, asOfDate);
    report.asOfDate = report.period.endDate;

    // Fetch company name
    QSqlQuery cQuery(m_dbManager.database());
    cQuery.prepare("SELECT name FROM companies WHERE id = :cid");
    cQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (cQuery.exec() && cQuery.next()) {
        report.companyName = cQuery.value(0).toString().toStdString();
    }

    // Fetch BS Groups (ASSET, LIABILITY, EQUITY)
    struct BSGroupDef {
        std::string id;
        std::string name;
        domain::GroupNature nature;
        std::optional<std::string> parentId;
    };
    std::vector<BSGroupDef> bsGroups;
    std::map<std::string, BSGroupDef> groupMap;

    QSqlQuery gQuery(m_dbManager.database());
    gQuery.prepare("SELECT id, name, nature, parent_id FROM accounting_groups WHERE company_id = :cid AND nature IN ('ASSET', 'LIABILITY', 'EQUITY') ORDER BY name");
    gQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (gQuery.exec()) {
        while (gQuery.next()) {
            BSGroupDef g;
            g.id = gQuery.value(0).toString().toStdString();
            g.name = gQuery.value(1).toString().toStdString();
            g.nature = domain::groupNatureFromString(gQuery.value(2).toString().toStdString());
            if (!gQuery.value(3).isNull()) g.parentId = gQuery.value(3).toString().toStdString();
            bsGroups.push_back(g);
            groupMap[g.id] = g;
        }
    }

    // Fetch BS Ledgers
    struct BSLedgerDef {
        std::string id;
        std::string groupId;
        std::string name;
        domain::ExactDecimal openingBalance;
        domain::BalanceType openingType;
    };
    std::vector<BSLedgerDef> bsLedgers;
    QSqlQuery lQuery(m_dbManager.database());
    lQuery.prepare(R"(
        SELECT l.id, l.group_id, l.name, l.opening_balance, l.opening_balance_type
        FROM ledgers l
        JOIN accounting_groups g ON l.group_id = g.id
        WHERE l.company_id = :cid AND g.nature IN ('ASSET', 'LIABILITY', 'EQUITY')
        ORDER BY l.name
    )");
    lQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (lQuery.exec()) {
        while (lQuery.next()) {
            BSLedgerDef l;
            l.id = lQuery.value(0).toString().toStdString();
            l.groupId = lQuery.value(1).toString().toStdString();
            l.name = lQuery.value(2).toString().toStdString();
            l.openingBalance = domain::ExactDecimal::fromString(lQuery.value(3).toString().toStdString());
            l.openingType = domain::balanceTypeFromString(lQuery.value(4).toString().toStdString());
            bsLedgers.push_back(l);
        }
    }

    // Aggregate movements up to asOfDate
    std::map<std::string, std::pair<domain::ExactDecimal, domain::ExactDecimal>> mvMap;
    QSqlQuery mvQuery(m_dbManager.database());
    mvQuery.prepare(R"(
        SELECT ve.ledger_id, ve.entry_type, ve.amount
        FROM voucher_entries ve
        JOIN vouchers v ON ve.voucher_id = v.id
        WHERE v.company_id = :cid AND v.is_cancelled = 0 AND v.date <= :as_of_date
    )");
    mvQuery.bindValue(":cid", QString::fromStdString(companyId));
    mvQuery.bindValue(":as_of_date", QString::fromStdString(report.asOfDate));
    if (mvQuery.exec()) {
        while (mvQuery.next()) {
            std::string lid = mvQuery.value(0).toString().toStdString();
            std::string etype = mvQuery.value(1).toString().toStdString();
            domain::ExactDecimal amt = domain::ExactDecimal::fromString(mvQuery.value(2).toString().toStdString());
            if (etype == "DR") mvMap[lid].first = mvMap[lid].first.add(amt);
            else mvMap[lid].second = mvMap[lid].second.add(amt);
        }
    }

    // Net balance per ledger
    std::map<std::string, std::vector<BalanceSheetLedgerDetail>> ledgersByGroup;
    for (const auto& l : bsLedgers) {
        auto gIt = groupMap.find(l.groupId);
        if (gIt == groupMap.end()) continue;

        domain::ExactDecimal initSigned = (l.openingType == domain::BalanceType::Dr) ? l.openingBalance : l.openingBalance.neg();
        auto mv = mvMap[l.id];
        domain::ExactDecimal netSigned = initSigned.add(mv.first).sub(mv.second);

        domain::ExactDecimal presentationAmount;
        domain::BalanceType side;
        if (gIt->second.nature == domain::GroupNature::Asset) {
            presentationAmount = netSigned;
            side = netSigned.isNegative() ? domain::BalanceType::Cr : domain::BalanceType::Dr;
        } else {
            presentationAmount = netSigned.neg();
            side = netSigned.isPositive() ? domain::BalanceType::Dr : domain::BalanceType::Cr;
        }

        BalanceSheetLedgerDetail d;
        d.ledgerId = l.id;
        d.ledgerName = l.name;
        d.amount = presentationAmount.abs();
        d.side = side;
        ledgersByGroup[l.groupId].push_back(d);
    }

    auto buildSections = [&](const std::vector<BSGroupDef>& groups) -> std::vector<BalanceSheetGroupSection> {
        std::function<BalanceSheetGroupSection(const BSGroupDef&)> buildNode = [&](const BSGroupDef& g) {
            BalanceSheetGroupSection sec;
            sec.groupId = g.id;
            sec.groupName = g.name;
            sec.ledgers = ledgersByGroup[g.id];

            domain::ExactDecimal tot = domain::ExactDecimal::zero();
            for (const auto& l : sec.ledgers) {
                tot = tot.add(l.amount);
            }

            for (const auto& child : groups) {
                if (child.parentId && *child.parentId == g.id) {
                    auto childSec = buildNode(child);
                    tot = tot.add(childSec.totalAmount);
                    sec.subGroups.push_back(childSec);
                }
            }
            sec.totalAmount = tot;
            return sec;
        };

        std::vector<BalanceSheetGroupSection> roots;
        for (const auto& g : groups) {
            if (!g.parentId.has_value() || g.parentId->empty()) {
                roots.push_back(buildNode(g));
            }
        }
        return roots;
    };

    auto sumSections = [](const std::vector<BalanceSheetGroupSection>& secs) -> domain::ExactDecimal {
        domain::ExactDecimal sum = domain::ExactDecimal::zero();
        for (const auto& s : secs) sum = sum.add(s.totalAmount);
        return sum;
    };

    // 1. Assets
    std::vector<BSGroupDef> assetGroups;
    for (const auto& g : bsGroups) if (g.nature == domain::GroupNature::Asset) assetGroups.push_back(g);
    report.assets.sections = buildSections(assetGroups);
    report.assets.total = sumSections(report.assets.sections);
    report.totalAssets = report.assets.total;

    // 2. Liabilities
    std::vector<BSGroupDef> liabGroups;
    for (const auto& g : bsGroups) if (g.nature == domain::GroupNature::Liability) liabGroups.push_back(g);
    report.liabilities.sections = buildSections(liabGroups);
    report.liabilities.total = sumSections(report.liabilities.sections);

    // 3. Equity
    std::vector<BSGroupDef> eqGroups;
    for (const auto& g : bsGroups) if (g.nature == domain::GroupNature::Equity) eqGroups.push_back(g);
    report.equity.sections = buildSections(eqGroups);
    report.equity.totalCapital = sumSections(report.equity.sections);

    // Current period profit/loss from P&L
    ProfitAndLossReport pnl = generateProfitAndLoss(companyId, report.period.startDate, report.asOfDate);
    report.equity.isCurrentPeriodProfit = pnl.operatingAccount.isNetProfit;
    report.equity.currentPeriodProfitOrLoss = report.equity.isCurrentPeriodProfit ? pnl.operatingAccount.netProfit : pnl.operatingAccount.netLoss;

    if (report.equity.isCurrentPeriodProfit) {
        report.equity.totalEquity = report.equity.totalCapital.add(report.equity.currentPeriodProfitOrLoss);
    } else {
        report.equity.totalEquity = report.equity.totalCapital.sub(report.equity.currentPeriodProfitOrLoss);
    }

    report.totalLiabilitiesAndEquity = report.liabilities.total.add(report.equity.totalEquity);

    // Check Fundamental Accounting Equation: Assets == Liabilities + Equity
    report.discrepancy = (report.totalAssets.sub(report.totalLiabilitiesAndEquity)).abs();
    report.isReconciled = report.discrepancy.isZero();

    return report;
}

GroupSummaryReport ReportService::generateGroupSummary(
    const std::string& companyId,
    const std::string& groupId,
    const std::optional<std::string>& fromDate,
    const std::optional<std::string>& toDate
) {
    GroupSummaryReport report;
    report.groupId = groupId;
    report.period = resolvePeriod(companyId, fromDate, toDate);

    // Fetch company name
    QSqlQuery cQuery(m_dbManager.database());
    cQuery.prepare("SELECT name FROM companies WHERE id = :cid");
    cQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (cQuery.exec() && cQuery.next()) {
        report.companyName = cQuery.value(0).toString().toStdString();
    }

    // Fetch group details
    domain::GroupNature groupNature = domain::GroupNature::Asset;
    QSqlQuery gQuery(m_dbManager.database());
    gQuery.prepare("SELECT name, nature FROM accounting_groups WHERE id = :gid AND company_id = :cid");
    gQuery.bindValue(":gid", QString::fromStdString(groupId));
    gQuery.bindValue(":cid", QString::fromStdString(companyId));
    if (gQuery.exec() && gQuery.next()) {
        report.groupName = gQuery.value(0).toString().toStdString();
        groupNature = domain::groupNatureFromString(gQuery.value(1).toString().toStdString());
    }

    // Use generateTrialBalance internally to get accurate pre/period/closing numbers across the tree
    auto tb = generateTrialBalance(companyId, report.period.startDate, report.period.endDate, true);

    // Find our group in trial balance tree
    std::function<const TrialBalanceGroupNode*(const std::vector<TrialBalanceGroupNode>&)> findNode;
    findNode = [&](const std::vector<TrialBalanceGroupNode>& nodes) -> const TrialBalanceGroupNode* {
        for (const auto& node : nodes) {
            if (node.groupId == groupId) return &node;
            auto* sub = findNode(node.subGroups);
            if (sub) return sub;
        }
        return nullptr;
    };

    const TrialBalanceGroupNode* targetNode = findNode(tb.groups);
    if (targetNode) {
        // Add sub-groups
        for (const auto& sg : targetNode->subGroups) {
            GroupSummaryItem item;
            item.id = sg.groupId;
            item.name = sg.groupName;
            item.isGroup = true;
            item.openingDebit = sg.openingDebit;
            item.openingCredit = sg.openingCredit;
            item.periodDebit = sg.periodDebit;
            item.periodCredit = sg.periodCredit;
            item.closingDebit = sg.closingDebit;
            item.closingCredit = sg.closingCredit;
            report.items.push_back(item);

            report.totalOpeningDebit = report.totalOpeningDebit.add(sg.openingDebit);
            report.totalOpeningCredit = report.totalOpeningCredit.add(sg.openingCredit);
            report.totalPeriodDebit = report.totalPeriodDebit.add(sg.periodDebit);
            report.totalPeriodCredit = report.totalPeriodCredit.add(sg.periodCredit);
            report.totalClosingDebit = report.totalClosingDebit.add(sg.closingDebit);
            report.totalClosingCredit = report.totalClosingCredit.add(sg.closingCredit);
        }

        // Add ledgers
        for (const auto& l : targetNode->ledgers) {
            GroupSummaryItem item;
            item.id = l.ledgerId;
            item.name = l.ledgerName;
            item.isGroup = false;
            item.openingDebit = l.openingDebit;
            item.openingCredit = l.openingCredit;
            item.periodDebit = l.periodDebit;
            item.periodCredit = l.periodCredit;
            item.closingDebit = l.closingDebit;
            item.closingCredit = l.closingCredit;
            report.items.push_back(item);

            report.totalOpeningDebit = report.totalOpeningDebit.add(l.openingDebit);
            report.totalOpeningCredit = report.totalOpeningCredit.add(l.openingCredit);
            report.totalPeriodDebit = report.totalPeriodDebit.add(l.periodDebit);
            report.totalPeriodCredit = report.totalPeriodCredit.add(l.periodCredit);
            report.totalClosingDebit = report.totalClosingDebit.add(l.closingDebit);
            report.totalClosingCredit = report.totalClosingCredit.add(l.closingCredit);
        }
    }

    return report;
}

} // namespace tally::services
