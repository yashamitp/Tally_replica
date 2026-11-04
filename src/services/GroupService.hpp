#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../domain/groups/AccountingGroup.hpp"
#include "../database/DatabaseManager.hpp"

namespace tally::services {

struct CreateGroupParams {
    std::string companyId;
    std::string name;
    std::optional<std::string> parentId;
    domain::GroupNature nature = domain::GroupNature::Asset;
    bool affectsGrossProfit = false;
    bool isPrimary = false;
};

class GroupService {
public:
    explicit GroupService(database::DatabaseManager& dbManager);

    domain::AccountingGroup createGroup(const CreateGroupParams& params);
    bool alterGroup(const domain::AccountingGroup& group);
    std::optional<domain::AccountingGroup> getGroup(const std::string& id);
    std::optional<domain::AccountingGroup> getGroupByName(const std::string& companyId, const std::string& name);
    std::vector<domain::AccountingGroup> listGroups(const std::string& companyId);
    bool deleteGroup(const std::string& id);

private:
    database::DatabaseManager& m_dbManager;
};

} // namespace tally::services
