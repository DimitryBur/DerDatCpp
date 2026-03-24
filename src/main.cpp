#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <nfd.hpp>
#include "model/DataModel.hpp"
#include "core/AppState.hpp"
#include "view/ImportWindow.hpp"
#include "view/CleanWindow.hpp"
#include "view/LabWindow.hpp"
#include "view/PlotWindow.hpp"



int main() {
    // --- ИНИЦИАЛИЗАЦИЯ ---
    if (!glfwInit()) return -1;
    NFD_Init();

    GLFWwindow* window = glfwCreateWindow(1400, 800, "DerDatCpp Analysis", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Шрифты (Кириллица)
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    // Объекты управления
    DataModel model;
    AppState state;
    auto& db_con = model.GetConnection(); // Ссылка на соединение DuckDB

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        float fw = (float)w, fh = (float)h;

        // --- 1. HEADER (Всегда на виду) ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(fw, 60));
        ImGui::Begin("Header", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            
            // Combo 1: Активный файл
            ImGui::Text("Файл:"); ImGui::SameLine();
            ImGui::SetNextItemWidth(250);
            
            // Обновляем список таблиц из БД
            state.refreshTableList(db_con); 

            if (ImGui::BeginCombo("##file_combo", state.activeTable.empty() ? "Выберите файл..." : state.activeTable.c_str())) {
                for (const auto& t : state.tableList) {
                    bool is_selected = (state.activeTable == t);
                    if (ImGui::Selectable(t.c_str(), is_selected)) {
                        state.setActiveTable(t, db_con); // Автоматически обновит и список колонок
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine(450);

            // Combo 2: Активная колонка
            ImGui::Text("Колонка:"); ImGui::SameLine();
            ImGui::SetNextItemWidth(250);
            if (ImGui::BeginCombo("##col_combo", state.activeColumn.empty() ? "Выберите колонку..." : state.activeColumn.c_str())) {
                for (const auto& c : state.columnList) {
                    bool is_selected = (state.activeColumn == c);
                    if (ImGui::Selectable(c.c_str(), is_selected)) {
                        state.activeColumn = c;
                    }
                }
                ImGui::EndCombo();
            }
        ImGui::End();

        // --- 2. WORKSPACE (75% ширины) ---
        ImGui::SetNextWindowPos(ImVec2(0, 60));
        ImGui::SetNextWindowSize(ImVec2(fw * 0.75f, fh - 120));
        ImGui::Begin("Workspace", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            
            // Навигация по страницам через state
            switch(state.currentWindow) {
                case AppState::Window::IMPORT:
                    ImportWindow::Render(model, state);
                    break;
                case AppState::Window::CLEAN:
                    CleanWindow::Render(model, state);
                    break;
                case AppState::Window::MODELING:
                    LabWindow::Render(model, state);
                    break;
                case AppState::Window::PLOT:
                    PlotWindow::Render(model, state);
                    break;
            }
        ImGui::End();

        // --- 3. INSPECTOR (Правая панель 25% ширины) ---
        ImGui::SetNextWindowPos(ImVec2(fw * 0.75f, 60));
        ImGui::SetNextWindowSize(ImVec2(fw * 0.25f, fh - 120));
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.8f, 0, 1));
    ImGui::Text("ЖИВОЙ ОТЧЕТ (СТАТИСТИКА)");
    ImGui::PopStyleColor();
    ImGui::Separator();
    
    if (state.activeColumn.empty()) {
        ImGui::TextWrapped("Выберите колонку в Header для быстрого анализа.");
    } else {
        // ВЫЗОВ РЕАЛЬНОЙ СТАТИСТИКИ
        ColumnStats stats = model.GetStats(state.activeTable, state.activeColumn);
        
        ImGui::Text("Таблица:"); ImGui::SameLine(120); ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s", state.activeTable.c_str());
        ImGui::Text("Колонка:"); ImGui::SameLine(120); ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s", state.activeColumn.c_str());
        ImGui::Separator();

        // Основные метрики
        ImGui::Text("Строк всего:");  ImGui::SameLine(120); ImGui::Text("%s", stats.count.c_str());
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.4f, 0.4f, 1));
        ImGui::Text("Пропуски:");    ImGui::SameLine(120); ImGui::Text("%s (NULL)", stats.nulls.c_str());
        ImGui::PopStyleColor();

        ImGui::Text("Уникальных:");   ImGui::SameLine(120); ImGui::Text("%s", stats.unique.c_str());
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), "МАТЕМАТИКА:");
        
        ImGui::Text("Минимум:");     ImGui::SameLine(120); ImGui::Text("%s", stats.min.c_str());
        ImGui::Text("Максимум:");    ImGui::SameLine(120); ImGui::Text("%s", stats.max.c_str());
        ImGui::Text("Среднее:");     ImGui::SameLine(120); ImGui::Text("%s", stats.avg.c_str());
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0.7f, 1));
        ImGui::Text("Медиана:");     ImGui::SameLine(120); ImGui::Text("%s", stats.median.c_str());
        ImGui::PopStyleColor();

        // Место под мини-график (ImPlot) в будущем
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("(Здесь будет распределение)");
        }
        ImGui::End();


        // --- 4. FOOTER (Навигация) ---
        ImGui::SetNextWindowPos(ImVec2(0, fh - 60));
        ImGui::SetNextWindowSize(ImVec2(fw, 60));
        ImGui::Begin("Footer", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            
            auto btnSize = ImVec2(150, 40);
            if (ImGui::Button("ИМПОРТ", btnSize))   state.currentWindow = AppState::Window::IMPORT; ImGui::SameLine();
            if (ImGui::Button("CLEAN", btnSize))    state.currentWindow = AppState::Window::CLEAN; ImGui::SameLine();
            if (ImGui::Button("MODELING", btnSize)) state.currentWindow = AppState::Window::MODELING; ImGui::SameLine();
            if (ImGui::Button("PLOT", btnSize))     state.currentWindow = AppState::Window::PLOT;
            
        ImGui::End();

        // --- ОТРЕНДЕР ---
        ImGui::Render();
        glViewport(0, 0, (int)fw, (int)fh);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Завершение
    NFD_Quit();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
