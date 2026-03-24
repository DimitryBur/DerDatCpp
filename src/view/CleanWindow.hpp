#pragma once
#include "../model/DataModel.hpp"
#include "../core/AppState.hpp"
#include <imgui.h>

class CleanWindow {
public:
    // Метод должен быть статическим, чтобы вызывать его как ImportWindow::Render
    static void Render(DataModel& model, AppState& state);
};