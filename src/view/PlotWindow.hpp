#pragma once
#include "../model/DataModel.hpp"
#include "../core/AppState.hpp"
#include <imgui.h>
#include <implot.h>
#include <vector>

class PlotWindow {
public:
    static void Render(DataModel& model, AppState& state);
};
