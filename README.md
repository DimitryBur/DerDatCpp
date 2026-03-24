# 📊 DerDatCpp — C++ Data Analysis Combine

A system for complex data analytics managed by simple **"Button + List"** pairs. Professional-grade power wrapped in an intuitive interface.

## 🚀 Key Features (Design Passport)

The application is divided into 4 workspaces, accessible via Footer navigation:

1.  **IMPORT (File Manager)**
    *   CSV loading with automatic delimiter detection.
    *   Vertical merging (**UNION**) and horizontal joining (**JOIN**) via keys.
    *   Fast export of processed tables back to CSV.
2.  **CLEAN (Sanitation Zone)**
    *   **Visuals:** Table preview with active column highlighting (blue focus).
    *   **Parsing:** Numeric extraction from text using RegEx.
    *   **Smart Fill:** Intelligent NULL filling (cross-column matching + linear interpolation).
    *   **Outliers:** Symmetric outlier removal (quantiles: 5%, 10%, 25%).
3.  **MODELING (SQL Console)**
    *   Direct access to the **DuckDB** engine.
    *   Query editor with syntax hints and the ability to register results as new tables.
4.  **PLOT (Visualization)**
    *   Axis constructor (X/Y) based on **ImPlot**.
    *   Interactive charts (Zoom, Pan) for visual hypothesis testing.

### 🧠 Global Layout (Shell)
*   **Header:** Combo boxes for selecting active file and column (syncs the entire app).
*   **Inspector:** Right panel with a "live" statistics report (Mean, Median, Nulls, Uniques).
*   **Engine:** All data is stored and processed in-memory using **DuckDB**.

---

## 🛠 Tech Stack

*   **Language:** C++17
*   **Database:** [DuckDB](https://duckdb.org) (In-memory OLAP)
*   **Interface:** [ImGui](https://github.com) + [ImPlot](https://github.com)
*   **File Dialog:** [Native File Dialog Extended](https://github.com)
*   **Build System:** CMake

---

## 📦 Build & Launch

The project uses the **vcpkg** package manager to handle dependencies.

### 1. Install Dependencies (via vcpkg)
```bash
vcpkg install imgui[glfw-binding,opengl3-binding] implot duckdb nfd glfw3


# 📊 DerDatCpp — Аналитический комбайн на C++

Система для сложной аналитики данных, управляемая простыми парами **«Кнопка + Список»**. Профессиональный инструмент в обертке интуитивного интерфейса.

## 🚀 Основной функционал (Паспорт Дизайна)

Приложение разделено на 4 рабочих зоны, доступных через навигацию в Footer:

1.  **IMPORT (Файловый менеджер)**
    *   Загрузка CSV с автоопределением разделителей.
    *   Вертикальная склейка (**UNION**) и горизонтальное соединение (**JOIN**) по ключам.
    *   Быстрый экспорт обработанных таблиц обратно в CSV.
2.  **CLEAN (Санитарная зона)**
    *   **Визуал:** Таблица-превью с подсветкой активной колонки (синий фокус).
    *   **Parsing:** Извлечение чисел из текста через RegEx.
    *   **Smart Fill:** Умное заполнение NULL (сверка с другой колонкой + линейная интерполяция).
    *   **Outliers:** Двустороннее удаление выбросов (квантили 5%, 10%, 25%).
3.  **MODELING (SQL-Консоль)**
    *   Прямой доступ к движку **DuckDB**.
    *   Редактор запросов с подсказками по синтаксису и сохранением результатов в новые таблицы.
4.  **PLOT (Визуализация)**
    *   Конструктор осей (X/Y) на базе **ImPlot**.
    *   Интерактивные графики (Zoom, Pan) для визуальной проверки гипотез.

### 🧠 Глобальный макет (Shell)
*   **Header:** Combo-боксы для выбора активного файла и колонки (синхронизируют всё приложение).
*   **Inspector:** Правая панель с «живым» отчетом статистики (Mean, Median, Nulls, Uniques).
*   **Engine:** Все данные хранятся и обрабатываются в оперативной памяти с помощью **DuckDB**.

---

## 🛠 Технический стек

*   **Язык:** C++17
*   **База данных:** [DuckDB](https://duckdb.org) (In-memory OLAP)
*   **Интерфейс:** [ImGui](https://github.com) + [ImPlot](https://github.com)
*   **Файловый диалог:** [Native File Dialog Extended](https://github.com)
*   **Сборка:** CMake

---

## 📦 Сборка и запуск

Проект использует менеджер пакетов **vcpkg** для управления зависимостями.

### 1. Установка зависимостей (через vcpkg)
```bash
vcpkg install imgui[glfw-binding,opengl3-binding] implot duckdb nfd glfw3
