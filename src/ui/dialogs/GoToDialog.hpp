#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include "../../application/NavigationManager.hpp"

namespace tally::ui {

struct GoToItem {
    QString label;
    application::ScreenId screen;
    QString group;
};

class GoToDialog : public QDialog {
    Q_OBJECT

public:
    explicit GoToDialog(application::NavigationManager& navManager, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onFilterChanged(const QString& text);
    void onItemSelected(QListWidgetItem* item);

private:
    application::NavigationManager& m_navManager;
    QLineEdit* m_searchEdit;
    QListWidget* m_listWidget;
    std::vector<GoToItem> m_allItems;
};

} // namespace tally::ui
