#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <string>

namespace tally::ui {

class DateDialog : public QDialog {
    Q_OBJECT

public:
    explicit DateDialog(
        const std::string& currentDate,
        const std::string& fromDate,
        const std::string& toDate,
        QWidget* parent = nullptr
    );

    std::string currentDate() const { return m_currentDate; }
    std::string fromDate() const { return m_fromDate; }
    std::string toDate() const { return m_toDate; }

private slots:
    void onApply();

private:
    std::string m_currentDate;
    std::string m_fromDate;
    std::string m_toDate;

    QLineEdit* m_currentDateEdit = nullptr;
    QLineEdit* m_fromDateEdit = nullptr;
    QLineEdit* m_toDateEdit = nullptr;
};

} // namespace tally::ui
