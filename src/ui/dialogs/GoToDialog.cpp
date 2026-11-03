#include "GoToDialog.hpp"
#include <QKeyEvent>
#include <QLabel>

namespace tally::ui {

GoToDialog::GoToDialog(application::NavigationManager& navManager, QWidget* parent)
    : QDialog(parent), m_navManager(navManager) {
    setWindowTitle("Go To - Jump to Screen (Alt+G)");
    setFixedWidth(450);
    setFixedHeight(400);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QLabel* titleLabel = new QLabel("Type to search reports, vouchers, and masters:", this);
    titleLabel->setStyleSheet("font-weight: bold; color: #1B4977;");
    layout->addWidget(titleLabel);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search (e.g. Day Book, Balance Sheet, Voucher Entry)...");
    layout->addWidget(m_searchEdit);

    m_listWidget = new QListWidget(this);
    layout->addWidget(m_listWidget);

    m_allItems = {
        {"Gateway of Tally", application::ScreenId::Gateway, "General"},
        {"Accounting Voucher Creation", application::ScreenId::VoucherEntry, "Transactions"},
        {"Day Book", application::ScreenId::DayBook, "Reports"},
        {"Trial Balance", application::ScreenId::TrialBalance, "Reports"},
        {"Profit & Loss A/c", application::ScreenId::ProfitLoss, "Reports"},
        {"Balance Sheet", application::ScreenId::BalanceSheet, "Reports"},
        {"Ledger Statement / Account Books", application::ScreenId::LedgerStatement, "Reports"}
    };

    onFilterChanged("");

    connect(m_searchEdit, &QLineEdit::textChanged, this, &GoToDialog::onFilterChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &GoToDialog::onItemSelected);

    m_searchEdit->setFocus();
}

void GoToDialog::onFilterChanged(const QString& text) {
    m_listWidget->clear();
    for (const auto& item : m_allItems) {
        if (text.isEmpty() || item.label.contains(text, Qt::CaseInsensitive)) {
            QListWidgetItem* listItem = new QListWidgetItem(item.label, m_listWidget);
            listItem->setData(Qt::UserRole, static_cast<int>(item.screen));
        }
    }
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void GoToDialog::onItemSelected(QListWidgetItem* item) {
    if (!item) return;
    auto screen = static_cast<application::ScreenId>(item->data(Qt::UserRole).toInt());
    m_navManager.navigateTo(screen);
    accept();
}

void GoToDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onItemSelected(m_listWidget->currentItem());
    } else if (event->key() == Qt::Key_Down) {
        int next = m_listWidget->currentRow() + 1;
        if (next < m_listWidget->count()) m_listWidget->setCurrentRow(next);
    } else if (event->key() == Qt::Key_Up) {
        int prev = m_listWidget->currentRow() - 1;
        if (prev >= 0) m_listWidget->setCurrentRow(prev);
    } else {
        QDialog::keyPressEvent(event);
    }
}

} // namespace tally::ui
