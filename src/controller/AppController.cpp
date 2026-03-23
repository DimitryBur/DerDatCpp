#include <string>

// Состояния согласно твоему паспорту дизайна
enum class AppPage {
    IMPORT,
    CLEAN,
    MODELING,
    PLOT
};

class AppController {
public:
    // По умолчанию открываем ИМПОРТ
    AppController() : currentPage(AppPage::IMPORT) {}

    // Метод для смены страницы (вызывается кнопками в Footer)
    void SetPage(AppPage page) {
        currentPage = page;
    }

    // Метод для получения текущей страницы (нужен для отрисовки в Workspace)
    AppPage GetCurrentPage() const {
        return currentPage;
    }

    // Вспомогательный метод для получения заголовка текущей зоны
    std::string GetPageTitle() const {
        switch (currentPage) {
            case AppPage::IMPORT:   return "Окно: ИМПОРТ (Файловый менеджер)";
            case AppPage::CLEAN:    return "Окно: CLEAN (Санитарная зона)";
            case AppPage::MODELING: return "Окно: MODELING (SQL-Консоль)";
            case AppPage::PLOT:     return "Окно: PLOT (Визуализация)";
            default:                return "Unknown";
        }
    }

private:
    AppPage currentPage;
};
