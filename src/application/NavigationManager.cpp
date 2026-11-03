#include "NavigationManager.hpp"

namespace tally::application {

NavigationManager::NavigationManager(QObject* parent)
    : QObject(parent) {}

void NavigationManager::navigateTo(ScreenId screen, const std::string& param) {
    if (m_currentScreen == screen && m_currentParam == param) {
        return;
    }
    m_history.push_back({m_currentScreen, m_currentParam});
    m_currentScreen = screen;
    m_currentParam = param;
    emit screenChanged(m_currentScreen, m_currentParam);
}

bool NavigationManager::navigateBack() {
    if (!m_history.empty()) {
        auto prev = m_history.back();
        m_history.pop_back();
        m_currentScreen = prev.screen;
        m_currentParam = prev.param;
        emit screenChanged(m_currentScreen, m_currentParam);
        return true;
    } else if (m_currentScreen != ScreenId::Gateway) {
        m_currentScreen = ScreenId::Gateway;
        m_currentParam.clear();
        emit screenChanged(m_currentScreen, m_currentParam);
        return true;
    }
    return false;
}

ScreenId NavigationManager::currentScreen() const {
    return m_currentScreen;
}

std::string NavigationManager::currentParam() const {
    return m_currentParam;
}

} // namespace tally::application
