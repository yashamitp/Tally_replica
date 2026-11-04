#include "GroupSummaryView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QKeyEvent>

namespace tally::ui {

GroupSummaryView::GroupSummaryView(
    application::NavigationManager& navManager,
    services::ReportService& reportService,
    services::GroupService& groupService,
    QWidget* parent
) : QWidget(parent),
    m_navManager(navManager),
    m_reportService(reportService),
    m_groupService(groupService) {
    setupUi();
}

void GroupSummaryView::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 8, 12, 8);
    rootLayout->setSpacing(8);

    // Top Filter Bar
    QFrame* filterFrame = new QFrame(this);
    filterFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* fLayout = new QHBoxLayout(filterFrame);

    m_titleLabel = new QLabel("Group Summary", filterFrame);
    m_titleLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #1B4977;");
    fLayout->addWidget(m_titleLabel);

    fLayout->addSpacing(16);
    fLayout->addWidget(new QLabel("Group:", filterFrame));

    m_groupCombo = new QComboBox(filterFrame);
    m_groupCombo->setMinimumWidth(220);
    connect(m_groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GroupSummaryView::onGroupSelectionChanged);
    fLayout->addWidget(m_groupCombo);

    fLayout->addStretch();

    fLayout->addWidget(new QLabel("From:", filterFrame));
    m_fromDateEdit = new QLineEdit("2026-04-01", filterFrame);
    m_fromDateEdit->setFixedWidth(100);
    fLayout->addWidget(m_fromDateEdit);

    fLayout->addWidget(new QLabel("To:", filterFrame));
    m_toDateEdit = new QLineEdit("2027-03-31", filterFrame);
    m_toDateEdit->setFixedWidth(100);
    fLayout->addWidget(m_toDateEdit);

    QPushButton* refreshBtn = new QPushButton("Refresh", filterFrame);
    refreshBtn->setStyleSheet("background-color: #1B4977; color: #FFFFFF; font-weight: bold; padding: 4px 12px;");
    connect(refreshBtn, &QPushButton::clicked, this, &GroupSummaryView::refreshData);
    fLayout->addWidget(refreshBtn);

    rootLayout->addWidget(filterFrame);

    // Table
    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({
        "Particulars", "Opening Balance", "Debit (₹)", "Credit (₹)", "Closing Balance"
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 140);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 130);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->setColumnWidth(3, 130);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(4, 140);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &GroupSummaryView::onRowDoubleClicked);
    rootLayout->addWidget(m_table);

    // Footer Summary
    QFrame* footFrame = new QFrame(this);
    footFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* footLayout = new QHBoxLayout(footFrame);

    QLabel* note = new QLabel("Double-click row to drill down to subgroup or ledger statement", footFrame);
    note->setStyleSheet("color: #556B82; font-style: italic;");
    footLayout->addWidget(note);

    footLayout->addStretch();

    footLayout->addWidget(new QLabel("Total Debits: ₹ ", footFrame));
    m_totPeriodDrLabel = new QLabel("0.00", footFrame);
    m_totPeriodDrLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totPeriodDrLabel);

    footLayout->addSpacing(20);

    footLayout->addWidget(new QLabel("Total Credits: ₹ ", footFrame));
    m_totPeriodCrLabel = new QLabel("0.00", footFrame);
    m_totPeriodCrLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totPeriodCrLabel);

    rootLayout->addWidget(footFrame);
}

void GroupSummaryView::setCompanyId(const std::string& companyId) {
    m_companyId = companyId;
    loadGroupsCombo();
    refreshData();
}

void GroupSummaryView::setGroupId(const std::string& groupId) {
    m_selectedGroupId = groupId;
    for (int i = 0; i < m_groupCombo->count(); ++i) {
        if (m_groupCombo->itemData(i).toString().toStdString() == groupId) {
            m_groupCombo->setCurrentIndex(i);
            break;
        }
    }
    refreshData();
}

