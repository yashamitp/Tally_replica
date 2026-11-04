#include "DateDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QKeyEvent>

namespace tally::ui {

DateDialog::DateDialog(
    const std::string& currentDate,
    const std::string& fromDate,
    const std::string& toDate,
    QWidget* parent
) : QDialog(parent),
    m_currentDate(currentDate),
    m_fromDate(fromDate),
    m_toDate(toDate) {
    setWindowTitle("Change Date / Period (F2)");
    setFixedWidth(380);

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    QLabel* header = new QLabel("Accounting Period & Voucher Date", this);
    header->setStyleSheet("font-weight: bold; color: #1B4977; font-size: 11pt;");
    rootLayout->addWidget(header);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);

    m_currentDateEdit = new QLineEdit(QString::fromStdString(currentDate), this);
    m_currentDateEdit->setPlaceholderText("YYYY-MM-DD");
    form->addRow("Voucher Date (F2):", m_currentDateEdit);

    m_fromDateEdit = new QLineEdit(QString::fromStdString(fromDate), this);
    m_fromDateEdit->setPlaceholderText("YYYY-MM-DD");
    form->addRow("Period From (Alt+F2):", m_fromDateEdit);

    m_toDateEdit = new QLineEdit(QString::fromStdString(toDate), this);
    m_toDateEdit->setPlaceholderText("YYYY-MM-DD");
    form->addRow("Period To:", m_toDateEdit);

    rootLayout->addLayout(form);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton* okBtn = new QPushButton("Apply (Enter)", this);
    okBtn->setStyleSheet("background-color: #1B4977; color: white; font-weight: bold; padding: 4px 14px;");
    connect(okBtn, &QPushButton::clicked, this, &DateDialog::onApply);
    btnLayout->addWidget(okBtn);

    rootLayout->addLayout(btnLayout);

    m_currentDateEdit->setFocus();
}

void DateDialog::onApply() {
    m_currentDate = m_currentDateEdit->text().trimmed().toStdString();
    m_fromDate = m_fromDateEdit->text().trimmed().toStdString();
    m_toDate = m_toDateEdit->text().trimmed().toStdString();

    if (m_currentDate.empty() || m_fromDate.empty() || m_toDate.empty()) {
        QMessageBox::warning(this, "Validation", "Date fields cannot be empty.");
        return;
    }

    accept();
}

} // namespace tally::ui
