#include "GroupService.hpp"
#include <QUuid>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include "../utilities/DateUtils.hpp"
#include "../utilities/Logger.hpp"

namespace tally::services {

GroupService::GroupService(database::DatabaseManager& dbManager)
    : m_dbManager(dbManager) {}

domain::AccountingGroup GroupService::createGroup(const CreateGroupParams& params) {
    if (params.name.empty()) {
        throw std::invalid_argument("Group name is required");
    }

    // Check duplicate
    if (getGroupByName(params.companyId, params.name).has_value()) {
        throw std::invalid_argument("Group with name '" + params.name + "' already exists");
    }

    std::string id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    std::string now = utilities::DateUtils::currentIsoDateTime();

    QSqlQuery q(m_dbManager.database());
    q.prepare(R"(
        INSERT INTO accounting_groups (
            id, company_id, parent_id, name, nature, affects_gross_profit,
            is_primary, is_system, created_at, updated_at
        ) VALUES (
            :id, :company_id, :parent_id, :name, :nature, :affects_gross_profit,
            :is_primary, 0, :created_at, :updated_at
        )
    )");

    q.bindValue(":id", QString::fromStdString(id));
    q.bindValue(":company_id", QString::fromStdString(params.companyId));
    q.bindValue(":parent_id", params.parentId ? QString::fromStdString(*params.parentId) : QVariant(QMetaType(QMetaType::QString)));
    q.bindValue(":name", QString::fromStdString(params.name));
    q.bindValue(":nature", QString::fromStdString(domain::groupNatureToString(params.nature)));
    q.bindValue(":affects_gross_profit", params.affectsGrossProfit ? 1 : 0);
    q.bindValue(":is_primary", params.isPrimary ? 1 : 0);
    q.bindValue(":created_at", QString::fromStdString(now));
    q.bindValue(":updated_at", QString::fromStdString(now));

    if (!q.exec()) {
        throw std::runtime_error("Failed to insert group: " + q.lastError().text().toStdString());
    }

    domain::AccountingGroup g;
    g.id = id;
    g.companyId = params.companyId;
    g.parentId = params.parentId;
    g.name = params.name;
    g.nature = params.nature;
    g.affectsGrossProfit = params.affectsGrossProfit;
    g.isPrimary = params.isPrimary;
    g.isSystem = false;
    g.createdAt = now;
    g.updatedAt = now;
    return g;
}

bool GroupService::alterGroup(const domain::AccountingGroup& group) {
    if (group.name.empty()) {
        throw std::invalid_argument("Group name cannot be empty");
    }

    std::string now = utilities::DateUtils::currentIsoDateTime();
    QSqlQuery q(m_dbManager.database());
    q.prepare(R"(
        UPDATE accounting_groups SET
            name = :name,
            parent_id = :parent_id,
            nature = :nature,
            affects_gross_profit = :affects_gross_profit,
            updated_at = :updated_at
        WHERE id = :id AND company_id = :cid
    )");

    q.bindValue(":name", QString::fromStdString(group.name));
    q.bindValue(":parent_id", group.parentId ? QString::fromStdString(*group.parentId) : QVariant(QMetaType(QMetaType::QString)));
    q.bindValue(":nature", QString::fromStdString(domain::groupNatureToString(group.nature)));
    q.bindValue(":affects_gross_profit", group.affectsGrossProfit ? 1 : 0);
    q.bindValue(":updated_at", QString::fromStdString(now));
    q.bindValue(":id", QString::fromStdString(group.id));
    q.bindValue(":cid", QString::fromStdString(group.companyId));

    return q.exec() && q.numRowsAffected() > 0;
}

std::optional<domain::AccountingGroup> GroupService::getGroup(const std::string& id) {
    QSqlQuery q(m_dbManager.database());
    q.prepare("SELECT id, company_id, parent_id, name, nature, affects_gross_profit, is_primary, is_system, created_at, updated_at FROM accounting_groups WHERE id = :id");
    q.bindValue(":id", QString::fromStdString(id));

    if (q.exec() && q.next()) {
        domain::AccountingGroup g;
        g.id = q.value(0).toString().toStdString();
        g.companyId = q.value(1).toString().toStdString();
        if (!q.value(2).isNull()) g.parentId = q.value(2).toString().toStdString();
        g.name = q.value(3).toString().toStdString();
        g.nature = domain::groupNatureFromString(q.value(4).toString().toStdString());
        g.affectsGrossProfit = q.value(5).toInt() != 0;
        g.isPrimary = q.value(6).toInt() != 0;
        g.isSystem = q.value(7).toInt() != 0;
        g.createdAt = q.value(8).toString().toStdString();
        g.updatedAt = q.value(9).toString().toStdString();
        return g;
    }
    return std::nullopt;
}

std::optional<domain::AccountingGroup> GroupService::getGroupByName(const std::string& companyId, const std::string& name) {
    QSqlQuery q(m_dbManager.database());
    q.prepare("SELECT id, company_id, parent_id, name, nature, affects_gross_profit, is_primary, is_system, created_at, updated_at FROM accounting_groups WHERE company_id = :cid AND LOWER(name) = LOWER(:name)");
    q.bindValue(":cid", QString::fromStdString(companyId));
    q.bindValue(":name", QString::fromStdString(name));

    if (q.exec() && q.next()) {
        domain::AccountingGroup g;
        g.id = q.value(0).toString().toStdString();
        g.companyId = q.value(1).toString().toStdString();
        if (!q.value(2).isNull()) g.parentId = q.value(2).toString().toStdString();
        g.name = q.value(3).toString().toStdString();
        g.nature = domain::groupNatureFromString(q.value(4).toString().toStdString());
        g.affectsGrossProfit = q.value(5).toInt() != 0;
        g.isPrimary = q.value(6).toInt() != 0;
        g.isSystem = q.value(7).toInt() != 0;
        g.createdAt = q.value(8).toString().toStdString();
        g.updatedAt = q.value(9).toString().toStdString();
        return g;
    }
    return std::nullopt;
}

std::vector<domain::AccountingGroup> GroupService::listGroups(const std::string& companyId) {
    std::vector<domain::AccountingGroup> result;
    QSqlQuery q(m_dbManager.database());
    q.prepare("SELECT id, company_id, parent_id, name, nature, affects_gross_profit, is_primary, is_system, created_at, updated_at FROM accounting_groups WHERE company_id = :cid ORDER BY name");
    q.bindValue(":cid", QString::fromStdString(companyId));

    if (q.exec()) {
        while (q.next()) {
            domain::AccountingGroup g;
            g.id = q.value(0).toString().toStdString();
            g.companyId = q.value(1).toString().toStdString();
            if (!q.value(2).isNull()) g.parentId = q.value(2).toString().toStdString();
            g.name = q.value(3).toString().toStdString();
            g.nature = domain::groupNatureFromString(q.value(4).toString().toStdString());
            g.affectsGrossProfit = q.value(5).toInt() != 0;
            g.isPrimary = q.value(6).toInt() != 0;
            g.isSystem = q.value(7).toInt() != 0;
            g.createdAt = q.value(8).toString().toStdString();
            g.updatedAt = q.value(9).toString().toStdString();
            result.push_back(g);
        }
    }
    return result;
}

bool GroupService::deleteGroup(const std::string& id) {
    // Check if system group
    auto g = getGroup(id);
    if (!g.has_value() || g->isSystem) {
        return false;
    }

    // Check if any child groups exist
    QSqlQuery childCheck(m_dbManager.database());
    childCheck.prepare("SELECT COUNT(*) FROM accounting_groups WHERE parent_id = :id");
    childCheck.bindValue(":id", QString::fromStdString(id));
    if (childCheck.exec() && childCheck.next() && childCheck.value(0).toInt() > 0) {
        throw std::runtime_error("Cannot delete group because child groups exist under it");
    }

    // Check if any ledgers exist under this group
    QSqlQuery ledgerCheck(m_dbManager.database());
    ledgerCheck.prepare("SELECT COUNT(*) FROM ledgers WHERE group_id = :id");
    ledgerCheck.bindValue(":id", QString::fromStdString(id));
    if (ledgerCheck.exec() && ledgerCheck.next() && ledgerCheck.value(0).toInt() > 0) {
        throw std::runtime_error("Cannot delete group because ledger accounts exist under it");
    }

    QSqlQuery q(m_dbManager.database());
    q.prepare("DELETE FROM accounting_groups WHERE id = :id");
    q.bindValue(":id", QString::fromStdString(id));
    return q.exec() && q.numRowsAffected() > 0;
}

} // namespace tally::services
