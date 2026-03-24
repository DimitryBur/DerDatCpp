#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <nfd.hpp>
#include "model/DataModel.hpp"     // Заголовок модели
#include "controller/AppController.cpp" 
#include "view/ImportWindow.cpp"  
#include "view/CleanWindow.cpp" 
#include "view/LabWindow.cpp" 
#include "view/PlotWindow.cpp" 

int main() {
    if (!glfwInit()) return -1;
    NFD_Init();

    GLFWwindow* window = glfwCreateWindow(1280, 720, "DerDatCpp Analysis", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    DataModel model;
    AppController controller;

    std::string activeTable = "";
    std::string activeColumn = "";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        float fw = (float)w, fh = (float)h;

        // 1. HEADER
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(fw, 60));
        ImGui::Begin("Header", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            
            // Выбор Таблицы
            ImGui::Text("Файл:"); ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            // --- HEADER: ВЫБОР ФАЙЛА ---
            auto tables = model.GetTableNames();
            if (ImGui::BeginCombo("##file_combo", activeTable.empty() ? "Выберите..." : activeTable.c_str())) {
                for (size_t i = 0; i < tables.size(); i++) {
                    const std::string& t = tables[i];
                    bool is_selected = (activeTable == t);
                    if (ImGui::Selectable(t.c_str(), is_selected)) {
                        activeTable = t;   // Если снова будет ошибка, замени на: activeTable.assign(t);
                        activeColumn = ""; 
                    }
                }
            ImGui::EndCombo();
            }

ImGui::SameLine(400);
ImGui::Text("Колонка:"); ImGui::SameLine();
ImGui::SetNextItemWidth(200);

// --- HEADER: ВЫБОР КОЛОНКИ ---
if (!activeTable.empty()) {
    auto columns = model.GetColumns(activeTable);
    if (ImGui::BeginCombo("##col_combo", activeColumn.empty() ? "Выберите..." : activeColumn.c_str())) {
        for (size_t i = 0; i < columns.size(); i++) {
            const std::string& c = columns[i];
            bool is_selected = (activeColumn == c);
            if (ImGui::Selectable(c.c_str(), is_selected)) {
                activeColumn = c; // Если снова будет ошибка, замени на: activeColumn.assign(c);
            }
        }
        ImGui::EndCombo();
    }
}

        ImGui::End();

        // 2. WORKSPACE
        ImGui::SetNextWindowPos(ImVec2(0, 60));
        ImGui::SetNextWindowSize(ImVec2(fw * 0.75f, fh - 120));
        ImGui::Begin("Workspace", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            ImGui::TextColored(ImVec4(0, 1, 1, 1), controller.GetPageTitle().c_str());
            ImGui::Separator();
            
            if (controller.GetCurrentPage() == AppPage::IMPORT) {
                ImportWindow::Render(model, activeTable);
            }
            if (controller.GetCurrentPage() == AppPage::CLEAN) {
                CleanWindow::Render(model, activeTable, activeColumn);
}

        ImGui::End();

        // 3. INSPECTOR
        ImGui::SetNextWindowPos(ImVec2(fw * 0.75f, 60));
        ImGui::SetNextWindowSize(ImVec2(fw * 0.25f, fh - 120));
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            ImGui::Text("ИНСПЕКТОР СТАТИСТИКИ");
            ImGui::Separator();
            if (!activeColumn.empty()) ImGui::Text("Активная колонка: %s", activeColumn.c_str());
        ImGui::End();

        // 4. FOOTER
        ImGui::SetNextWindowPos(ImVec2(0, fh - 60));
        ImGui::SetNextWindowSize(ImVec2(fw, 60));
        ImGui::Begin("Footer", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            if (ImGui::Button("ИМПОРТ", ImVec2(120, 40)))   controller.SetPage(AppPage::IMPORT); ImGui::SameLine();
            if (ImGui::Button("CLEAN", ImVec2(120, 40)))    controller.SetPage(AppPage::CLEAN); ImGui::SameLine();
            if (ImGui::Button("MODELING", ImVec2(120, 40))) controller.SetPage(AppPage::MODELING); ImGui::SameLine();
            if (ImGui::Button("PLOT", ImVec2(120, 40)))     controller.SetPage(AppPage::PLOT);
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    NFD_Quit();
    return 0;
}
