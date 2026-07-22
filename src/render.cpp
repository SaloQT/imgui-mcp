// render.cpp – render_widget() implementation for all Dear ImGui widget types
// Part of imgui_mcp_app: Interactive Dear ImGui application controlled via MCP

#include "types.h"
#include <cmath>
#include <cfloat>

// ─── Helpers ─────────────────────────────────────────────────────────────────

static inline const char* fmt_or_null(const std::string& f) {
    return f.empty() ? nullptr : f.c_str();
}

static inline const char* text_or_label(const Widget& w) {
    return w.content.empty() ? w.label.c_str() : w.content.c_str();
}

// ─── render_widget ───────────────────────────────────────────────────────────

void render_widget(const std::string& win_id, Widget& w) {
    // Reset per-frame state flags
    w.clicked  = false;
    w.changed  = false;
    w.hovered  = false;
    w.active   = false;
    w.focused  = false;

    // Handle keyboard focus request
    if (w.request_focus) {
        ImGui::SetKeyboardFocusHere();
        w.request_focus = false;
    }

    // Apply cursor offset if set (from move_widget command)
    if (w.cursor_offset[0] != 0.0f || w.cursor_offset[1] != 0.0f) {
        ImVec2 cur = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cur.x + w.cursor_offset[0], cur.y + w.cursor_offset[1]));
    }

    // ── Conditional visibility check ──────────────────────────────────────
    if (!w.visible_condition.empty() && w.visible_condition != "always") {
        if (w.visible_condition == "never") {
            return; // skip rendering entirely
        }
        // Parse simple conditions: "widget_id.checked==true" or "widget_id.value>X"
        // Look up referenced widget in the same window (caller holds g_mutex)
        auto win_it = g_windows.find(win_id);
        if (win_it != g_windows.end()) {
            const std::string& cond = w.visible_condition;
            // Check for ".checked==true" pattern
            auto cpos = cond.find(".checked==true");
            if (cpos != std::string::npos) {
                std::string ref_id = cond.substr(0, cpos);
                auto ref_it = win_it->second.widgets.find(ref_id);
                if (ref_it == win_it->second.widgets.end() || !ref_it->second.bool_val) {
                    return; // condition not met, skip
                }
            } else {
                // Check for ".value>X" pattern
                auto vpos = cond.find(".value>");
                if (vpos != std::string::npos) {
                    std::string ref_id = cond.substr(0, vpos);
                    std::string threshold_str = cond.substr(vpos + 7); // after ".value>"
                    float threshold = 0.0f;
                    try { threshold = std::stof(threshold_str); } catch (...) {}
                    auto ref_it = win_it->second.widgets.find(ref_id);
                    if (ref_it == win_it->second.widgets.end() ||
                        !(ref_it->second.float_val[0] > threshold)) {
                        return; // condition not met, skip
                    }
                }
            }
        }
    }

    if (!w.enabled)
        ImGui::BeginDisabled();

    switch (w.type) {

    // ── Buttons ──────────────────────────────────────────────────────────
    case WidgetType::Button:
        if (ImGui::Button(w.label.c_str(), ImVec2(w.size_x, w.size_y))) {
            w.clicked = true;
            emit_event(win_id, w.id, "clicked");
        }
        break;

    case WidgetType::SmallButton:
        if (ImGui::SmallButton(w.label.c_str())) {
            w.clicked = true;
            emit_event(win_id, w.id, "clicked");
        }
        break;

    case WidgetType::InvisibleButton:
        if (ImGui::InvisibleButton(w.id.c_str(), ImVec2(w.size_x, w.size_y))) {
            w.clicked = true;
            emit_event(win_id, w.id, "clicked");
        }
        break;

    case WidgetType::ArrowButton:
        if (ImGui::ArrowButton(w.id.c_str(), (ImGuiDir)w.dir)) {
            w.clicked = true;
            emit_event(win_id, w.id, "clicked");
        }
        break;

    // ── Toggles / flags ──────────────────────────────────────────────────
    case WidgetType::Checkbox:
        if (ImGui::Checkbox(w.label.c_str(), &w.bool_val)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"checked", w.bool_val}});
        }
        break;

    case WidgetType::CheckboxFlags:
        if (ImGui::CheckboxFlags(w.label.c_str(), &w.int_val[0], w.int_val[1])) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"flags", w.int_val[0]}});
        }
        break;

    case WidgetType::RadioButton:
        if (ImGui::RadioButton(w.label.c_str(), w.bool_val)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"checked", true}});
        }
        break;

    // ── Progress / misc ──────────────────────────────────────────────────
    case WidgetType::ProgressBar:
        ImGui::ProgressBar(w.float_val[0], ImVec2(w.size_x, w.size_y),
                           w.overlay.empty() ? NULL : w.overlay.c_str());
        break;

    case WidgetType::Bullet:
        ImGui::Bullet();
        break;

    // ── Links ────────────────────────────────────────────────────────────
    case WidgetType::TextLink:
        if (ImGui::TextLink(w.label.c_str())) {
            w.clicked = true;
            emit_event(win_id, w.id, "clicked");
        }
        break;

    case WidgetType::TextLinkOpenURL:
        if (ImGui::TextLinkOpenURL(w.label.c_str(), w.content.c_str())) {
            w.clicked = true;
            emit_event(win_id, w.id, "clicked");
        }
        break;

    // ── Text ─────────────────────────────────────────────────────────────
    case WidgetType::Text:
        ImGui::TextUnformatted(text_or_label(w));
        break;

    case WidgetType::TextColored:
        ImGui::TextColored(ImVec4(w.color[0], w.color[1], w.color[2], w.color[3]),
                           "%s", text_or_label(w));
        break;

    case WidgetType::TextDisabled:
        ImGui::TextDisabled("%s", text_or_label(w));
        break;

    case WidgetType::TextWrapped:
        ImGui::TextWrapped("%s", text_or_label(w));
        break;

    case WidgetType::LabelText:
        ImGui::LabelText(w.label.c_str(), "%s", w.content.c_str());
        break;

    case WidgetType::BulletText:
        ImGui::BulletText("%s", text_or_label(w));
        break;

    case WidgetType::SeparatorText:
        ImGui::SeparatorText(w.label.c_str());
        break;

    // ── Layout ───────────────────────────────────────────────────────────
    case WidgetType::Separator:
        ImGui::Separator();
        break;

    case WidgetType::SameLine:
        ImGui::SameLine(w.float_val[0], w.float_val[1]);
        break;

    case WidgetType::NewLine:
        ImGui::NewLine();
        break;

    case WidgetType::Spacing:
        ImGui::Spacing();
        break;

    case WidgetType::Dummy:
        ImGui::Dummy(ImVec2(w.size_x, w.size_y));
        break;

    case WidgetType::Indent:
        ImGui::Indent(w.float_val[0]);
        break;

    case WidgetType::Unindent:
        ImGui::Unindent(w.float_val[0]);
        break;

    case WidgetType::AlignTextToFramePadding:
        ImGui::AlignTextToFramePadding();
        break;

    // ── Input: text ──────────────────────────────────────────────────────
    case WidgetType::InputText:
        if (ImGui::InputText(w.label.c_str(), w.text_buf, sizeof(w.text_buf))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"text", std::string(w.text_buf)}});
        }
        break;

    case WidgetType::InputTextMultiline:
        if (ImGui::InputTextMultiline(w.label.c_str(), w.text_buf, sizeof(w.text_buf),
                                      ImVec2(w.size_x, w.size_y))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"text", std::string(w.text_buf)}});
        }
        break;

    case WidgetType::InputTextWithHint:
        if (ImGui::InputTextWithHint(w.label.c_str(), w.hint.c_str(),
                                     w.text_buf, sizeof(w.text_buf))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"text", std::string(w.text_buf)}});
        }
        break;

    // ── Input: numeric ───────────────────────────────────────────────────
    case WidgetType::InputFloat:
        if (ImGui::InputFloat(w.label.c_str(), &w.float_val[0],
                              (float)w.step, (float)w.step_fast,
                              fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.float_val[0]}});
        }
        break;

    case WidgetType::InputFloat2:
        if (ImGui::InputFloat2(w.label.c_str(), w.float_val, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.float_val[0], w.float_val[1]}}});
        }
        break;

    case WidgetType::InputFloat3:
        if (ImGui::InputFloat3(w.label.c_str(), w.float_val, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.float_val[0], w.float_val[1], w.float_val[2]}}});
        }
        break;

    case WidgetType::InputFloat4:
        if (ImGui::InputFloat4(w.label.c_str(), w.float_val, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.float_val[0], w.float_val[1], w.float_val[2], w.float_val[3]}}});
        }
        break;

    case WidgetType::InputInt:
        if (ImGui::InputInt(w.label.c_str(), &w.int_val[0],
                            (int)w.step, (int)w.step_fast)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.int_val[0]}});
        }
        break;

    case WidgetType::InputInt2:
        if (ImGui::InputInt2(w.label.c_str(), w.int_val)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.int_val[0], w.int_val[1]}}});
        }
        break;

    case WidgetType::InputInt3:
        if (ImGui::InputInt3(w.label.c_str(), w.int_val)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.int_val[0], w.int_val[1], w.int_val[2]}}});
        }
        break;

    case WidgetType::InputInt4:
        if (ImGui::InputInt4(w.label.c_str(), w.int_val)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.int_val[0], w.int_val[1], w.int_val[2], w.int_val[3]}}});
        }
        break;

    case WidgetType::InputDouble:
        if (ImGui::InputDouble(w.label.c_str(), &w.double_val,
                               w.step, w.step_fast, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.double_val}});
        }
        break;

    // ── Sliders ──────────────────────────────────────────────────────────
    case WidgetType::SliderFloat:
        if (ImGui::SliderFloat(w.label.c_str(), &w.float_val[0],
                               w.float_min, w.float_max, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.float_val[0]}});
        }
        break;

    case WidgetType::SliderFloat2:
        if (ImGui::SliderFloat2(w.label.c_str(), w.float_val,
                                w.float_min, w.float_max, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.float_val[0], w.float_val[1]}}});
        }
        break;

    case WidgetType::SliderFloat3:
        if (ImGui::SliderFloat3(w.label.c_str(), w.float_val,
                                w.float_min, w.float_max, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.float_val[0], w.float_val[1], w.float_val[2]}}});
        }
        break;

    case WidgetType::SliderFloat4:
        if (ImGui::SliderFloat4(w.label.c_str(), w.float_val,
                                w.float_min, w.float_max, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.float_val[0], w.float_val[1], w.float_val[2], w.float_val[3]}}});
        }
        break;

    case WidgetType::SliderInt:
        if (ImGui::SliderInt(w.label.c_str(), &w.int_val[0],
                             w.int_min, w.int_max, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.int_val[0]}});
        }
        break;

    case WidgetType::SliderInt2:
        if (ImGui::SliderInt2(w.label.c_str(), w.int_val,
                              w.int_min, w.int_max, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.int_val[0], w.int_val[1]}}});
        }
        break;

    case WidgetType::SliderInt3:
        if (ImGui::SliderInt3(w.label.c_str(), w.int_val,
                              w.int_min, w.int_max, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.int_val[0], w.int_val[1], w.int_val[2]}}});
        }
        break;

    case WidgetType::SliderInt4:
        if (ImGui::SliderInt4(w.label.c_str(), w.int_val,
                              w.int_min, w.int_max, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.int_val[0], w.int_val[1], w.int_val[2], w.int_val[3]}}});
        }
        break;

    case WidgetType::SliderAngle:
        if (ImGui::SliderAngle(w.label.c_str(), &w.float_val[0],
                               w.float_min, w.float_max, fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.float_val[0]}});
        }
        break;

    case WidgetType::VSliderFloat:
        if (ImGui::VSliderFloat(w.label.c_str(), ImVec2(w.size_x, w.size_y),
                                &w.float_val[0], w.float_min, w.float_max,
                                fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.float_val[0]}});
        }
        break;

    case WidgetType::VSliderInt:
        if (ImGui::VSliderInt(w.label.c_str(), ImVec2(w.size_x, w.size_y),
                              &w.int_val[0], w.int_min, w.int_max,
                              fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.int_val[0]}});
        }
        break;

    // ── Drag ─────────────────────────────────────────────────────────────
    case WidgetType::DragFloat:
        if (ImGui::DragFloat(w.label.c_str(), &w.float_val[0],
                             w.float_speed, w.float_min, w.float_max,
                             fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.float_val[0]}});
        }
        break;

    case WidgetType::DragFloat2:
        if (ImGui::DragFloat2(w.label.c_str(), w.float_val,
                              w.float_speed, w.float_min, w.float_max,
                              fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.float_val[0], w.float_val[1]}}});
        }
        break;

    case WidgetType::DragFloat3:
        if (ImGui::DragFloat3(w.label.c_str(), w.float_val,
                              w.float_speed, w.float_min, w.float_max,
                              fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.float_val[0], w.float_val[1], w.float_val[2]}}});
        }
        break;

    case WidgetType::DragFloat4:
        if (ImGui::DragFloat4(w.label.c_str(), w.float_val,
                              w.float_speed, w.float_min, w.float_max,
                              fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.float_val[0], w.float_val[1], w.float_val[2], w.float_val[3]}}});
        }
        break;

    case WidgetType::DragFloatRange2:
        if (ImGui::DragFloatRange2(w.label.c_str(), &w.float_val[0], &w.float_val[1],
                                   w.float_speed, w.float_min, w.float_max,
                                   fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"min", w.float_val[0]}, {"max", w.float_val[1]}});
        }
        break;

    case WidgetType::DragInt:
        if (ImGui::DragInt(w.label.c_str(), &w.int_val[0],
                           w.float_speed, w.int_min, w.int_max,
                           fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"value", w.int_val[0]}});
        }
        break;

    case WidgetType::DragInt2:
        if (ImGui::DragInt2(w.label.c_str(), w.int_val,
                            w.float_speed, w.int_min, w.int_max,
                            fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.int_val[0], w.int_val[1]}}});
        }
        break;

    case WidgetType::DragInt3:
        if (ImGui::DragInt3(w.label.c_str(), w.int_val,
                            w.float_speed, w.int_min, w.int_max,
                            fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.int_val[0], w.int_val[1], w.int_val[2]}}});
        }
        break;

    case WidgetType::DragInt4:
        if (ImGui::DragInt4(w.label.c_str(), w.int_val,
                            w.float_speed, w.int_min, w.int_max,
                            fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"value", {w.int_val[0], w.int_val[1], w.int_val[2], w.int_val[3]}}});
        }
        break;

    case WidgetType::DragIntRange2:
        if (ImGui::DragIntRange2(w.label.c_str(), &w.int_val[0], &w.int_val[1],
                                 w.float_speed, w.int_min, w.int_max,
                                 fmt_or_null(w.format))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"min", w.int_val[0]}, {"max", w.int_val[1]}});
        }
        break;

    // ── Color ────────────────────────────────────────────────────────────
    case WidgetType::ColorEdit3:
        if (ImGui::ColorEdit3(w.label.c_str(), w.color)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"color", {w.color[0], w.color[1], w.color[2]}}});
        }
        break;

    case WidgetType::ColorEdit4:
        if (ImGui::ColorEdit4(w.label.c_str(), w.color)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"color", {w.color[0], w.color[1], w.color[2], w.color[3]}}});
        }
        break;

    case WidgetType::ColorPicker3:
        if (ImGui::ColorPicker3(w.label.c_str(), w.color)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"color", {w.color[0], w.color[1], w.color[2]}}});
        }
        break;

    case WidgetType::ColorPicker4:
        if (ImGui::ColorPicker4(w.label.c_str(), w.color)) {
            w.changed = true;
            emit_event(win_id, w.id, "changed",
                {{"color", {w.color[0], w.color[1], w.color[2], w.color[3]}}});
        }
        break;

    case WidgetType::ColorButton:
        if (ImGui::ColorButton(w.id.c_str(),
                               ImVec4(w.color[0], w.color[1], w.color[2], w.color[3]),
                               0, ImVec2(w.size_x, w.size_y))) {
            w.clicked = true;
            emit_event(win_id, w.id, "clicked");
        }
        break;

    // ── Combo / ListBox ──────────────────────────────────────────────────
    case WidgetType::Combo: {
        if (!w.items.empty()) {
            // Build null-separated items string
            std::string items_str;
            for (const auto& item : w.items) {
                items_str += item;
                items_str += '\0';
            }
            items_str += '\0';
            if (ImGui::Combo(w.label.c_str(), &w.selected, items_str.c_str())) {
                w.changed = true;
                json data = {{"selected", w.selected}};
                if (w.selected >= 0 && w.selected < (int)w.items.size())
                    data["value"] = w.items[w.selected];
                emit_event(win_id, w.id, "changed", data);
            }
        }
        break;
    }

    case WidgetType::ListBox: {
        if (!w.items.empty()) {
            std::vector<const char*> c_items;
            c_items.reserve(w.items.size());
            for (const auto& item : w.items)
                c_items.push_back(item.c_str());
            if (ImGui::ListBox(w.label.c_str(), &w.selected,
                               c_items.data(), (int)c_items.size())) {
                w.changed = true;
                json data = {{"selected", w.selected}};
                if (w.selected >= 0 && w.selected < (int)w.items.size())
                    data["value"] = w.items[w.selected];
                emit_event(win_id, w.id, "changed", data);
            }
        }
        break;
    }

    // ── Tree / Selectable ────────────────────────────────────────────────
    case WidgetType::TreeNode:
        if (ImGui::TreeNode(w.label.c_str())) {
            w.tree_open = true;
            for (auto& child : w.children)
                render_widget(win_id, child);
            ImGui::TreePop();
        } else {
            w.tree_open = false;
        }
        break;

    case WidgetType::TreeNodeEx:
        if (ImGui::TreeNodeEx(w.label.c_str(), (ImGuiTreeNodeFlags)w.flags)) {
            w.tree_open = true;
            for (auto& child : w.children)
                render_widget(win_id, child);
            ImGui::TreePop();
        } else {
            w.tree_open = false;
        }
        break;

    case WidgetType::CollapsingHeader:
        if (ImGui::CollapsingHeader(w.label.c_str(), (ImGuiTreeNodeFlags)w.flags)) {
            for (auto& child : w.children)
                render_widget(win_id, child);
        }
        break;

    case WidgetType::Selectable:
        if (ImGui::Selectable(w.label.c_str(), &w.bool_val,
                              (ImGuiSelectableFlags)w.flags,
                              ImVec2(w.size_x, w.size_y))) {
            w.changed = true;
            emit_event(win_id, w.id, "changed", {{"selected", w.bool_val}});
        }
        break;

    // ── Table ────────────────────────────────────────────────────────────
    case WidgetType::Table: {
        int col_count = w.columns > 0 ? w.columns : (int)w.headers.size();
        if (col_count <= 0) col_count = 1;
        if (ImGui::BeginTable(w.id.c_str(), col_count,
                              (ImGuiTableFlags)w.flags,
                              ImVec2(w.size_x, w.size_y))) {
            // Setup columns from headers
            for (const auto& hdr : w.headers)
                ImGui::TableSetupColumn(hdr.c_str());
            if (!w.headers.empty())
                ImGui::TableHeadersRow();

            // Render rows
            for (const auto& row : w.rows) {
                ImGui::TableNextRow();
                for (int c = 0; c < (int)row.size() && c < col_count; c++) {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(row[c].c_str());
                }
            }

            // Render children (if any are provided instead of rows)
            for (auto& child : w.children)
                render_widget(win_id, child);

            ImGui::EndTable();
        }
        break;
    }

    // ── Menu ─────────────────────────────────────────────────────────────
    case WidgetType::MenuBar:
        if (ImGui::BeginMenuBar()) {
            for (auto& child : w.children)
                render_widget(win_id, child);
            ImGui::EndMenuBar();
        }
        break;

    case WidgetType::MainMenuBar:
        if (ImGui::BeginMainMenuBar()) {
            for (auto& child : w.children)
                render_widget(win_id, child);
            ImGui::EndMainMenuBar();
        }
        break;

    case WidgetType::Menu:
        if (ImGui::BeginMenu(w.label.c_str(), w.enabled)) {
            for (auto& child : w.children)
                render_widget(win_id, child);
            ImGui::EndMenu();
        }
        break;

    case WidgetType::MenuItem:
        if (ImGui::MenuItem(w.label.c_str(),
                            w.shortcut.empty() ? NULL : w.shortcut.c_str(),
                            &w.bool_val, w.enabled)) {
            w.clicked = true;
            emit_event(win_id, w.id, "clicked");
        }
        break;

    // ── Popup ────────────────────────────────────────────────────────────
    case WidgetType::Popup:
        if (w.popup_open)
            ImGui::OpenPopup(w.id.c_str());
        if (ImGui::BeginPopup(w.id.c_str())) {
            for (auto& child : w.children)
                render_widget(win_id, child);
            ImGui::EndPopup();
        } else {
            w.popup_open = false;
        }
        break;

    case WidgetType::PopupModal:
        if (w.popup_open)
            ImGui::OpenPopup(w.id.c_str());
        if (ImGui::BeginPopupModal(w.id.c_str(), NULL, (ImGuiWindowFlags)w.flags)) {
            for (auto& child : w.children)
                render_widget(win_id, child);
            ImGui::EndPopup();
        } else {
            w.popup_open = false;
        }
        break;

    // ── Plots ────────────────────────────────────────────────────────────
    case WidgetType::PlotLines:
        if (!w.plot_values.empty()) {
            ImGui::PlotLines(w.label.c_str(), w.plot_values.data(),
                             (int)w.plot_values.size(), 0,
                             w.overlay.empty() ? NULL : w.overlay.c_str(),
                             FLT_MAX, FLT_MAX,
                             ImVec2(w.size_x, w.size_y));
        }
        break;

    case WidgetType::PlotHistogram:
        if (!w.plot_values.empty()) {
            ImGui::PlotHistogram(w.label.c_str(), w.plot_values.data(),
                                 (int)w.plot_values.size(), 0,
                                 w.overlay.empty() ? NULL : w.overlay.c_str(),
                                 FLT_MAX, FLT_MAX,
                                 ImVec2(w.size_x, w.size_y));
        }
        break;

    // ── Containers ───────────────────────────────────────────────────────
    case WidgetType::ChildWindow:
        if (ImGui::BeginChild(w.id.c_str(), ImVec2(w.size_x, w.size_y),
                              (ImGuiChildFlags)w.flags)) {
            for (auto& child : w.children)
                render_widget(win_id, child);
        }
        ImGui::EndChild();
        break;

    case WidgetType::TabBar:
        if (ImGui::BeginTabBar(w.id.c_str(), (ImGuiTabBarFlags)w.flags)) {
            for (auto& child : w.children)
                render_widget(win_id, child);
            ImGui::EndTabBar();
        }
        break;

    case WidgetType::TabItem:
        if (ImGui::BeginTabItem(w.label.c_str(),
                                w.bool_val ? &w.bool_val2 : NULL,
                                (ImGuiTabItemFlags)w.flags)) {
            for (auto& child : w.children)
                render_widget(win_id, child);
            ImGui::EndTabItem();
        }
        break;

    case WidgetType::Group:
        ImGui::BeginGroup();
        for (auto& child : w.children)
            render_widget(win_id, child);
        ImGui::EndGroup();
        break;

    // ── Value helpers ────────────────────────────────────────────────────
    case WidgetType::ValueBool:
        ImGui::Value(w.label.c_str(), w.bool_val);
        break;

    case WidgetType::ValueInt:
        ImGui::Value(w.label.c_str(), w.int_val[0]);
        break;

    case WidgetType::ValueFloat:
        ImGui::Value(w.label.c_str(), w.float_val[0], fmt_or_null(w.format));
        break;

    // ── Tooltip ──────────────────────────────────────────────────────────
    case WidgetType::Tooltip:
        if (ImGui::BeginTooltip()) {
            if (!w.children.empty()) {
                for (auto& child : w.children)
                    render_widget(win_id, child);
            } else {
                ImGui::TextUnformatted(text_or_label(w));
            }
            ImGui::EndTooltip();
        }
        break;

    case WidgetType::SetItemTooltip:
        ImGui::SetItemTooltip("%s", w.tooltip.c_str());
        break;

    // ── DrawList ─────────────────────────────────────────────────────────
    case WidgetType::DrawList: {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            for (const auto& cmd : w.draw_commands) {
                ImU32 col = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(cmd.color[0], cmd.color[1], cmd.color[2], cmd.color[3]));
                ImVec2 p1(cmd.p1[0], cmd.p1[1]);
                ImVec2 p2(cmd.p2[0], cmd.p2[1]);
                ImVec2 p3(cmd.p3[0], cmd.p3[1]);
                ImVec2 p4(cmd.p4[0], cmd.p4[1]);

                switch (cmd.type) {
                case 0: // Line
                    dl->AddLine(p1, p2, col, cmd.thickness);
                    break;
                case 1: // Rect
                    dl->AddRect(p1, p2, col, 0.0f, cmd.thickness);
                    break;
                case 2: // RectFilled
                    dl->AddRectFilled(p1, p2, col);
                    break;
                case 3: // Circle
                    dl->AddCircle(p1, p2[0], col, cmd.num_segments, cmd.thickness);
                    break;
                case 4: // CircleFilled
                    dl->AddCircleFilled(p1, p2[0], col, cmd.num_segments);
                    break;
                case 5: // Triangle
                    dl->AddTriangle(p1, p2, p3, col, cmd.thickness);
                    break;
                case 6: // TriangleFilled
                    dl->AddTriangleFilled(p1, p2, p3, col);
                    break;
                case 7: { // Polyline
                    int n = cmd.num_segments > 0 ? cmd.num_segments : 4;
                    if (n > 4) n = 4;
                    if (n < 2) n = 2;
                    ImVec2 pts[4] = { p1, p2, p3, p4 };
                    dl->AddPolyline(pts, n, col, cmd.thickness);
                    break;
                }
                case 8: { // ConvexPolyFilled
                    int n = cmd.num_segments > 0 ? cmd.num_segments : 4;
                    if (n > 4) n = 4;
                    if (n < 3) n = 3;
                    ImVec2 pts[4] = { p1, p2, p3, p4 };
                    dl->AddConvexPolyFilled(pts, n, col);
                    break;
                }
                case 9: // Quad (via AddQuad)
                    dl->AddQuad(p1, p2, p3, p4, col, cmd.thickness);
                    break;
                case 10: // QuadFilled
                    dl->AddQuadFilled(p1, p2, p3, p4, col);
                    break;
                case 11: // Text
                    dl->AddText(p1, col, cmd.text.c_str());
                    break;
                case 12: // BezierCubic
                    dl->AddBezierCubic(p1, p2, p3, p4, col, cmd.thickness, cmd.num_segments);
                    break;
                case 13: // BezierQuadratic
                    dl->AddBezierQuadratic(p1, p2, p3, col, cmd.thickness, cmd.num_segments);
                    break;
                default:
                    break;
                }
            }
        }
        break;
    }

    // ─── Game UI Pattern Widgets ─────────────────────────────────────────────

    case WidgetType::HealthBar: {
        float frac = w.float_val[0]; // current 0-1
        float maxv = w.float_val[1] > 0.0f ? w.float_val[1] : 1.0f;
        float cur  = frac * maxv;
        float bar_w = w.size_x > 0 ? w.size_x : 200.0f;
        float bar_h = w.size_y > 0 ? w.size_y : 24.0f;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // Background
        dl->AddRectFilled(p, ImVec2(p.x + bar_w, p.y + bar_h), IM_COL32(60, 60, 60, 255), 6.0f);
        // Filled portion: green→yellow→red
        float clamped = frac < 0 ? 0 : (frac > 1 ? 1 : frac);
        ImU32 fill_col;
        if (clamped > 0.5f) {
            float t = (clamped - 0.5f) * 2.0f;
            fill_col = IM_COL32((int)(255 * (1 - t)), 200, 50, 255);
        } else {
            float t = clamped * 2.0f;
            fill_col = IM_COL32(220, (int)(200 * t), 50, 255);
        }
        float fill_w = bar_w * clamped;
        if (fill_w > 1.0f)
            dl->AddRectFilled(p, ImVec2(p.x + fill_w, p.y + bar_h), fill_col, 6.0f);
        // Border
        dl->AddRect(p, ImVec2(p.x + bar_w, p.y + bar_h), IM_COL32(200, 200, 200, 255), 6.0f, 0, 1.5f);
        // Text overlay
        char buf[128];
        snprintf(buf, sizeof(buf), "HP: %.0f/%.0f", cur, maxv);
        ImVec2 ts = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(p.x + (bar_w - ts.x) * 0.5f, p.y + (bar_h - ts.y) * 0.5f), IM_COL32(255, 255, 255, 255), buf);
        ImGui::Dummy(ImVec2(bar_w, bar_h));
        break;
    }

    case WidgetType::ManaBar: {
        float frac = w.float_val[0];
        float maxv = w.float_val[1] > 0.0f ? w.float_val[1] : 1.0f;
        float cur  = frac * maxv;
        float bar_w = w.size_x > 0 ? w.size_x : 200.0f;
        float bar_h = w.size_y > 0 ? w.size_y : 24.0f;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p, ImVec2(p.x + bar_w, p.y + bar_h), IM_COL32(40, 40, 70, 255), 6.0f);
        float clamped = frac < 0 ? 0 : (frac > 1 ? 1 : frac);
        // Blue→purple gradient based on fraction
        int r = (int)(80 + 120 * clamped);
        int g = (int)(80 * (1 - clamped));
        int b = 220;
        ImU32 fill_col = IM_COL32(r, g, b, 255);
        float fill_w = bar_w * clamped;
        if (fill_w > 1.0f)
            dl->AddRectFilled(p, ImVec2(p.x + fill_w, p.y + bar_h), fill_col, 6.0f);
        dl->AddRect(p, ImVec2(p.x + bar_w, p.y + bar_h), IM_COL32(150, 150, 220, 255), 6.0f, 0, 1.5f);
        char buf[128];
        snprintf(buf, sizeof(buf), "MP: %.0f/%.0f", cur, maxv);
        ImVec2 ts = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(p.x + (bar_w - ts.x) * 0.5f, p.y + (bar_h - ts.y) * 0.5f), IM_COL32(255, 255, 255, 255), buf);
        ImGui::Dummy(ImVec2(bar_w, bar_h));
        break;
    }

    case WidgetType::InventoryGrid: {
        int cols = w.int_val[0] > 0 ? w.int_val[0] : 4;
        int rows = w.int_val[1] > 0 ? w.int_val[1] : 4;
        float slot_sz = 48.0f;
        float pad = 4.0f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                int idx = r * cols + c;
                ImVec2 p = ImGui::GetCursorScreenPos();
                bool is_sel = (idx == w.selected);
                ImU32 bg = is_sel ? IM_COL32(80, 120, 200, 255) : IM_COL32(50, 50, 55, 255);
                ImU32 border = is_sel ? IM_COL32(130, 170, 255, 255) : IM_COL32(100, 100, 110, 255);
                dl->AddRectFilled(p, ImVec2(p.x + slot_sz, p.y + slot_sz), bg, 4.0f);
                dl->AddRect(p, ImVec2(p.x + slot_sz, p.y + slot_sz), border, 4.0f, 0, 1.5f);
                // Draw item label if present
                if (idx < (int)w.items.size() && !w.items[idx].empty()) {
                    ImVec2 ts = ImGui::CalcTextSize(w.items[idx].c_str());
                    float tx = p.x + (slot_sz - ts.x) * 0.5f;
                    float ty = p.y + (slot_sz - ts.y) * 0.5f;
                    dl->AddText(ImVec2(tx, ty), IM_COL32(230, 230, 230, 255), w.items[idx].c_str());
                }
                // Invisible button for click detection
                ImGui::SetCursorScreenPos(p);
                char btn_id[64];
                snprintf(btn_id, sizeof(btn_id), "##inv%d", idx);
                if (ImGui::InvisibleButton(btn_id, ImVec2(slot_sz, slot_sz))) {
                    w.selected = idx;
                    w.clicked = true;
                    emit_event(win_id, w.id, "click", {{"slot", idx}});
                }
                if (c < cols - 1) ImGui::SameLine(0, pad);
            }
        }
        break;
    }

    case WidgetType::DialogueBox: {
        float box_w = w.size_x > 0 ? w.size_x : ImGui::GetContentRegionAvail().x;
        float box_h = w.size_y > 0 ? w.size_y : 120.0f;
        ImGui::BeginChild((w.id + "_dlg").c_str(), ImVec2(box_w, box_h), ImGuiChildFlags_Borders);
        // Speaker name
        if (!w.label.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", w.label.c_str());
            ImGui::Separator();
        }
        // Typewriter text
        int reveal = w.int_val[0] > 0 ? w.int_val[0] : (int)w.content.size();
        int show_n = reveal < (int)w.content.size() ? reveal : (int)w.content.size();
        std::string visible_text = w.content.substr(0, show_n);
        ImGui::TextWrapped("%s", visible_text.c_str());
        // Choices
        if (!w.items.empty()) {
            ImGui::Spacing();
            for (int i = 0; i < (int)w.items.size(); i++) {
                char choice_label[512];
                snprintf(choice_label, sizeof(choice_label), "%d. %s", i + 1, w.items[i].c_str());
                if (ImGui::Selectable(choice_label, w.selected == i)) {
                    w.selected = i;
                    w.clicked = true;
                    emit_event(win_id, w.id, "choice", {{"index", i}});
                }
            }
        }
        ImGui::EndChild();
        break;
    }

    case WidgetType::Minimap: {
        float map_w = w.size_x > 0 ? w.size_x : 150.0f;
        float map_h = w.size_y > 0 ? w.size_y : 150.0f;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // Dark background
        dl->AddRectFilled(p, ImVec2(p.x + map_w, p.y + map_h), IM_COL32(20, 25, 20, 255), 4.0f);
        dl->AddRect(p, ImVec2(p.x + map_w, p.y + map_h), IM_COL32(100, 140, 100, 255), 4.0f, 0, 1.5f);
        // Player triangle at center
        float cx = p.x + map_w * 0.5f;
        float cy = p.y + map_h * 0.5f;
        float ts = 6.0f;
        dl->AddTriangleFilled(
            ImVec2(cx, cy - ts), ImVec2(cx - ts * 0.7f, cy + ts * 0.6f),
            ImVec2(cx + ts * 0.7f, cy + ts * 0.6f), IM_COL32(50, 255, 50, 255));
        // Entity dots from plot_values [x1,y1,x2,y2,...]
        for (int i = 0; i + 1 < (int)w.plot_values.size(); i += 2) {
            float dx = p.x + w.plot_values[i] * map_w;
            float dy = p.y + w.plot_values[i + 1] * map_h;
            if (dx >= p.x && dx <= p.x + map_w && dy >= p.y && dy <= p.y + map_h)
                dl->AddCircleFilled(ImVec2(dx, dy), 3.0f, IM_COL32(255, 80, 80, 255));
        }
        ImGui::Dummy(ImVec2(map_w, map_h));
        break;
    }

    case WidgetType::TooltipCard: {
        float card_w = w.size_x > 0 ? w.size_x : 220.0f;
        ImGui::BeginChild((w.id + "_tip").c_str(), ImVec2(card_w, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
        // Title colored by rarity
        ImVec4 title_col(w.color[0], w.color[1], w.color[2], w.color[3] > 0 ? w.color[3] : 1.0f);
        if (title_col.x == 0 && title_col.y == 0 && title_col.z == 0 && title_col.w == 0)
            title_col = ImVec4(1, 1, 1, 1);
        ImGui::TextColored(title_col, "%s", w.label.c_str());
        ImGui::Separator();
        // Description
        if (!w.content.empty())
            ImGui::TextWrapped("%s", w.content.c_str());
        // Stat lines
        for (const auto& s : w.items)
            ImGui::Text("%s", s.c_str());
        ImGui::EndChild();
        break;
    }

    case WidgetType::CooldownRadial: {
        float radius = w.size_x > 0 ? w.size_x : 30.0f;
        float remaining = w.float_val[0]; // 0-1
        if (remaining < 0) remaining = 0;
        if (remaining > 1) remaining = 1;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImVec2 center(p.x + radius, p.y + radius);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // Background circle
        dl->AddCircleFilled(center, radius, IM_COL32(40, 40, 45, 255), 32);
        dl->AddCircle(center, radius, IM_COL32(120, 120, 130, 255), 32, 1.5f);
        // Radial sweep (cooldown overlay)
        if (remaining > 0.001f) {
            const float kPI = 3.14159265358979323846f;
            float start_angle = -kPI * 0.5f;
            float end_angle = start_angle + remaining * 2.0f * kPI;
            int segments = (int)(32 * remaining) + 2;
            ImVec2 pts[64];
            int n = 0;
            pts[n++] = center;
            for (int i = 0; i <= segments && n < 63; i++) {
                float a = start_angle + (end_angle - start_angle) * ((float)i / segments);
                pts[n++] = ImVec2(center.x + cosf(a) * radius, center.y + sinf(a) * radius);
            }
            dl->AddConvexPolyFilled(pts, n, IM_COL32(0, 0, 0, 160));
        }
        // Label
        if (!w.label.empty()) {
            ImVec2 ts = ImGui::CalcTextSize(w.label.c_str());
            dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f), IM_COL32(255, 255, 255, 255), w.label.c_str());
        }
        ImGui::Dummy(ImVec2(radius * 2, radius * 2));
        break;
    }

    case WidgetType::NotificationToast: {
        float opacity = w.float_val[0];
        if (opacity <= 0) opacity = 1.0f;
        if (opacity > 1) opacity = 1.0f;
        int alpha = (int)(255 * opacity);
        const char* txt = w.content.empty() ? w.label.c_str() : w.content.c_str();
        ImVec2 ts = ImGui::CalcTextSize(txt);
        float pad = 12.0f;
        float toast_w = ts.x + pad * 2;
        float toast_h = ts.y + pad * 2;
        // Position at top-right of current window content region
        float avail_w = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(avail_w - toast_w);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p, ImVec2(p.x + toast_w, p.y + toast_h), IM_COL32(50, 50, 60, alpha), 8.0f);
        dl->AddRect(p, ImVec2(p.x + toast_w, p.y + toast_h), IM_COL32(160, 160, 180, alpha), 8.0f, 0, 1.0f);
        dl->AddText(ImVec2(p.x + pad, p.y + pad), IM_COL32(255, 255, 255, alpha), txt);
        ImGui::Dummy(ImVec2(toast_w, toast_h));
        break;
    }

    case WidgetType::SkillBar: {
        int count = w.int_val[0] > 0 ? w.int_val[0] : 4;
        float icon_sz = w.size_x > 0 ? w.size_x : 48.0f;
        float pad = 6.0f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (int i = 0; i < count; i++) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            // Icon background
            dl->AddRectFilled(p, ImVec2(p.x + icon_sz, p.y + icon_sz), IM_COL32(55, 55, 65, 255), 4.0f);
            dl->AddRect(p, ImVec2(p.x + icon_sz, p.y + icon_sz), IM_COL32(130, 130, 145, 255), 4.0f, 0, 1.5f);
            // Keybind label
            if (i < (int)w.items.size() && !w.items[i].empty()) {
                ImVec2 kts = ImGui::CalcTextSize(w.items[i].c_str());
                dl->AddText(ImVec2(p.x + 2, p.y + icon_sz - kts.y - 2), IM_COL32(200, 200, 100, 255), w.items[i].c_str());
            }
            // Cooldown overlay from plot_values
            if (i < (int)w.plot_values.size() && w.plot_values[i] > 0.0f) {
                float cd = w.plot_values[i];
                if (cd > 1.0f) cd = 1.0f;
                float overlay_h = icon_sz * cd;
                dl->AddRectFilled(ImVec2(p.x, p.y + icon_sz - overlay_h),
                                  ImVec2(p.x + icon_sz, p.y + icon_sz),
                                  IM_COL32(0, 0, 0, 160), 0.0f);
            }
            // Click detection
            ImGui::SetCursorScreenPos(p);
            char btn_id[64];
            snprintf(btn_id, sizeof(btn_id), "##skill%d", i);
            if (ImGui::InvisibleButton(btn_id, ImVec2(icon_sz, icon_sz))) {
                w.selected = i;
                w.clicked = true;
                emit_event(win_id, w.id, "click", {{"slot", i}});
            }
            if (i < count - 1) ImGui::SameLine(0, pad);
        }
        break;
    }

    case WidgetType::QuestTracker: {
        float panel_w = w.size_x > 0 ? w.size_x : 250.0f;
        ImGui::BeginChild((w.id + "_quest").c_str(), ImVec2(panel_w, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
        // Header
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "%s", w.label.empty() ? "Quests" : w.label.c_str());
        ImGui::Separator();
        // Quest entries with progress bars
        for (int i = 0; i < (int)w.items.size(); i++) {
            ImGui::Text("%s", w.items[i].c_str());
            float prog = (i < (int)w.plot_values.size()) ? w.plot_values[i] : 0.0f;
            if (prog < 0) prog = 0;
            if (prog > 1) prog = 1;
            char overlay[32];
            snprintf(overlay, sizeof(overlay), "%.0f%%", prog * 100.0f);
            ImGui::ProgressBar(prog, ImVec2(-1, 0), overlay);
        }
        ImGui::EndChild();
        break;
    }

    case WidgetType::CharacterSheet: {
        if (ImGui::BeginTabBar((w.id + "_tabs").c_str())) {
            for (int t = 0; t < (int)w.headers.size(); t++) {
                if (ImGui::BeginTabItem(w.headers[t].c_str())) {
                    // Show key-value pairs from rows
                    for (int r = 0; r < (int)w.rows.size(); r++) {
                        const auto& row = w.rows[r];
                        if (row.size() >= 2) {
                            ImGui::Text("%s:", row[0].c_str());
                            ImGui::SameLine(150);
                            ImGui::Text("%s", row[1].c_str());
                        } else if (row.size() == 1) {
                            ImGui::Text("%s", row[0].c_str());
                        }
                    }
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
        break;
    }

    // ── Image / Texture ─────────────────────────────────────────────────
    case WidgetType::Image: {
        auto it = g_textures.find(w.content);
        if (it != g_textures.end() && it->second.valid) {
            ImGui::Image((ImTextureID)(intptr_t)it->second.tex_id,
                         ImVec2(w.size_x > 0 ? w.size_x : (float)it->second.width,
                                w.size_y > 0 ? w.size_y : (float)it->second.height));
        } else {
            // Placeholder colored rect
            ImVec2 sz(w.size_x > 0 ? w.size_x : 64, w.size_y > 0 ? w.size_y : 64);
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImGui::GetCursorScreenPos(),
                ImVec2(ImGui::GetCursorScreenPos().x + sz.x, ImGui::GetCursorScreenPos().y + sz.y),
                IM_COL32(100, 100, 200, 255));
            ImGui::Dummy(sz);
        }
        break;
    }

    case WidgetType::ImageButton: {
        auto it = g_textures.find(w.content);
        if (it != g_textures.end() && it->second.valid) {
            if (ImGui::ImageButton(w.id.c_str(),
                        (ImTextureID)(intptr_t)it->second.tex_id,
                        ImVec2(w.size_x > 0 ? w.size_x : (float)it->second.width,
                               w.size_y > 0 ? w.size_y : (float)it->second.height))) {
                w.clicked = true;
                emit_event(win_id, w.id, "clicked");
            }
        } else {
            // Placeholder button
            ImVec2 sz(w.size_x > 0 ? w.size_x : 64, w.size_y > 0 ? w.size_y : 64);
            if (ImGui::Button(w.label.empty() ? w.id.c_str() : w.label.c_str(), sz)) {
                w.clicked = true;
                emit_event(win_id, w.id, "clicked");
            }
        }
        break;
    }

    } // end switch

    // Post-widget item state tracking
    w.hovered = ImGui::IsItemHovered();
    w.active  = ImGui::IsItemActive();
    w.focused = ImGui::IsItemFocused();

    // Store rendered bounding box (skip layout-only widgets that don't produce items)
    if (w.type != WidgetType::Separator && w.type != WidgetType::SameLine &&
        w.type != WidgetType::NewLine && w.type != WidgetType::Spacing &&
        w.type != WidgetType::Indent && w.type != WidgetType::Unindent &&
        w.type != WidgetType::AlignTextToFramePadding) {
        ImVec2 rmin = ImGui::GetItemRectMin();
        ImVec2 rmax = ImGui::GetItemRectMax();
        w.rect_min[0] = rmin.x; w.rect_min[1] = rmin.y;
        w.rect_max[0] = rmax.x; w.rect_max[1] = rmax.y;
    }

    if (!w.enabled)
        ImGui::EndDisabled();
}