void GroupSummaryView::loadGroupsCombo() {
    m_groupCombo->clear();
    auto list = m_groupService.listGroups(m_companyId);
    for (const auto& g : list) {
        m_groupCombo->addItem(QString::fromStdString(g.name), QString::fromStdString(g.id));
    }
    if (m_groupCombo->count() > 0) {
        m_selectedGroupId = m_groupCombo->itemData(0).toString().toStdString();
    }
}

void GroupSummaryView::onGroupSelectionChanged(int index) {
    if (index >= 0) {
        m_selectedGroupId = m_groupCombo->itemData(index).toString().toStdString();
        refreshData();
    }
}

void GroupSummaryView::refreshData() {
    if (m_companyId.empty() || m_selectedGroupId.empty()) return;

    std::string fromDate = m_fromDateEdit->text().trimmed().toStdString();
    std::string toDate = m_toDateEdit->text().trimmed().toStdString();

    auto report = m_reportService.generateGroupSummary(m_companyId, m_selectedGroupId, fromDate, toDate);

    m_titleLabel->setText(QString("Group Summary: %1").arg(QString::fromStdString(report.groupName)));
    m_table->setRowCount(0);

    for (const auto& item : report.items) {
        int r = m_table->rowCount();
        m_table->insertRow(r);

        QString displayName = QString::fromStdString(item.name);
        if (item.isGroup) displayName = "[Group] " + displayName;

        auto* pItem = new QTableWidgetItem(displayName);
        pItem->setData(Qt::UserRole, QString::fromStdString(item.id));
        pItem->setData(Qt::UserRole + 1, item.isGroup);
        if (item.isGroup) {
            QFont f = pItem->font();
            f.setBold(true);
            pItem->setFont(f);
            pItem->setForeground(QColor("#1B4977"));
        }
        m_table->setItem(r, 0, pItem);

        // Opening
        QString opStr = "";
        if (item.openingDebit > domain::ExactDecimal::zero()) opStr = QString::fromStdString(item.openingDebit.formatInr(false)) + " Dr";
        else if (item.openingCredit > domain::ExactDecimal::zero()) opStr = QString::fromStdString(item.openingCredit.formatInr(false)) + " Cr";
        auto* opItem = new QTableWidgetItem(opStr);
        opItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 1, opItem);

        // Debit
        auto* drItem = new QTableWidgetItem(item.periodDebit > domain::ExactDecimal::zero() ? QString::fromStdString(item.periodDebit.formatInr(false)) : "");
        drItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 2, drItem);

        // Credit
        auto* crItem = new QTableWidgetItem(item.periodCredit > domain::ExactDecimal::zero() ? QString::fromStdString(item.periodCredit.formatInr(false)) : "");
        crItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 3, crItem);

        // Closing
        QString clStr = "";
        if (item.closingDebit > domain::ExactDecimal::zero()) clStr = QString::fromStdString(item.closingDebit.formatInr(false)) + " Dr";
        else if (item.closingCredit > domain::ExactDecimal::zero()) clStr = QString::fromStdString(item.closingCredit.formatInr(false)) + " Cr";
        auto* clItem = new QTableWidgetItem(clStr);
        clItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 4, clItem);
    }

    m_totPeriodDrLabel->setText(QString::fromStdString(report.totalPeriodDebit.formatInr(false)));
    m_totPeriodCrLabel->setText(QString::fromStdString(report.totalPeriodCredit.formatInr(false)));
}

void GroupSummaryView::onRowDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    auto* item = m_table->item(row, 0);
    if (!item) return;

    std::string id = item->data(Qt::UserRole).toString().toStdString();
    bool isGroup = item->data(Qt::UserRole + 1).toBool();

    if (isGroup) {
        setGroupId(id);
    } else {
        m_navManager.navigateTo(application::ScreenId::LedgerStatement, id);
    }
}

void GroupSummaryView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        m_navManager.navigateBack();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        int r = m_table->currentRow();
        if (r >= 0) onRowDoubleClicked(r, 0);
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace tally::ui
