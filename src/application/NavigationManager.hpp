#pragma once

#include <vector>
#include <string>
#include <functional>
#include <QObject>

namespace tally::application {

enum class ScreenId {
    Gateway,
    VoucherEntry,
    DayBook,
    TrialBalance,
    ProfitLoss,
    BalanceSheet,
    LedgerStatement,
    MasterCreation,
    MasterAlteration
};

struct HistoryItem {
    ScreenId screen;
    std::string param; // e.g. ledgerId or voucherId
};

class NavigationManager : public QObject {
    Q_OBJECT

public:
    explicit NavigationManager(QObject* parent = nullptr);

    void navigateTo(ScreenId screen, const std::string& param = "");
    bool navigateBack();
    ScreenId currentScreen() const;
    std::string currentParam() const;

signals:
    void screenChanged(ScreenId newScreen, const std::string& param);

private:
    std::vector<HistoryItem> m_history;
    ScreenId m_currentScreen = ScreenId::Gateway;
    std::string m_currentParam;
};

} // namespace tally::application
