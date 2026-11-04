#include "MasterSelectionDialog.hpp"
#include "GroupDialog.hpp"
#include "LedgerDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMessageBox>

namespace tally::ui {

MasterSelectionDialog::MasterSelectionDialog(
    MasterMode mode,
    const std::string& companyId,
    services::CompanyService& companyService,
    services::GroupService& groupService,
    services::LedgerService& ledgerService,
    QWidget* parent
) : QDialog(parent),
    m_mode(mode),
    m_companyId(companyId),
    m_companyService(companyService),
    m_groupService(groupService),
    m_ledgerService(ledgerService) {
    setWindowTitle(mode == MasterMode::Create ? "Master Creation" : "Master Alteration");
    setFixedSize(460, 480);
    setupUi();
}

void MasterSelectionDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(10);

    m_titleLabel = new QLabel(m_mode == MasterMode::Create ? "Select Master Type to Create:" : "Select Master Type to Alter:", this);
    m_titleLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #1B4977;");
    rootLayout->addWidget(m_titleLabel);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Type to filter...");
    m_searchEdit->setVisible(false);
    rootLayout->addWidget(m_searchEdit);

    m_typeListWidget = new QListWidget(this);
    m_typeListWidget->addItem("Group (Accounting Group)");
    m_typeListWidget->addItem("Ledger (Account Head)");
    m_typeListWidget->setCurrentRow(0);
    rootLayout->addWidget(m_typeListWidget);

    m_itemsListWidget = new QListWidget(this);
    m_itemsListWidget->setVisible(false);
    rootLayout->addWidget(m_itemsListWidget);

    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_backBtn = new QPushButton("Back", this);
    m_backBtn->setVisible(false);
    connect(m_backBtn, &QPushButton::clicked, [this]() {
        m_selectingSpecificItem = false;
        m_itemsListWidget->setVisible(false);
        m_searchEdit->setVisible(false);
        m_typeListWidget->setVisible(true);
        m_backBtn->setVisible(false);
        m_titleLabel->setText(m_mode == MasterMode::Create ? "Select Master Type to Create:" : "Select Master Type to Alter:");
        m_actionBtn->setText("Select (Enter)");
    });
    btnLayout->addWidget(m_backBtn);

    btnLayout->addStretch();

    QPushButton* closeBtn = new QPushButton("Cancel (Esc)", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(closeBtn);

    m_actionBtn = new QPushButton("Select (Enter)", this);
    m_actionBtn->setStyleSheet("background-color: #1B4977; color: white; font-weight: bold; padding: 6px 16px;");
    connect(m_actionBtn, &QPushButton::clicked, this, &MasterSelectionDialog::onMasterTypeSelected);
    btnLayout->addWidget(m_actionBtn);

    rootLayout->addLayout(btnLayout);

    connect(m_typeListWidget, &QListWidget::itemDoubleClicked, this, &MasterSelectionDialog::onMasterTypeSelected);
    connect(m_itemsListWidget, &QListWidget::itemDoubleClicked, this, &MasterSelectionDialog::onSpecificItemSelected);

    connect(m_searchEdit, &QLineEdit::textChanged, [this](const QString& filter) {
        for (int i = 0; i < m_itemsListWidget->count(); ++i) {
            auto* item = m_itemsListWidget->item(i);
            bool match = item->text().contains(filter, Qt::CaseInsensitive);
            item->setHidden(!match);
        }
    });
}

void MasterSelectionDialog::onMasterTypeSelected() {
    if (m_selectingSpecificItem) {
        onSpecificItemSelected();
        return;
    }

    int row = m_typeListWidget->currentRow();
    if (row < 0) return;

    if (row == 0) {
        // Group
        if (m_mode == MasterMode::Create) {
            GroupDialog dlg(m_companyId, m_groupService, this);
            if (dlg.exec() == QDialog::Accepted) {
                accept();
            }
        } else {
            showGroupSelection();
        }
    } else if (row == 1) {
        // Ledger
        if (m_mode == MasterMode::Create) {
            LedgerDialog dlg(m_companyId, m_companyService, m_ledgerService, this);
            if (dlg.exec() == QDialog::Accepted) {
                accept();
            }
        } else {
            showLedgerSelection();
        }
    }
}

void MasterSelectionDialog::showGroupSelection() {
    m_selectingSpecificItem = true;
    m_selectedMasterType = "Group";
    m_typeListWidget->setVisible(false);
    m_itemsListWidget->setVisible(true);
    m_searchEdit->setVisible(true);
    m_searchEdit->clear();
    m_backBtn->setVisible(true);
    m_titleLabel->setText("Select Group to Alter:");
    m_actionBtn->setText("Alter Group (Enter)");

    m_itemsListWidget->clear();
    auto groups = m_groupService.listGroups(m_companyId);
    for (const auto& g : groups) {
        QString label = QString::fromStdString(g.name);
        if (g.isSystem) label += " [System]";
        QListWidgetItem* item = new QListWidgetItem(label, m_itemsListWidget);
        item->setData(Qt::UserRole, QString::fromStdString(g.id));
    }
    if (m_itemsListWidget->count() > 0) m_itemsListWidget->setCurrentRow(0);
    m_searchEdit->setFocus();
}

void MasterSelectionDialog::showLedgerSelection() {
    m_selectingSpecificItem = true;
    m_selectedMasterType = "Ledger";
    m_typeListWidget->setVisible(false);
    m_itemsListWidget->setVisible(true);
    m_searchEdit->setVisible(true);
    m_searchEdit->clear();
    m_backBtn->setVisible(true);
    m_titleLabel->setText("Select Ledger to Alter:");
    m_actionBtn->setText("Alter Ledger (Enter)");

    m_itemsListWidget->clear();
    auto ledgers = m_ledgerService.listLedgers(m_companyId);
    for (const auto& l : ledgers) {
        QString label = QString::fromStdString(l.name);
        if (l.isSystem) label += " [System]";
        QListWidgetItem* item = new QListWidgetItem(label, m_itemsListWidget);
        item->setData(Qt::UserRole, QString::fromStdString(l.id));
    }
    if (m_itemsListWidget->count() > 0) m_itemsListWidget->setCurrentRow(0);
    m_searchEdit->setFocus();
}

void MasterSelectionDialog::onSpecificItemSelected() {
    auto* item = m_itemsListWidget->currentItem();
    if (!item) return;

    std::string id = item->data(Qt::UserRole).toString().toStdString();

    if (m_selectedMasterType == "Group") {
        auto gOpt = m_groupService.getGroup(id);
        if (gOpt) {
            GroupDialog dlg(m_companyId, *gOpt, m_groupService, this);
            if (dlg.exec() == QDialog::Accepted) {
                accept();
            }
        }
    } else if (m_selectedMasterType == "Ledger") {
        auto lOpt = m_ledgerService.getLedger(id);
        if (lOpt) {
            LedgerDialog dlg(m_companyId, *lOpt, m_companyService, m_ledgerService, this);
            if (dlg.exec() == QDialog::Accepted) {
                accept();
            }
        }
    }
}

} // namespace tally::ui
