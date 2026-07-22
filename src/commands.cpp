// commands.cpp - Command processing for the ImGui MCP application.
// Implements process_command() which handles every JSON command received
// over stdin, plus helpers for parsing widget types and widget definitions.

#include "types.h"

#include "imgui.h"
#include <SDL2/SDL.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <GL/gl.h>

// ─── Widget type parsing ────────────────────────────────────────────────────

WidgetType parse_widget_type(const std::string& s) {
    if (s == "button") return WidgetType::Button;
    if (s == "small_button") return WidgetType::SmallButton;
    if (s == "invisible_button") return WidgetType::InvisibleButton;
    if (s == "arrow_button") return WidgetType::ArrowButton;
    if (s == "checkbox") return WidgetType::Checkbox;
    if (s == "checkbox_flags") return WidgetType::CheckboxFlags;
    if (s == "radio_button") return WidgetType::RadioButton;
    if (s == "progress_bar") return WidgetType::ProgressBar;
    if (s == "bullet") return WidgetType::Bullet;
    if (s == "text_link") return WidgetType::TextLink;
    if (s == "text_link_open_url") return WidgetType::TextLinkOpenURL;
    if (s == "text") return WidgetType::Text;
    if (s == "text_colored") return WidgetType::TextColored;
    if (s == "text_disabled") return WidgetType::TextDisabled;
    if (s == "text_wrapped") return WidgetType::TextWrapped;
    if (s == "label_text") return WidgetType::LabelText;
    if (s == "bullet_text") return WidgetType::BulletText;
    if (s == "separator_text") return WidgetType::SeparatorText;
    if (s == "separator") return WidgetType::Separator;
    if (s == "same_line") return WidgetType::SameLine;
    if (s == "new_line") return WidgetType::NewLine;
    if (s == "spacing") return WidgetType::Spacing;
    if (s == "dummy") return WidgetType::Dummy;
    if (s == "indent") return WidgetType::Indent;
    if (s == "unindent") return WidgetType::Unindent;
    if (s == "align_text_to_frame_padding") return WidgetType::AlignTextToFramePadding;
    if (s == "input_text") return WidgetType::InputText;
    if (s == "input_text_multiline") return WidgetType::InputTextMultiline;
    if (s == "input_text_with_hint") return WidgetType::InputTextWithHint;
    if (s == "input_float") return WidgetType::InputFloat;
    if (s == "input_float2") return WidgetType::InputFloat2;
    if (s == "input_float3") return WidgetType::InputFloat3;
    if (s == "input_float4") return WidgetType::InputFloat4;
    if (s == "input_int") return WidgetType::InputInt;
    if (s == "input_int2") return WidgetType::InputInt2;
    if (s == "input_int3") return WidgetType::InputInt3;
    if (s == "input_int4") return WidgetType::InputInt4;
    if (s == "input_double") return WidgetType::InputDouble;
    if (s == "slider_float") return WidgetType::SliderFloat;
    if (s == "slider_float2") return WidgetType::SliderFloat2;
    if (s == "slider_float3") return WidgetType::SliderFloat3;
    if (s == "slider_float4") return WidgetType::SliderFloat4;
    if (s == "slider_int") return WidgetType::SliderInt;
    if (s == "slider_int2") return WidgetType::SliderInt2;
    if (s == "slider_int3") return WidgetType::SliderInt3;
    if (s == "slider_int4") return WidgetType::SliderInt4;
    if (s == "slider_angle") return WidgetType::SliderAngle;
    if (s == "vslider_float") return WidgetType::VSliderFloat;
    if (s == "vslider_int") return WidgetType::VSliderInt;
    if (s == "drag_float") return WidgetType::DragFloat;
    if (s == "drag_float2") return WidgetType::DragFloat2;
    if (s == "drag_float3") return WidgetType::DragFloat3;
    if (s == "drag_float4") return WidgetType::DragFloat4;
    if (s == "drag_float_range2") return WidgetType::DragFloatRange2;
    if (s == "drag_int") return WidgetType::DragInt;
    if (s == "drag_int2") return WidgetType::DragInt2;
    if (s == "drag_int3") return WidgetType::DragInt3;
    if (s == "drag_int4") return WidgetType::DragInt4;
    if (s == "drag_int_range2") return WidgetType::DragIntRange2;
    if (s == "color_edit3") return WidgetType::ColorEdit3;
    if (s == "color_edit4") return WidgetType::ColorEdit4;
    if (s == "color_picker3") return WidgetType::ColorPicker3;
    if (s == "color_picker4") return WidgetType::ColorPicker4;
    if (s == "color_button") return WidgetType::ColorButton;
    if (s == "combo") return WidgetType::Combo;
    if (s == "listbox") return WidgetType::ListBox;
    if (s == "tree_node") return WidgetType::TreeNode;
    if (s == "tree_node_ex") return WidgetType::TreeNodeEx;
    if (s == "collapsing_header") return WidgetType::CollapsingHeader;
    if (s == "selectable") return WidgetType::Selectable;
    if (s == "table") return WidgetType::Table;
    if (s == "menu_bar" || s == "menubar") return WidgetType::MenuBar;
    if (s == "main_menu_bar") return WidgetType::MainMenuBar;
    if (s == "menu") return WidgetType::Menu;
    if (s == "menu_item") return WidgetType::MenuItem;
    if (s == "popup") return WidgetType::Popup;
    if (s == "popup_modal") return WidgetType::PopupModal;
    if (s == "plot_lines") return WidgetType::PlotLines;
    if (s == "plot_histogram") return WidgetType::PlotHistogram;
    if (s == "child_window" || s == "child") return WidgetType::ChildWindow;
    if (s == "tab_bar") return WidgetType::TabBar;
    if (s == "tab_item") return WidgetType::TabItem;
    if (s == "group") return WidgetType::Group;
    if (s == "value_bool") return WidgetType::ValueBool;
    if (s == "value_int") return WidgetType::ValueInt;
    if (s == "value_float") return WidgetType::ValueFloat;
    if (s == "tooltip") return WidgetType::Tooltip;
    if (s == "set_item_tooltip") return WidgetType::SetItemTooltip;
    if (s == "draw_list" || s == "drawlist") return WidgetType::DrawList;
    if (s == "health_bar") return WidgetType::HealthBar;
    if (s == "mana_bar") return WidgetType::ManaBar;
    if (s == "inventory_grid") return WidgetType::InventoryGrid;
    if (s == "dialogue_box") return WidgetType::DialogueBox;
    if (s == "minimap") return WidgetType::Minimap;
    if (s == "tooltip_card") return WidgetType::TooltipCard;
    if (s == "cooldown_radial") return WidgetType::CooldownRadial;
    if (s == "notification_toast") return WidgetType::NotificationToast;
    if (s == "skill_bar") return WidgetType::SkillBar;
    if (s == "quest_tracker") return WidgetType::QuestTracker;
    if (s == "character_sheet") return WidgetType::CharacterSheet;
    if (s == "image") return WidgetType::Image;
    if (s == "image_button") return WidgetType::ImageButton;
    return WidgetType::Text; // fallback for unknown widget types
}

// ─── Style index parsing ────────────────────────────────────────────────────

// Normalize a style name: lowercase, CamelCase → snake_case, strip any
// "ImGuiCol_" / "ImGuiStyleVar_" prefix. "WindowBg" -> "window_bg".
static std::string norm_style_name(const std::string& in) {
    std::string s;
    s.reserve(in.size() + 4);
    for (size_t i = 0; i < in.size(); i++) {
        unsigned char ch = (unsigned char)in[i];
        if (std::isupper(ch)) {
            if (i > 0 && in[i - 1] != '_' && !std::isupper((unsigned char)in[i - 1]))
                s += '_';
            s += (char)std::tolower(ch);
        } else {
            s += (char)ch;
        }
    }
    const char* prefixes[] = {"imguicol_", "imguistylevar_"};
    for (const char* p : prefixes) {
        size_t n = std::strlen(p);
        if (s.size() > n && s.compare(0, n, p) == 0) {
            s = s.substr(n);
            break;
        }
    }
    return s;
}

// Returns -1 for unknown names. Accepts an int or a string (with or without
// the ImGuiCol_ prefix, CamelCase or snake_case).
static int parse_style_color_index(const json& v) {
    if (v.is_number_integer()) return v.get<int>();
    if (!v.is_string()) return -1;

    static const std::map<std::string, int> names = {
        {"text", ImGuiCol_Text},
        {"text_disabled", ImGuiCol_TextDisabled},
        {"window_bg", ImGuiCol_WindowBg},
        {"child_bg", ImGuiCol_ChildBg},
        {"popup_bg", ImGuiCol_PopupBg},
        {"border", ImGuiCol_Border},
        {"border_shadow", ImGuiCol_BorderShadow},
        {"frame_bg", ImGuiCol_FrameBg},
        {"frame_bg_hovered", ImGuiCol_FrameBgHovered},
        {"frame_bg_active", ImGuiCol_FrameBgActive},
        {"title_bg", ImGuiCol_TitleBg},
        {"title_bg_active", ImGuiCol_TitleBgActive},
        {"title_bg_collapsed", ImGuiCol_TitleBgCollapsed},
        {"menu_bar_bg", ImGuiCol_MenuBarBg},
        {"scrollbar_bg", ImGuiCol_ScrollbarBg},
        {"scrollbar_grab", ImGuiCol_ScrollbarGrab},
        {"scrollbar_grab_hovered", ImGuiCol_ScrollbarGrabHovered},
        {"scrollbar_grab_active", ImGuiCol_ScrollbarGrabActive},
        {"check_mark", ImGuiCol_CheckMark},
        {"checkbox_selected_bg", ImGuiCol_CheckboxSelectedBg},
        {"slider_grab", ImGuiCol_SliderGrab},
        {"slider_grab_active", ImGuiCol_SliderGrabActive},
        {"button", ImGuiCol_Button},
        {"button_hovered", ImGuiCol_ButtonHovered},
        {"button_active", ImGuiCol_ButtonActive},
        {"header", ImGuiCol_Header},
        {"header_hovered", ImGuiCol_HeaderHovered},
        {"header_active", ImGuiCol_HeaderActive},
        {"separator", ImGuiCol_Separator},
        {"separator_hovered", ImGuiCol_SeparatorHovered},
        {"separator_active", ImGuiCol_SeparatorActive},
        {"resize_grip", ImGuiCol_ResizeGrip},
        {"resize_grip_hovered", ImGuiCol_ResizeGripHovered},
        {"resize_grip_active", ImGuiCol_ResizeGripActive},
        {"input_text_cursor", ImGuiCol_InputTextCursor},
        {"tab_hovered", ImGuiCol_TabHovered},
        {"tab", ImGuiCol_Tab},
        {"tab_selected", ImGuiCol_TabSelected},
        {"tab_selected_overline", ImGuiCol_TabSelectedOverline},
        {"tab_dimmed", ImGuiCol_TabDimmed},
        {"tab_dimmed_selected", ImGuiCol_TabDimmedSelected},
        {"tab_dimmed_selected_overline", ImGuiCol_TabDimmedSelectedOverline},
        {"plot_lines", ImGuiCol_PlotLines},
        {"plot_lines_hovered", ImGuiCol_PlotLinesHovered},
        {"plot_histogram", ImGuiCol_PlotHistogram},
        {"plot_histogram_hovered", ImGuiCol_PlotHistogramHovered},
        {"table_header_bg", ImGuiCol_TableHeaderBg},
        {"table_border_strong", ImGuiCol_TableBorderStrong},
        {"table_border_light", ImGuiCol_TableBorderLight},
        {"table_row_bg", ImGuiCol_TableRowBg},
        {"table_row_bg_alt", ImGuiCol_TableRowBgAlt},
        {"text_link", ImGuiCol_TextLink},
        {"text_selected_bg", ImGuiCol_TextSelectedBg},
        {"tree_lines", ImGuiCol_TreeLines},
        {"drag_drop_target", ImGuiCol_DragDropTarget},
        {"drag_drop_target_bg", ImGuiCol_DragDropTargetBg},
        {"unsaved_marker", ImGuiCol_UnsavedMarker},
        {"nav_cursor", ImGuiCol_NavCursor},
        {"nav_windowing_highlight", ImGuiCol_NavWindowingHighlight},
        {"nav_windowing_dim_bg", ImGuiCol_NavWindowingDimBg},
        {"modal_window_dim_bg", ImGuiCol_ModalWindowDimBg},
    };
    auto it = names.find(norm_style_name(v.get<std::string>()));
    return it == names.end() ? -1 : it->second;
}

// Returns -1 for unknown names. Accepts an int or a string (with or without
// the ImGuiStyleVar_ prefix, CamelCase or snake_case).
static int parse_style_var_index(const json& v) {
    if (v.is_number_integer()) return v.get<int>();
    if (!v.is_string()) return -1;

    static const std::map<std::string, int> names = {
        {"alpha", ImGuiStyleVar_Alpha},
        {"disabled_alpha", ImGuiStyleVar_DisabledAlpha},
        {"window_padding", ImGuiStyleVar_WindowPadding},
        {"window_rounding", ImGuiStyleVar_WindowRounding},
        {"window_border_size", ImGuiStyleVar_WindowBorderSize},
        {"window_min_size", ImGuiStyleVar_WindowMinSize},
        {"window_title_align", ImGuiStyleVar_WindowTitleAlign},
        {"child_rounding", ImGuiStyleVar_ChildRounding},
        {"child_border_size", ImGuiStyleVar_ChildBorderSize},
        {"popup_rounding", ImGuiStyleVar_PopupRounding},
        {"popup_border_size", ImGuiStyleVar_PopupBorderSize},
        {"frame_padding", ImGuiStyleVar_FramePadding},
        {"frame_rounding", ImGuiStyleVar_FrameRounding},
        {"frame_border_size", ImGuiStyleVar_FrameBorderSize},
        {"item_spacing", ImGuiStyleVar_ItemSpacing},
        {"item_inner_spacing", ImGuiStyleVar_ItemInnerSpacing},
        {"indent_spacing", ImGuiStyleVar_IndentSpacing},
        {"cell_padding", ImGuiStyleVar_CellPadding},
        {"scrollbar_size", ImGuiStyleVar_ScrollbarSize},
        {"scrollbar_rounding", ImGuiStyleVar_ScrollbarRounding},
        {"scrollbar_padding", ImGuiStyleVar_ScrollbarPadding},
        {"grab_min_size", ImGuiStyleVar_GrabMinSize},
        {"grab_rounding", ImGuiStyleVar_GrabRounding},
        {"image_rounding", ImGuiStyleVar_ImageRounding},
        {"image_border_size", ImGuiStyleVar_ImageBorderSize},
        {"tab_rounding", ImGuiStyleVar_TabRounding},
        {"tab_border_size", ImGuiStyleVar_TabBorderSize},
        {"tab_min_width_base", ImGuiStyleVar_TabMinWidthBase},
        {"tab_min_width_shrink", ImGuiStyleVar_TabMinWidthShrink},
        {"tab_bar_border_size", ImGuiStyleVar_TabBarBorderSize},
        {"tab_bar_overline_size", ImGuiStyleVar_TabBarOverlineSize},
        {"table_angled_headers_angle", ImGuiStyleVar_TableAngledHeadersAngle},
        {"table_angled_headers_text_align", ImGuiStyleVar_TableAngledHeadersTextAlign},
        {"tree_lines_size", ImGuiStyleVar_TreeLinesSize},
        {"tree_lines_rounding", ImGuiStyleVar_TreeLinesRounding},
        {"drag_drop_target_rounding", ImGuiStyleVar_DragDropTargetRounding},
        {"button_text_align", ImGuiStyleVar_ButtonTextAlign},
        {"selectable_text_align", ImGuiStyleVar_SelectableTextAlign},
        {"separator_size", ImGuiStyleVar_SeparatorSize},
        {"separator_text_border_size", ImGuiStyleVar_SeparatorTextBorderSize},
        {"separator_text_align", ImGuiStyleVar_SeparatorTextAlign},
        {"separator_text_padding", ImGuiStyleVar_SeparatorTextPadding},
    };
    auto it = names.find(norm_style_name(v.get<std::string>()));
    return it == names.end() ? -1 : it->second;
}

// ─── Widget JSON parsing ────────────────────────────────────────────────────

Widget parse_widget_json(const json& j); // forward decl (defined below)

// Apply every optional field present in `j` onto widget `w`. Used both when
// creating a widget (add_widget) and when patching one (update_widget).
static void apply_widget_fields(Widget& w, const json& j) {
    // String fields
    if (j.contains("label") && j["label"].is_string()) w.label = j["label"].get<std::string>();
    if (j.contains("content") && j["content"].is_string()) w.content = j["content"].get<std::string>();
    if (j.contains("hint") && j["hint"].is_string()) w.hint = j["hint"].get<std::string>();
    if (j.contains("tooltip") && j["tooltip"].is_string()) w.tooltip = j["tooltip"].get<std::string>();
    if (j.contains("shortcut") && j["shortcut"].is_string()) w.shortcut = j["shortcut"].get<std::string>();
    if (j.contains("overlay") && j["overlay"].is_string()) w.overlay = j["overlay"].get<std::string>();
    if (j.contains("format") && j["format"].is_string()) w.format = j["format"].get<std::string>();

    // value: scalar -> float_val[0], array -> float_val[0..3]
    if (j.contains("value")) {
        const json& v = j["value"];
        if (v.is_array()) {
            for (int i = 0; i < 4 && i < (int)v.size(); i++)
                w.float_val[i] = v[i].get<float>();
        } else if (v.is_number()) {
            w.float_val[0] = v.get<float>();
        }
    }

    // values: array -> float_val[0..3] plus full plot data
    if (j.contains("values") && j["values"].is_array()) {
        const json& v = j["values"];
        for (int i = 0; i < 4 && i < (int)v.size(); i++)
            w.float_val[i] = v[i].get<float>();
        w.plot_values.clear();
        for (const auto& e : v)
            w.plot_values.push_back(e.get<float>());
    }

    // int_value: scalar -> int_val[0], array -> int_val[0..3]
    if (j.contains("int_value")) {
        const json& v = j["int_value"];
        if (v.is_array()) {
            for (int i = 0; i < 4 && i < (int)v.size(); i++)
                w.int_val[i] = v[i].get<int>();
        } else if (v.is_number()) {
            w.int_val[0] = v.get<int>();
        }
    }

    // int_values: array -> int_val[0..3]
    if (j.contains("int_values") && j["int_values"].is_array()) {
        const json& v = j["int_values"];
        for (int i = 0; i < 4 && i < (int)v.size(); i++)
            w.int_val[i] = v[i].get<int>();
    }

    if (j.contains("double_val") && j["double_val"].is_number()) w.double_val = j["double_val"].get<double>();

    if (j.contains("min") && j["min"].is_number()) w.float_min = j["min"].get<float>();
    if (j.contains("max") && j["max"].is_number()) w.float_max = j["max"].get<float>();
    if (j.contains("int_min") && j["int_min"].is_number()) w.int_min = j["int_min"].get<int>();
    if (j.contains("int_max") && j["int_max"].is_number()) w.int_max = j["int_max"].get<int>();
    if (j.contains("speed") && j["speed"].is_number()) w.float_speed = j["speed"].get<float>();

    if (j.contains("checked") && j["checked"].is_boolean()) w.bool_val = j["checked"].get<bool>();

    // text -> text_buf
    if (j.contains("text") && j["text"].is_string()) {
        std::string t = j["text"].get<std::string>();
        std::strncpy(w.text_buf, t.c_str(), sizeof(w.text_buf) - 1);
        w.text_buf[sizeof(w.text_buf) - 1] = '\0';
    }

    // color[4]
    if (j.contains("color") && j["color"].is_array()) {
        const json& c = j["color"];
        for (int i = 0; i < 4 && i < (int)c.size(); i++)
            w.color[i] = c[i].get<float>();
    }

    // items[]
    if (j.contains("items") && j["items"].is_array()) {
        w.items.clear();
        for (const auto& it : j["items"])
            w.items.push_back(it.get<std::string>());
    }

    if (j.contains("selected") && j["selected"].is_number()) w.selected = j["selected"].get<int>();
    if (j.contains("enabled") && j["enabled"].is_boolean()) w.enabled = j["enabled"].get<bool>();
    if (j.contains("flags") && j["flags"].is_number()) w.flags = j["flags"].get<int>();

    // size [x, y]
    if (j.contains("size") && j["size"].is_array() && j["size"].size() >= 2) {
        w.size_x = j["size"][0].get<float>();
        w.size_y = j["size"][1].get<float>();
    }

    if (j.contains("dir") && j["dir"].is_number()) w.dir = j["dir"].get<int>();
    if (j.contains("step") && j["step"].is_number()) w.step = j["step"].get<double>();
    if (j.contains("step_fast") && j["step_fast"].is_number()) w.step_fast = j["step_fast"].get<double>();
    if (j.contains("columns") && j["columns"].is_number()) w.columns = j["columns"].get<int>();

    // headers[]
    if (j.contains("headers") && j["headers"].is_array()) {
        w.headers.clear();
        for (const auto& h : j["headers"])
            w.headers.push_back(h.get<std::string>());
    }

    // rows[][]
    if (j.contains("rows") && j["rows"].is_array()) {
        w.rows.clear();
        for (const auto& r : j["rows"]) {
            std::vector<std::string> row;
            if (r.is_array()) {
                for (const auto& cell : r) {
                    if (cell.is_string()) row.push_back(cell.get<std::string>());
                    else row.push_back(cell.dump());
                }
            }
            w.rows.push_back(std::move(row));
        }
    }

    // children[] (recursive)
    if (j.contains("children") && j["children"].is_array()) {
        w.children.clear();
        for (const auto& c : j["children"])
            w.children.push_back(parse_widget_json(c));
    }

    if (j.contains("open") && j["open"].is_boolean()) w.tree_open = j["open"].get<bool>();
    if (j.contains("popup_open") && j["popup_open"].is_boolean()) w.popup_open = j["popup_open"].get<bool>();
}

// Parse a full widget definition from JSON (used by add_widget and the
// recursive "children" arrays).
Widget parse_widget_json(const json& j) {
    Widget w;
    w.id = j.value("id", "");
    w.type = parse_widget_type(j.value("widget_type", "text"));
    w.label = j.value("label", w.id);
    apply_widget_fields(w, j);
    return w;
}

// ─── Recursive widget lookup ────────────────────────────────────────────────

static Widget* find_in_children(std::vector<Widget>& children, const std::string& id) {
    for (auto& c : children) {
        if (c.id == id) return &c;
        if (Widget* f = find_in_children(c.children, id)) return f;
    }
    return nullptr;
}

// Find a widget by id anywhere inside a window (top-level map or nested
// children vectors).
static Widget* find_widget_in_window(WindowState& win, const std::string& id) {
    auto it = win.widgets.find(id);
    if (it != win.widgets.end()) return &it->second;
    for (auto& [wid, w] : win.widgets) {
        if (Widget* f = find_in_children(w.children, id)) return f;
    }
    return nullptr;
}

// ─── Draw command parsing ───────────────────────────────────────────────────

static int parse_draw_type(const json& v) {
    if (v.is_number_integer()) return v.get<int>();
    if (!v.is_string()) return 0;
    const std::string& s = v.get_ref<const std::string&>();
    if (s == "line") return 0;
    if (s == "rect") return 1;
    if (s == "rect_filled") return 2;
    if (s == "circle") return 3;
    if (s == "circle_filled") return 4;
    if (s == "triangle") return 5;
    if (s == "triangle_filled") return 6;
    if (s == "polyline") return 7;
    if (s == "convex_poly_filled") return 8;
    if (s == "quad") return 9;
    if (s == "quad_filled") return 10;
    if (s == "text") return 11;
    if (s == "bezier_cubic") return 12;
    if (s == "bezier_quadratic") return 13;
    return 0;
}

// Read a 2-element [x, y] array from key `key` into dst[2].
static void read_point(const json& j, const char* key, float dst[2]) {
    if (j.contains(key) && j[key].is_array() && j[key].size() >= 2) {
        dst[0] = j[key][0].get<float>();
        dst[1] = j[key][1].get<float>();
    }
}

static DrawCommand parse_draw_command(const json& j) {
    DrawCommand dc;
    if (j.contains("type")) dc.type = parse_draw_type(j["type"]);

    read_point(j, "p1", dc.p1);
    read_point(j, "p2", dc.p2);
    read_point(j, "p3", dc.p3);
    read_point(j, "p4", dc.p4);

    // "points": array of [x, y] pairs, fills p1..p4 (as many as available)
    if (j.contains("points") && j["points"].is_array()) {
        const json& pts = j["points"];
        float* dst[4] = {dc.p1, dc.p2, dc.p3, dc.p4};
        for (int i = 0; i < 4 && i < (int)pts.size(); i++) {
            if (pts[i].is_array() && pts[i].size() >= 2) {
                dst[i][0] = pts[i][0].get<float>();
                dst[i][1] = pts[i][1].get<float>();
            }
        }
    }

    if (j.contains("color") && j["color"].is_array()) {
        const json& c = j["color"];
        for (int i = 0; i < 4 && i < (int)c.size(); i++)
            dc.color[i] = c[i].get<float>();
    }
    if (j.contains("thickness") && j["thickness"].is_number()) dc.thickness = j["thickness"].get<float>();
    if (j.contains("filled") && j["filled"].is_boolean()) dc.filled = j["filled"].get<bool>();
    if (j.contains("num_segments") && j["num_segments"].is_number()) dc.num_segments = j["num_segments"].get<int>();
    if (j.contains("text") && j["text"].is_string()) dc.text = j["text"].get<std::string>();
    return dc;
}

// ─── State serialization ────────────────────────────────────────────────────

static json widget_state_json(const Widget& w) {
    json gj;
    gj["label"] = w.label;
    gj["value"] = w.float_val[0];
    gj["int_value"] = w.int_val[0];
    gj["checked"] = w.bool_val;
    gj["text"] = std::string(w.text_buf);
    gj["selected"] = w.selected;
    gj["clicked"] = w.clicked;
    gj["changed"] = w.changed;
    gj["enabled"] = w.enabled;
    gj["hovered"] = w.hovered;
    gj["active"] = w.active;
    gj["focused"] = w.focused;
    return gj;
}

// ─── Ease type parsing ──────────────────────────────────────────────────────

static EaseType parse_ease_type(const std::string& s) {
    if (s == "ease_in") return EaseType::EaseIn;
    if (s == "ease_out") return EaseType::EaseOut;
    if (s == "ease_in_out") return EaseType::EaseInOut;
    if (s == "bounce") return EaseType::Bounce;
    if (s == "elastic") return EaseType::Elastic;
    if (s == "back") return EaseType::Back;
    return EaseType::Linear;
}


// ─── Key name to ImGuiKey mapping ──────────────────────────────────────────

static ImGuiKey parse_key_name(const std::string& name) {
    // Handle modifier combos like "Ctrl+S" - return the main key
    std::string key = name;
    // Strip modifier prefixes for the main key lookup
    auto pos = key.rfind('+');
    if (pos != std::string::npos && pos + 1 < key.size())
        key = key.substr(pos + 1);

    if (key == "Tab")        return ImGuiKey_Tab;
    if (key == "Enter" || key == "Return") return ImGuiKey_Enter;
    if (key == "Escape" || key == "Esc")   return ImGuiKey_Escape;
    if (key == "Space")      return ImGuiKey_Space;
    if (key == "Backspace")  return ImGuiKey_Backspace;
    if (key == "Delete" || key == "Del")   return ImGuiKey_Delete;
    if (key == "Insert")     return ImGuiKey_Insert;
    if (key == "Home")       return ImGuiKey_Home;
    if (key == "End")        return ImGuiKey_End;
    if (key == "PageUp" || key == "Prior") return ImGuiKey_PageUp;
    if (key == "PageDown" || key == "Next") return ImGuiKey_PageDown;
    if (key == "Up")         return ImGuiKey_UpArrow;
    if (key == "Down")       return ImGuiKey_DownArrow;
    if (key == "Left")       return ImGuiKey_LeftArrow;
    if (key == "Right")      return ImGuiKey_RightArrow;
    if (key == "A")          return ImGuiKey_A;
    if (key == "B")          return ImGuiKey_B;
    if (key == "C")          return ImGuiKey_C;
    if (key == "D")          return ImGuiKey_D;
    if (key == "E")          return ImGuiKey_E;
    if (key == "F")          return ImGuiKey_F;
    if (key == "G")          return ImGuiKey_G;
    if (key == "H")          return ImGuiKey_H;
    if (key == "I")          return ImGuiKey_I;
    if (key == "J")          return ImGuiKey_J;
    if (key == "K")          return ImGuiKey_K;
    if (key == "L")          return ImGuiKey_L;
    if (key == "M")          return ImGuiKey_M;
    if (key == "N")          return ImGuiKey_N;
    if (key == "O")          return ImGuiKey_O;
    if (key == "P")          return ImGuiKey_P;
    if (key == "Q")          return ImGuiKey_Q;
    if (key == "R")          return ImGuiKey_R;
    if (key == "S")          return ImGuiKey_S;
    if (key == "T")          return ImGuiKey_T;
    if (key == "U")          return ImGuiKey_U;
    if (key == "V")          return ImGuiKey_V;
    if (key == "W")          return ImGuiKey_W;
    if (key == "X")          return ImGuiKey_X;
    if (key == "Y")          return ImGuiKey_Y;
    if (key == "Z")          return ImGuiKey_Z;
    if (key == "0")          return ImGuiKey_0;
    if (key == "1")          return ImGuiKey_1;
    if (key == "2")          return ImGuiKey_2;
    if (key == "3")          return ImGuiKey_3;
    if (key == "4")          return ImGuiKey_4;
    if (key == "5")          return ImGuiKey_5;
    if (key == "6")          return ImGuiKey_6;
    if (key == "7")          return ImGuiKey_7;
    if (key == "8")          return ImGuiKey_8;
    if (key == "9")          return ImGuiKey_9;
    if (key == "F1")         return ImGuiKey_F1;
    if (key == "F2")         return ImGuiKey_F2;
    if (key == "F3")         return ImGuiKey_F3;
    if (key == "F4")         return ImGuiKey_F4;
    if (key == "F5")         return ImGuiKey_F5;
    if (key == "F6")         return ImGuiKey_F6;
    if (key == "F7")         return ImGuiKey_F7;
    if (key == "F8")         return ImGuiKey_F8;
    if (key == "F9")         return ImGuiKey_F9;
    if (key == "F10")        return ImGuiKey_F10;
    if (key == "F11")        return ImGuiKey_F11;
    if (key == "F12")        return ImGuiKey_F12;
    return ImGuiKey_None;
}

static bool parse_modifier(const std::string& name, const std::string& mod) {
    return name.find(mod + "+") != std::string::npos;
}

// ─── Full state serialization for undo/redo & snapshots ─────────────────────

static std::string widget_type_to_string(WidgetType t) {
    switch (t) {
        case WidgetType::Button: return "button";
        case WidgetType::SmallButton: return "small_button";
        case WidgetType::InvisibleButton: return "invisible_button";
        case WidgetType::ArrowButton: return "arrow_button";
        case WidgetType::Checkbox: return "checkbox";
        case WidgetType::CheckboxFlags: return "checkbox_flags";
        case WidgetType::RadioButton: return "radio_button";
        case WidgetType::ProgressBar: return "progress_bar";
        case WidgetType::Bullet: return "bullet";
        case WidgetType::TextLink: return "text_link";
        case WidgetType::TextLinkOpenURL: return "text_link_open_url";
        case WidgetType::Text: return "text";
        case WidgetType::TextColored: return "text_colored";
        case WidgetType::TextDisabled: return "text_disabled";
        case WidgetType::TextWrapped: return "text_wrapped";
        case WidgetType::LabelText: return "label_text";
        case WidgetType::BulletText: return "bullet_text";
        case WidgetType::SeparatorText: return "separator_text";
        case WidgetType::Separator: return "separator";
        case WidgetType::SameLine: return "same_line";
        case WidgetType::NewLine: return "new_line";
        case WidgetType::Spacing: return "spacing";
        case WidgetType::Dummy: return "dummy";
        case WidgetType::Indent: return "indent";
        case WidgetType::Unindent: return "unindent";
        case WidgetType::AlignTextToFramePadding: return "align_text_to_frame_padding";
        case WidgetType::InputText: return "input_text";
        case WidgetType::InputTextMultiline: return "input_text_multiline";
        case WidgetType::InputTextWithHint: return "input_text_with_hint";
        case WidgetType::InputFloat: return "input_float";
        case WidgetType::InputFloat2: return "input_float2";
        case WidgetType::InputFloat3: return "input_float3";
        case WidgetType::InputFloat4: return "input_float4";
        case WidgetType::InputInt: return "input_int";
        case WidgetType::InputInt2: return "input_int2";
        case WidgetType::InputInt3: return "input_int3";
        case WidgetType::InputInt4: return "input_int4";
        case WidgetType::InputDouble: return "input_double";
        case WidgetType::SliderFloat: return "slider_float";
        case WidgetType::SliderFloat2: return "slider_float2";
        case WidgetType::SliderFloat3: return "slider_float3";
        case WidgetType::SliderFloat4: return "slider_float4";
        case WidgetType::SliderInt: return "slider_int";
        case WidgetType::SliderInt2: return "slider_int2";
        case WidgetType::SliderInt3: return "slider_int3";
        case WidgetType::SliderInt4: return "slider_int4";
        case WidgetType::SliderAngle: return "slider_angle";
        case WidgetType::VSliderFloat: return "vslider_float";
        case WidgetType::VSliderInt: return "vslider_int";
        case WidgetType::DragFloat: return "drag_float";
        case WidgetType::DragFloat2: return "drag_float2";
        case WidgetType::DragFloat3: return "drag_float3";
        case WidgetType::DragFloat4: return "drag_float4";
        case WidgetType::DragFloatRange2: return "drag_float_range2";
        case WidgetType::DragInt: return "drag_int";
        case WidgetType::DragInt2: return "drag_int2";
        case WidgetType::DragInt3: return "drag_int3";
        case WidgetType::DragInt4: return "drag_int4";
        case WidgetType::DragIntRange2: return "drag_int_range2";
        case WidgetType::ColorEdit3: return "color_edit3";
        case WidgetType::ColorEdit4: return "color_edit4";
        case WidgetType::ColorPicker3: return "color_picker3";
        case WidgetType::ColorPicker4: return "color_picker4";
        case WidgetType::ColorButton: return "color_button";
        case WidgetType::Combo: return "combo";
        case WidgetType::ListBox: return "listbox";
        case WidgetType::TreeNode: return "tree_node";
        case WidgetType::TreeNodeEx: return "tree_node_ex";
        case WidgetType::CollapsingHeader: return "collapsing_header";
        case WidgetType::Selectable: return "selectable";
        case WidgetType::Table: return "table";
        case WidgetType::MenuBar: return "menu_bar";
        case WidgetType::MainMenuBar: return "main_menu_bar";
        case WidgetType::Menu: return "menu";
        case WidgetType::MenuItem: return "menu_item";
        case WidgetType::Popup: return "popup";
        case WidgetType::PopupModal: return "popup_modal";
        case WidgetType::PlotLines: return "plot_lines";
        case WidgetType::PlotHistogram: return "plot_histogram";
        case WidgetType::ChildWindow: return "child_window";
        case WidgetType::TabBar: return "tab_bar";
        case WidgetType::TabItem: return "tab_item";
        case WidgetType::Group: return "group";
        case WidgetType::ValueBool: return "value_bool";
        case WidgetType::ValueInt: return "value_int";
        case WidgetType::ValueFloat: return "value_float";
        case WidgetType::Tooltip: return "tooltip";
        case WidgetType::SetItemTooltip: return "set_item_tooltip";
        case WidgetType::DrawList: return "draw_list";
        case WidgetType::HealthBar: return "health_bar";
        case WidgetType::ManaBar: return "mana_bar";
        case WidgetType::InventoryGrid: return "inventory_grid";
        case WidgetType::DialogueBox: return "dialogue_box";
        case WidgetType::Minimap: return "minimap";
        case WidgetType::TooltipCard: return "tooltip_card";
        case WidgetType::CooldownRadial: return "cooldown_radial";
        case WidgetType::NotificationToast: return "notification_toast";
        case WidgetType::SkillBar: return "skill_bar";
        case WidgetType::QuestTracker: return "quest_tracker";
        case WidgetType::CharacterSheet: return "character_sheet";
    }
    return "text";
}

// Serialize a widget's full definition (all fields needed to recreate it).
static json serialize_widget_full(const Widget& w) {
    json j;
    j["id"] = w.id;
    j["widget_type"] = widget_type_to_string(w.type);
    j["label"] = w.label;
    j["content"] = w.content;
    j["hint"] = w.hint;
    j["tooltip"] = w.tooltip;
    j["shortcut"] = w.shortcut;
    j["overlay"] = w.overlay;
    j["format"] = w.format;
    j["value"] = {w.float_val[0], w.float_val[1], w.float_val[2], w.float_val[3]};
    j["min"] = w.float_min;
    j["max"] = w.float_max;
    j["speed"] = w.float_speed;
    j["int_value"] = {w.int_val[0], w.int_val[1], w.int_val[2], w.int_val[3]};
    j["int_min"] = w.int_min;
    j["int_max"] = w.int_max;
    j["double_val"] = w.double_val;
    j["checked"] = w.bool_val;
    j["text"] = std::string(w.text_buf);
    j["color"] = {w.color[0], w.color[1], w.color[2], w.color[3]};
    j["items"] = w.items;
    j["selected"] = w.selected;
    j["plot_values"] = w.plot_values;
    j["columns"] = w.columns;
    j["headers"] = w.headers;
    j["rows"] = w.rows;
    j["flags"] = w.flags;
    j["enabled"] = w.enabled;
    j["open"] = w.tree_open;
    j["popup_open"] = w.popup_open;
    j["size"] = {w.size_x, w.size_y};
    j["dir"] = w.dir;
    j["step"] = w.step;
    j["step_fast"] = w.step_fast;
    j["cursor_offset"] = {w.cursor_offset[0], w.cursor_offset[1]};
    // Children (recursive)
    if (!w.children.empty()) {
        j["children"] = json::array();
        for (const auto& c : w.children)
            j["children"].push_back(serialize_widget_full(c));
    }
    // Draw commands
    if (!w.draw_commands.empty()) {
        j["draw_commands"] = json::array();
        for (const auto& dc : w.draw_commands) {
            json dj;
            dj["type"] = dc.type;
            dj["p1"] = {dc.p1[0], dc.p1[1]};
            dj["p2"] = {dc.p2[0], dc.p2[1]};
            dj["p3"] = {dc.p3[0], dc.p3[1]};
            dj["p4"] = {dc.p4[0], dc.p4[1]};
            dj["color"] = {dc.color[0], dc.color[1], dc.color[2], dc.color[3]};
            dj["thickness"] = dc.thickness;
            dj["filled"] = dc.filled;
            dj["num_segments"] = dc.num_segments;
            dj["text"] = dc.text;
            j["draw_commands"].push_back(dj);
        }
    }
    return j;
}

// Deserialize a widget from its full serialization (inverse of serialize_widget_full).
static Widget deserialize_widget_full(const json& j) {
    Widget w;
    w.id = j.value("id", "");
    w.type = parse_widget_type(j.value("widget_type", "text"));
    w.label = j.value("label", w.id);
    w.content = j.value("content", "");
    w.hint = j.value("hint", "");
    w.tooltip = j.value("tooltip", "");
    w.shortcut = j.value("shortcut", "");
    w.overlay = j.value("overlay", "");
    w.format = j.value("format", "");
    if (j.contains("value") && j["value"].is_array()) {
        for (int i = 0; i < 4 && i < (int)j["value"].size(); i++)
            w.float_val[i] = j["value"][i].get<float>();
    }
    w.float_min = j.value("min", 0.0f);
    w.float_max = j.value("max", 1.0f);
    w.float_speed = j.value("speed", 1.0f);
    if (j.contains("int_value") && j["int_value"].is_array()) {
        for (int i = 0; i < 4 && i < (int)j["int_value"].size(); i++)
            w.int_val[i] = j["int_value"][i].get<int>();
    }
    w.int_min = j.value("int_min", 0);
    w.int_max = j.value("int_max", 100);
    w.double_val = j.value("double_val", 0.0);
    w.bool_val = j.value("checked", false);
    if (j.contains("text") && j["text"].is_string()) {
        std::string t = j["text"].get<std::string>();
        std::strncpy(w.text_buf, t.c_str(), sizeof(w.text_buf) - 1);
        w.text_buf[sizeof(w.text_buf) - 1] = '\0';
    }
    if (j.contains("color") && j["color"].is_array()) {
        for (int i = 0; i < 4 && i < (int)j["color"].size(); i++)
            w.color[i] = j["color"][i].get<float>();
    }
    if (j.contains("items") && j["items"].is_array()) {
        w.items.clear();
        for (const auto& it : j["items"])
            w.items.push_back(it.get<std::string>());
    }
    w.selected = j.value("selected", -1);
    if (j.contains("plot_values") && j["plot_values"].is_array()) {
        w.plot_values.clear();
        for (const auto& v : j["plot_values"])
            w.plot_values.push_back(v.get<float>());
    }
    w.columns = j.value("columns", 0);
    if (j.contains("headers") && j["headers"].is_array()) {
        w.headers.clear();
        for (const auto& h : j["headers"])
            w.headers.push_back(h.get<std::string>());
    }
    if (j.contains("rows") && j["rows"].is_array()) {
        w.rows.clear();
        for (const auto& r : j["rows"]) {
            std::vector<std::string> row;
            if (r.is_array())
                for (const auto& cell : r)
                    row.push_back(cell.is_string() ? cell.get<std::string>() : cell.dump());
            w.rows.push_back(std::move(row));
        }
    }
    w.flags = j.value("flags", 0);
    w.enabled = j.value("enabled", true);
    w.tree_open = j.value("open", false);
    w.popup_open = j.value("popup_open", false);
    if (j.contains("size") && j["size"].is_array() && j["size"].size() >= 2) {
        w.size_x = j["size"][0].get<float>();
        w.size_y = j["size"][1].get<float>();
    }
    w.dir = j.value("dir", 0);
    w.step = j.value("step", 0.0);
    w.step_fast = j.value("step_fast", 0.0);
    if (j.contains("cursor_offset") && j["cursor_offset"].is_array() && j["cursor_offset"].size() >= 2) {
        w.cursor_offset[0] = j["cursor_offset"][0].get<float>();
        w.cursor_offset[1] = j["cursor_offset"][1].get<float>();
    }
    if (j.contains("children") && j["children"].is_array()) {
        w.children.clear();
        for (const auto& c : j["children"])
            w.children.push_back(deserialize_widget_full(c));
    }
    if (j.contains("draw_commands") && j["draw_commands"].is_array()) {
        w.draw_commands.clear();
        for (const auto& dj : j["draw_commands"]) {
            DrawCommand dc;
            dc.type = dj.value("type", 0);
            if (dj.contains("p1")) { dc.p1[0] = dj["p1"][0].get<float>(); dc.p1[1] = dj["p1"][1].get<float>(); }
            if (dj.contains("p2")) { dc.p2[0] = dj["p2"][0].get<float>(); dc.p2[1] = dj["p2"][1].get<float>(); }
            if (dj.contains("p3")) { dc.p3[0] = dj["p3"][0].get<float>(); dc.p3[1] = dj["p3"][1].get<float>(); }
            if (dj.contains("p4")) { dc.p4[0] = dj["p4"][0].get<float>(); dc.p4[1] = dj["p4"][1].get<float>(); }
            if (dj.contains("color")) { for (int i = 0; i < 4 && i < (int)dj["color"].size(); i++) dc.color[i] = dj["color"][i].get<float>(); }
            dc.thickness = dj.value("thickness", 1.0f);
            dc.filled = dj.value("filled", false);
            dc.num_segments = dj.value("num_segments", 0);
            dc.text = dj.value("text", "");
            w.draw_commands.push_back(dc);
        }
    }
    return w;
}

// Serialize the entire layout state (all windows and widgets).
// Caller MUST hold g_mutex.
static json serialize_state() {
    json state = json::object();
    state["window_order"] = g_window_order;
    state["windows"] = json::object();
    for (auto& [wid, win] : g_windows) {
        json wj;
        wj["id"] = win.id;
        wj["title"] = win.title;
        wj["x"] = win.x;
        wj["y"] = win.y;
        wj["w"] = win.w;
        wj["h"] = win.h;
        wj["open"] = win.open;
        wj["collapsed"] = win.collapsed;
        wj["flags"] = win.flags;
        wj["has_menubar"] = win.has_menubar;
        wj["widget_order"] = win.widget_order;
        wj["widgets"] = json::object();
        for (auto& [gid, w] : win.widgets)
            wj["widgets"][gid] = serialize_widget_full(w);
        state["windows"][wid] = wj;
    }
    return state;
}

// Restore the entire layout state from a serialized snapshot.
// Caller MUST hold g_mutex.
static void restore_state(const json& state) {
    g_windows.clear();
    g_window_order.clear();
    if (state.contains("window_order") && state["window_order"].is_array()) {
        for (const auto& wid : state["window_order"])
            g_window_order.push_back(wid.get<std::string>());
    }
    if (state.contains("windows") && state["windows"].is_object()) {
        for (auto& [wid, wj] : state["windows"].items()) {
            WindowState win;
            win.id = wj.value("id", wid);
            win.title = wj.value("title", wid);
            win.x = wj.value("x", 100.0f);
            win.y = wj.value("y", 100.0f);
            win.w = wj.value("w", 400.0f);
            win.h = wj.value("h", 300.0f);
            win.open = wj.value("open", true);
            win.collapsed = wj.value("collapsed", false);
            win.flags = wj.value("flags", 0);
            win.has_menubar = wj.value("has_menubar", false);
            if (wj.contains("widget_order") && wj["widget_order"].is_array()) {
                for (const auto& gid : wj["widget_order"])
                    win.widget_order.push_back(gid.get<std::string>());
            }
            if (wj.contains("widgets") && wj["widgets"].is_object()) {
                for (auto& [gid, wjson] : wj["widgets"].items())
                    win.widgets[gid] = deserialize_widget_full(wjson);
            }
            g_windows[wid] = std::move(win);
        }
    }
}

// Push current state onto the undo stack. Caller MUST hold g_mutex.
static void push_undo_state() {
    g_undo_stack.push_back(serialize_state());
    if ((int)g_undo_stack.size() > g_max_undo)
        g_undo_stack.erase(g_undo_stack.begin());
    g_redo_stack.clear(); // new action invalidates redo history
}
// ─── Anchor string parsing ─────────────────────────────────────────────────

static Anchor parse_anchor(const std::string& s) {
    if (s == "top_left")      return Anchor::TopLeft;
    if (s == "top_center")    return Anchor::TopCenter;
    if (s == "top_right")     return Anchor::TopRight;
    if (s == "center_left")   return Anchor::CenterLeft;
    if (s == "center")        return Anchor::Center;
    if (s == "center_right")  return Anchor::CenterRight;
    if (s == "bottom_left")   return Anchor::BottomLeft;
    if (s == "bottom_center") return Anchor::BottomCenter;
    if (s == "bottom_right")  return Anchor::BottomRight;
    return Anchor::None;
}

// ─── Export / Import helpers ─────────────────────────────────────────────────

// Make a valid C++ identifier from a widget id (replace non-alnum with _).
static std::string sanitize_id(const std::string& id) {
    std::string out;
    for (char c : id) {
        out += (std::isalnum(static_cast<unsigned char>(c)) || c == '_') ? c : '_';
    }
    if (!out.empty() && std::isdigit(static_cast<unsigned char>(out[0])))
        out = "_" + out;
    return out.empty() ? "w" : out;
}

// Escape a string for use inside C++ double-quoted string literals.
static std::string cpp_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

// Escape a string for use inside Lua double-quoted string literals.
static std::string lua_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

// Generate C++ imgui code for a single widget.
static void generate_cpp_widget(std::ostringstream& ss, const Widget& w, int indent) {
    std::string pad(indent * 4, ' ');
    std::string sid = sanitize_id(w.id);
    std::string label = cpp_escape(w.label.empty() ? w.id : w.label);

    switch (w.type) {
        case WidgetType::Text:
            ss << pad << "ImGui::Text(\"" << label << "\");\n";
            break;
        case WidgetType::TextColored:
            ss << pad << "ImGui::TextColored(ImVec4(" << w.color[0] << "f, " << w.color[1]
               << "f, " << w.color[2] << "f, " << w.color[3] << "f), \"" << label << "\");\n";
            break;
        case WidgetType::TextDisabled:
            ss << pad << "ImGui::TextDisabled(\"" << label << "\");\n";
            break;
        case WidgetType::TextWrapped:
            ss << pad << "ImGui::TextWrapped(\"" << label << "\");\n";
            break;
        case WidgetType::LabelText:
            ss << pad << "ImGui::LabelText(\"" << cpp_escape(w.hint) << "\", \"" << label << "\");\n";
            break;
        case WidgetType::BulletText:
            ss << pad << "ImGui::BulletText(\"" << label << "\");\n";
            break;
        case WidgetType::SeparatorText:
            ss << pad << "ImGui::SeparatorText(\"" << label << "\");\n";
            break;
        case WidgetType::Button:
            ss << pad << "if (ImGui::Button(\"" << label << "\"";
            if (w.size_x > 0 || w.size_y > 0)
                ss << ", ImVec2(" << w.size_x << "f, " << w.size_y << "f)";
            ss << ")) { /* handle */ }\n";
            break;
        case WidgetType::SmallButton:
            ss << pad << "if (ImGui::SmallButton(\"" << label << "\")) { /* handle */ }\n";
            break;
        case WidgetType::ArrowButton:
            ss << pad << "if (ImGui::ArrowButton(\"" << sid << "\", ImGuiDir_"
               << (w.dir == 0 ? "Left" : w.dir == 1 ? "Right" : w.dir == 2 ? "Up" : "Down")
               << ")) { /* handle */ }\n";
            break;
        case WidgetType::Checkbox:
            ss << pad << "static bool " << sid << " = " << (w.bool_val ? "true" : "false") << ";\n";
            ss << pad << "ImGui::Checkbox(\"" << label << "\", &" << sid << ");\n";
            break;
        case WidgetType::RadioButton:
            ss << pad << "static int " << sid << " = " << w.int_val[0] << ";\n";
            ss << pad << "ImGui::RadioButton(\"" << label << "\", &" << sid << ", " << w.int_val[0] << ");\n";
            break;
        case WidgetType::ProgressBar:
            ss << pad << "ImGui::ProgressBar(" << w.float_val[0] << "f";
            if (w.size_x > 0 || w.size_y > 0)
                ss << ", ImVec2(" << w.size_x << "f, " << w.size_y << "f)";
            if (!w.overlay.empty())
                ss << ", \"" << cpp_escape(w.overlay) << "\"";
            ss << ");\n";
            break;
        case WidgetType::Bullet:
            ss << pad << "ImGui::Bullet();\n";
            break;
        case WidgetType::Separator:
            ss << pad << "ImGui::Separator();\n";
            break;
        case WidgetType::SameLine:
            ss << pad << "ImGui::SameLine();\n";
            break;
        case WidgetType::NewLine:
            ss << pad << "ImGui::NewLine();\n";
            break;
        case WidgetType::Spacing:
            ss << pad << "ImGui::Spacing();\n";
            break;
        case WidgetType::Dummy:
            ss << pad << "ImGui::Dummy(ImVec2(" << w.size_x << "f, " << w.size_y << "f));\n";
            break;
        case WidgetType::Indent:
            ss << pad << "ImGui::Indent();\n";
            break;
        case WidgetType::Unindent:
            ss << pad << "ImGui::Unindent();\n";
            break;
        case WidgetType::AlignTextToFramePadding:
            ss << pad << "ImGui::AlignTextToFramePadding();\n";
            break;
        case WidgetType::SliderFloat:
            ss << pad << "static float " << sid << " = " << w.float_val[0] << "f;\n";
            ss << pad << "ImGui::SliderFloat(\"" << label << "\", &" << sid << ", "
               << w.float_min << "f, " << w.float_max << "f);\n";
            break;
        case WidgetType::SliderFloat2:
            ss << pad << "static float " << sid << "[2] = {" << w.float_val[0] << "f, " << w.float_val[1] << "f};\n";
            ss << pad << "ImGui::SliderFloat2(\"" << label << "\", " << sid << ", "
               << w.float_min << "f, " << w.float_max << "f);\n";
            break;
        case WidgetType::SliderFloat3:
            ss << pad << "static float " << sid << "[3] = {" << w.float_val[0] << "f, " << w.float_val[1]
               << "f, " << w.float_val[2] << "f};\n";
            ss << pad << "ImGui::SliderFloat3(\"" << label << "\", " << sid << ", "
               << w.float_min << "f, " << w.float_max << "f);\n";
            break;
        case WidgetType::SliderInt:
            ss << pad << "static int " << sid << " = " << w.int_val[0] << ";\n";
            ss << pad << "ImGui::SliderInt(\"" << label << "\", &" << sid << ", "
               << w.int_min << ", " << w.int_max << ");\n";
            break;
        case WidgetType::SliderInt2:
            ss << pad << "static int " << sid << "[2] = {" << w.int_val[0] << ", " << w.int_val[1] << "};\n";
            ss << pad << "ImGui::SliderInt2(\"" << label << "\", " << sid << ", "
               << w.int_min << ", " << w.int_max << ");\n";
            break;
        case WidgetType::SliderInt3:
            ss << pad << "static int " << sid << "[3] = {" << w.int_val[0] << ", " << w.int_val[1]
               << ", " << w.int_val[2] << "};\n";
            ss << pad << "ImGui::SliderInt3(\"" << label << "\", " << sid << ", "
               << w.int_min << ", " << w.int_max << ");\n";
            break;
        case WidgetType::DragFloat:
            ss << pad << "static float " << sid << " = " << w.float_val[0] << "f;\n";
            ss << pad << "ImGui::DragFloat(\"" << label << "\", &" << sid << ", "
               << w.float_speed << "f, " << w.float_min << "f, " << w.float_max << "f);\n";
            break;
        case WidgetType::DragInt:
            ss << pad << "static int " << sid << " = " << w.int_val[0] << ";\n";
            ss << pad << "ImGui::DragInt(\"" << label << "\", &" << sid << ", "
               << w.float_speed << "f, " << w.int_min << ", " << w.int_max << ");\n";
            break;
        case WidgetType::InputText:
            ss << pad << "static char " << sid << "[256] = \"" << cpp_escape(std::string(w.text_buf)) << "\";\n";
            ss << pad << "ImGui::InputText(\"" << label << "\", " << sid << ", sizeof(" << sid << "));\n";
            break;
        case WidgetType::InputTextWithHint:
            ss << pad << "static char " << sid << "[256] = \"" << cpp_escape(std::string(w.text_buf)) << "\";\n";
            ss << pad << "ImGui::InputTextWithHint(\"" << label << "\", \"" << cpp_escape(w.hint)
               << "\", " << sid << ", sizeof(" << sid << "));\n";
            break;
        case WidgetType::InputTextMultiline:
            ss << pad << "static char " << sid << "[4096] = \"" << cpp_escape(std::string(w.text_buf)) << "\";\n";
            ss << pad << "ImGui::InputTextMultiline(\"" << label << "\", " << sid << ", sizeof(" << sid << "));\n";
            break;
        case WidgetType::InputFloat:
            ss << pad << "static float " << sid << " = " << w.float_val[0] << "f;\n";
            ss << pad << "ImGui::InputFloat(\"" << label << "\", &" << sid << ");\n";
            break;
        case WidgetType::InputInt:
            ss << pad << "static int " << sid << " = " << w.int_val[0] << ";\n";
            ss << pad << "ImGui::InputInt(\"" << label << "\", &" << sid << ");\n";
            break;
        case WidgetType::InputDouble:
            ss << pad << "static double " << sid << " = " << w.double_val << ";\n";
            ss << pad << "ImGui::InputDouble(\"" << label << "\", &" << sid << ");\n";
            break;
        case WidgetType::ColorEdit3:
            ss << pad << "static float " << sid << "[3] = {" << w.color[0] << "f, "
               << w.color[1] << "f, " << w.color[2] << "f};\n";
            ss << pad << "ImGui::ColorEdit3(\"" << label << "\", " << sid << ");\n";
            break;
        case WidgetType::ColorEdit4:
            ss << pad << "static float " << sid << "[4] = {" << w.color[0] << "f, " << w.color[1]
               << "f, " << w.color[2] << "f, " << w.color[3] << "f};\n";
            ss << pad << "ImGui::ColorEdit4(\"" << label << "\", " << sid << ");\n";
            break;
        case WidgetType::ColorPicker3:
            ss << pad << "static float " << sid << "[3] = {" << w.color[0] << "f, "
               << w.color[1] << "f, " << w.color[2] << "f};\n";
            ss << pad << "ImGui::ColorPicker3(\"" << label << "\", " << sid << ");\n";
            break;
        case WidgetType::ColorPicker4:
            ss << pad << "static float " << sid << "[4] = {" << w.color[0] << "f, " << w.color[1]
               << "f, " << w.color[2] << "f, " << w.color[3] << "f};\n";
            ss << pad << "ImGui::ColorPicker4(\"" << label << "\", " << sid << ");\n";
            break;
        case WidgetType::Combo: {
            ss << pad << "static int " << sid << " = " << w.selected << ";\n";
            ss << pad << "const char* " << sid << "_items[] = {";
            for (size_t i = 0; i < w.items.size(); i++) {
                if (i > 0) ss << ", ";
                ss << "\"" << cpp_escape(w.items[i]) << "\"";
            }
            ss << "};\n";
            ss << pad << "ImGui::Combo(\"" << label << "\", &" << sid << ", " << sid << "_items, "
               << w.items.size() << ");\n";
            break;
        }
        case WidgetType::ListBox: {
            ss << pad << "static int " << sid << " = " << w.selected << ";\n";
            ss << pad << "const char* " << sid << "_items[] = {";
            for (size_t i = 0; i < w.items.size(); i++) {
                if (i > 0) ss << ", ";
                ss << "\"" << cpp_escape(w.items[i]) << "\"";
            }
            ss << "};\n";
            ss << pad << "ImGui::ListBox(\"" << label << "\", &" << sid << ", " << sid << "_items, "
               << w.items.size() << ");\n";
            break;
        }
        case WidgetType::Selectable:
            ss << pad << "static bool " << sid << " = " << (w.bool_val ? "true" : "false") << ";\n";
            ss << pad << "if (ImGui::Selectable(\"" << label << "\", &" << sid << ")) { /* handle */ }\n";
            break;
        case WidgetType::TreeNode:
            ss << pad << "if (ImGui::TreeNode(\"" << label << "\")) {\n";
            for (auto& child : w.children)
                generate_cpp_widget(ss, child, indent + 1);
            ss << pad << "    ImGui::TreePop();\n";
            ss << pad << "}\n";
            break;
        case WidgetType::CollapsingHeader:
            ss << pad << "if (ImGui::CollapsingHeader(\"" << label << "\")) {\n";
            for (auto& child : w.children)
                generate_cpp_widget(ss, child, indent + 1);
            ss << pad << "}\n";
            break;
        case WidgetType::TabBar:
            ss << pad << "if (ImGui::BeginTabBar(\"" << sid << "\")) {\n";
            for (auto& child : w.children)
                generate_cpp_widget(ss, child, indent + 1);
            ss << pad << "    ImGui::EndTabBar();\n";
            ss << pad << "}\n";
            break;
        case WidgetType::TabItem:
            ss << pad << "if (ImGui::BeginTabItem(\"" << label << "\")) {\n";
            for (auto& child : w.children)
                generate_cpp_widget(ss, child, indent + 1);
            ss << pad << "    ImGui::EndTabItem();\n";
            ss << pad << "}\n";
            break;
        case WidgetType::ChildWindow:
            ss << pad << "if (ImGui::BeginChild(\"" << sid << "\", ImVec2("
               << w.size_x << "f, " << w.size_y << "f))) {\n";
            for (auto& child : w.children)
                generate_cpp_widget(ss, child, indent + 1);
            ss << pad << "    ImGui::EndChild();\n";
            ss << pad << "}\n";
            break;
        case WidgetType::Group:
            ss << pad << "ImGui::BeginGroup();\n";
            for (auto& child : w.children)
                generate_cpp_widget(ss, child, indent + 1);
            ss << pad << "ImGui::EndGroup();\n";
            break;
        case WidgetType::PlotLines: {
            ss << pad << "static float " << sid << "_data[] = {";
            for (size_t i = 0; i < w.plot_values.size(); i++) {
                if (i > 0) ss << ", ";
                ss << w.plot_values[i] << "f";
            }
            ss << "};\n";
            ss << pad << "ImGui::PlotLines(\"" << label << "\", " << sid << "_data, "
               << w.plot_values.size() << ");\n";
            break;
        }
        case WidgetType::PlotHistogram: {
            ss << pad << "static float " << sid << "_data[] = {";
            for (size_t i = 0; i < w.plot_values.size(); i++) {
                if (i > 0) ss << ", ";
                ss << w.plot_values[i] << "f";
            }
            ss << "};\n";
            ss << pad << "ImGui::PlotHistogram(\"" << label << "\", " << sid << "_data, "
               << w.plot_values.size() << ");\n";
            break;
        }
        case WidgetType::MenuItem:
            ss << pad << "if (ImGui::MenuItem(\"" << label << "\")) { /* handle */ }\n";
            break;
        case WidgetType::Tooltip:
        case WidgetType::SetItemTooltip:
            ss << pad << "if (ImGui::IsItemHovered()) {\n";
            ss << pad << "    ImGui::SetTooltip(\"" << cpp_escape(w.tooltip.empty() ? w.content : w.tooltip) << "\");\n";
            ss << pad << "}\n";
            break;
        default:
            ss << pad << "// " << widget_type_to_string(w.type) << ": \"" << label << "\" (not exported)\n";
            break;
    }
}

// Generate Lua imgui code for a single widget.
static void generate_lua_widget(std::ostringstream& ss, const Widget& w, int indent) {
    std::string pad(indent * 4, ' ');
    std::string sid = sanitize_id(w.id);
    std::string label = lua_escape(w.label.empty() ? w.id : w.label);

    switch (w.type) {
        case WidgetType::Text:
            ss << pad << "imgui.Text(\"" << label << "\")\n";
            break;
        case WidgetType::TextColored:
            ss << pad << "imgui.TextColored(" << w.color[0] << ", " << w.color[1]
               << ", " << w.color[2] << ", " << w.color[3] << ", \"" << label << "\")\n";
            break;
        case WidgetType::TextDisabled:
            ss << pad << "imgui.TextDisabled(\"" << label << "\")\n";
            break;
        case WidgetType::TextWrapped:
            ss << pad << "imgui.TextWrapped(\"" << label << "\")\n";
            break;
        case WidgetType::BulletText:
            ss << pad << "imgui.BulletText(\"" << label << "\")\n";
            break;
        case WidgetType::SeparatorText:
            ss << pad << "imgui.SeparatorText(\"" << label << "\")\n";
            break;
        case WidgetType::Button:
            ss << pad << "if imgui.Button(\"" << label << "\"";
            if (w.size_x > 0 || w.size_y > 0)
                ss << ", " << w.size_x << ", " << w.size_y;
            ss << ") then end\n";
            break;
        case WidgetType::SmallButton:
            ss << pad << "if imgui.SmallButton(\"" << label << "\") then end\n";
            break;
        case WidgetType::ArrowButton:
            ss << pad << "if imgui.ArrowButton(\"" << sid << "\", " << w.dir << ") then end\n";
            break;
        case WidgetType::Checkbox:
            ss << pad << sid << " = imgui.Checkbox(\"" << label << "\", " << (w.bool_val ? "true" : "false") << ")\n";
            break;
        case WidgetType::RadioButton:
            ss << pad << sid << " = imgui.RadioButton(\"" << label << "\", " << sid << " or 0, " << w.int_val[0] << ")\n";
            break;
        case WidgetType::ProgressBar:
            ss << pad << "imgui.ProgressBar(" << w.float_val[0];
            if (w.size_x > 0 || w.size_y > 0)
                ss << ", " << w.size_x << ", " << w.size_y;
            ss << ")\n";
            break;
        case WidgetType::Bullet:
            ss << pad << "imgui.Bullet()\n";
            break;
        case WidgetType::Separator:
            ss << pad << "imgui.Separator()\n";
            break;
        case WidgetType::SameLine:
            ss << pad << "imgui.SameLine()\n";
            break;
        case WidgetType::NewLine:
            ss << pad << "imgui.NewLine()\n";
            break;
        case WidgetType::Spacing:
            ss << pad << "imgui.Spacing()\n";
            break;
        case WidgetType::Dummy:
            ss << pad << "imgui.Dummy(" << w.size_x << ", " << w.size_y << ")\n";
            break;
        case WidgetType::Indent:
            ss << pad << "imgui.Indent()\n";
            break;
        case WidgetType::Unindent:
            ss << pad << "imgui.Unindent()\n";
            break;
        case WidgetType::AlignTextToFramePadding:
            ss << pad << "imgui.AlignTextToFramePadding()\n";
            break;
        case WidgetType::SliderFloat:
            ss << pad << sid << " = imgui.SliderFloat(\"" << label << "\", " << sid << " or " << w.float_val[0]
               << ", " << w.float_min << ", " << w.float_max << ")\n";
            break;
        case WidgetType::SliderInt:
            ss << pad << sid << " = imgui.SliderInt(\"" << label << "\", " << sid << " or " << w.int_val[0]
               << ", " << w.int_min << ", " << w.int_max << ")\n";
            break;
        case WidgetType::DragFloat:
            ss << pad << sid << " = imgui.DragFloat(\"" << label << "\", " << sid << " or " << w.float_val[0]
               << ", " << w.float_speed << ", " << w.float_min << ", " << w.float_max << ")\n";
            break;
        case WidgetType::DragInt:
            ss << pad << sid << " = imgui.DragInt(\"" << label << "\", " << sid << " or " << w.int_val[0]
               << ", " << w.float_speed << ", " << w.int_min << ", " << w.int_max << ")\n";
            break;
        case WidgetType::InputText:
            ss << pad << sid << " = imgui.InputText(\"" << label << "\", " << sid
               << " or \"" << lua_escape(std::string(w.text_buf)) << "\")\n";
            break;
        case WidgetType::InputTextWithHint:
            ss << pad << sid << " = imgui.InputTextWithHint(\"" << label << "\", \"" << lua_escape(w.hint)
               << "\", " << sid << " or \"" << lua_escape(std::string(w.text_buf)) << "\")\n";
            break;
        case WidgetType::InputFloat:
            ss << pad << sid << " = imgui.InputFloat(\"" << label << "\", " << sid << " or " << w.float_val[0] << ")\n";
            break;
        case WidgetType::InputInt:
            ss << pad << sid << " = imgui.InputInt(\"" << label << "\", " << sid << " or " << w.int_val[0] << ")\n";
            break;
        case WidgetType::ColorEdit3:
            ss << pad << sid << " = imgui.ColorEdit3(\"" << label << "\", " << sid << " or {" << w.color[0]
               << ", " << w.color[1] << ", " << w.color[2] << "})\n";
            break;
        case WidgetType::ColorEdit4:
            ss << pad << sid << " = imgui.ColorEdit4(\"" << label << "\", " << sid << " or {" << w.color[0]
               << ", " << w.color[1] << ", " << w.color[2] << ", " << w.color[3] << "})\n";
            break;
        case WidgetType::Combo: {
            ss << pad << sid << " = imgui.Combo(\"" << label << "\", " << sid << " or " << w.selected << ", {";
            for (size_t i = 0; i < w.items.size(); i++) {
                if (i > 0) ss << ", ";
                ss << "\"" << lua_escape(w.items[i]) << "\"";
            }
            ss << "})\n";
            break;
        }
        case WidgetType::ListBox: {
            ss << pad << sid << " = imgui.ListBox(\"" << label << "\", " << sid << " or " << w.selected << ", {";
            for (size_t i = 0; i < w.items.size(); i++) {
                if (i > 0) ss << ", ";
                ss << "\"" << lua_escape(w.items[i]) << "\"";
            }
            ss << "})\n";
            break;
        }
        case WidgetType::Selectable:
            ss << pad << sid << " = imgui.Selectable(\"" << label << "\", " << (w.bool_val ? "true" : "false") << ")\n";
            break;
        case WidgetType::TreeNode:
            ss << pad << "if imgui.TreeNode(\"" << label << "\") then\n";
            for (auto& child : w.children)
                generate_lua_widget(ss, child, indent + 1);
            ss << pad << "    imgui.TreePop()\n";
            ss << pad << "end\n";
            break;
        case WidgetType::CollapsingHeader:
            ss << pad << "if imgui.CollapsingHeader(\"" << label << "\") then\n";
            for (auto& child : w.children)
                generate_lua_widget(ss, child, indent + 1);
            ss << pad << "end\n";
            break;
        case WidgetType::TabBar:
            ss << pad << "if imgui.BeginTabBar(\"" << sid << "\") then\n";
            for (auto& child : w.children)
                generate_lua_widget(ss, child, indent + 1);
            ss << pad << "    imgui.EndTabBar()\n";
            ss << pad << "end\n";
            break;
        case WidgetType::TabItem:
            ss << pad << "if imgui.BeginTabItem(\"" << label << "\") then\n";
            for (auto& child : w.children)
                generate_lua_widget(ss, child, indent + 1);
            ss << pad << "    imgui.EndTabItem()\n";
            ss << pad << "end\n";
            break;
        case WidgetType::ChildWindow:
            ss << pad << "if imgui.BeginChild(\"" << sid << "\", " << w.size_x << ", " << w.size_y << ") then\n";
            for (auto& child : w.children)
                generate_lua_widget(ss, child, indent + 1);
            ss << pad << "    imgui.EndChild()\n";
            ss << pad << "end\n";
            break;
        case WidgetType::Group:
            ss << pad << "imgui.BeginGroup()\n";
            for (auto& child : w.children)
                generate_lua_widget(ss, child, indent + 1);
            ss << pad << "imgui.EndGroup()\n";
            break;
        case WidgetType::MenuItem:
            ss << pad << "if imgui.MenuItem(\"" << label << "\") then end\n";
            break;
        case WidgetType::Tooltip:
        case WidgetType::SetItemTooltip:
            ss << pad << "if imgui.IsItemHovered() then\n";
            ss << pad << "    imgui.SetTooltip(\"" << lua_escape(w.tooltip.empty() ? w.content : w.tooltip) << "\")\n";
            ss << pad << "end\n";
            break;
        default:
            ss << pad << "-- " << widget_type_to_string(w.type) << ": \"" << label << "\" (not exported)\n";
            break;
    }
}

// Serialize a widget to JSON for export_json (public-facing format).
static json widget_export_json(const Widget& w) {
    json j;
    j["id"] = w.id;
    j["type"] = widget_type_to_string(w.type);
    j["label"] = w.label;
    if (!w.content.empty()) j["content"] = w.content;
    if (!w.hint.empty()) j["hint"] = w.hint;
    if (!w.tooltip.empty()) j["tooltip"] = w.tooltip;
    if (!w.overlay.empty()) j["overlay"] = w.overlay;
    if (!w.format.empty()) j["format"] = w.format;
    j["value"] = w.float_val[0];
    j["min"] = w.float_min;
    j["max"] = w.float_max;
    j["int_value"] = w.int_val[0];
    j["int_min"] = w.int_min;
    j["int_max"] = w.int_max;
    j["checked"] = w.bool_val;
    std::string tb(w.text_buf);
    if (!tb.empty()) j["text"] = tb;
    if (w.selected >= 0) j["selected"] = w.selected;
    if (!w.items.empty()) j["items"] = w.items;
    j["color"] = {w.color[0], w.color[1], w.color[2], w.color[3]};
    if (w.size_x > 0 || w.size_y > 0) { j["size_x"] = w.size_x; j["size_y"] = w.size_y; }
    if (w.flags != 0) j["flags"] = w.flags;
    if (!w.enabled) j["enabled"] = false;
    if (w.type == WidgetType::InputDouble) j["double_value"] = w.double_val;
    if (!w.children.empty()) {
        json kids = json::array();
        for (auto& c : w.children) kids.push_back(widget_export_json(c));
        j["children"] = kids;
    }
    if (w.type == WidgetType::Table) {
        j["columns"] = w.columns;
        if (!w.headers.empty()) j["headers"] = w.headers;
        if (!w.rows.empty()) j["rows"] = w.rows;
    }
    return j;
}
// ─── Command processing ─────────────────────────────────────────────────────

void process_command(const json& cmd) {
    const std::string type = cmd.value("cmd", "");

    if (type == "create_window") {
        std::string id = cmd.value("id", "window_" + std::to_string(g_windows.size()));
        WindowState win;
        win.id = id;
        win.title = cmd.value("title", id);
        win.x = cmd.value("x", 100.0f);
        win.y = cmd.value("y", 100.0f);
        win.w = cmd.value("w", 400.0f);
        win.h = cmd.value("h", 300.0f);
        win.flags = cmd.value("flags", 0);
        win.has_menubar = cmd.value("has_menubar", false);
        win.open = cmd.value("open", true);

        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_windows.find(id) == g_windows.end())
            g_window_order.push_back(id);
        g_windows[id] = win;
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id}});

    } else if (type == "remove_window") {
        std::string id = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        push_undo_state();
        g_windows.erase(id);
        g_window_order.erase(
            std::remove(g_window_order.begin(), g_window_order.end(), id),
            g_window_order.end());
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id}});

    } else if (type == "add_widget") {
        std::string win_id = cmd.value("window", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        push_undo_state();
        Widget w = parse_widget_json(cmd);
        auto& win = it->second;
        if (win.widgets.find(w.id) == win.widgets.end())
            win.widget_order.push_back(w.id);
        win.widgets[w.id] = w;
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", w.id}});

    } else if (type == "remove_widget") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        push_undo_state();
        auto it = g_windows.find(win_id);
        if (it != g_windows.end()) {
            it->second.widgets.erase(wid);
            auto& order = it->second.widget_order;
            order.erase(std::remove(order.begin(), order.end(), wid), order.end());
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}});

    } else if (type == "update_widget") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        Widget* w = find_widget_in_window(it->second, wid);
        if (!w) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
            return;
        }
        push_undo_state();
        if (cmd.contains("widget_type") && cmd["widget_type"].is_string())
            w->type = parse_widget_type(cmd["widget_type"].get<std::string>());
        apply_widget_fields(*w, cmd);
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}});

    } else if (type == "get_state") {
        std::lock_guard<std::mutex> lock(g_mutex);
        json state;
        state["type"] = "state";
        state["frame"] = g_frame_count;
        state["windows"] = json::object();
        for (auto& [wid, win] : g_windows) {
            json wj;
            wj["title"] = win.title;
            wj["open"] = win.open;
            wj["collapsed"] = win.collapsed;
            wj["x"] = win.x;
            wj["y"] = win.y;
            wj["w"] = win.w;
            wj["h"] = win.h;
            wj["flags"] = win.flags;
            wj["has_menubar"] = win.has_menubar;
            wj["widgets"] = json::object();
            for (auto& [gid, w] : win.widgets)
                wj["widgets"][gid] = widget_state_json(w);
            state["windows"][wid] = wj;
        }
        emit_json(state);

    } else if (type == "clear_all") {
        std::lock_guard<std::mutex> lock(g_mutex);
        push_undo_state();
        g_windows.clear();
        g_window_order.clear();
        emit_json({{"type", "ack"}, {"cmd", type}});

    } else if (type == "set_style") {
        std::string theme = cmd.value("theme", "dark");
        if (theme == "light") ImGui::StyleColorsLight();
        else if (theme == "classic") ImGui::StyleColorsClassic();
        else ImGui::StyleColorsDark();
        emit_json({{"type", "ack"}, {"cmd", type}, {"theme", theme}});

    } else if (type == "push_style_color") {
        int idx = -1;
        if (cmd.contains("col_idx")) idx = parse_style_color_index(cmd["col_idx"]);
        if (idx < 0) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "unknown style color index"}});
            return;
        }
        float c[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        if (cmd.contains("color") && cmd["color"].is_array()) {
            const json& arr = cmd["color"];
            for (int i = 0; i < 4 && i < (int)arr.size(); i++)
                c[i] = arr[i].get<float>();
        }
        ImGui::PushStyleColor(idx, ImVec4(c[0], c[1], c[2], c[3]));
        emit_json({{"type", "ack"}, {"cmd", type}, {"col_idx", idx}});

    } else if (type == "pop_style_color") {
        int count = cmd.value("count", 1);
        ImGui::PopStyleColor(count);
        emit_json({{"type", "ack"}, {"cmd", type}, {"count", count}});

    } else if (type == "push_style_var") {
        int idx = -1;
        if (cmd.contains("var_idx")) idx = parse_style_var_index(cmd["var_idx"]);
        if (idx < 0) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "unknown style var index"}});
            return;
        }
        if (cmd.contains("value") && cmd["value"].is_array() && cmd["value"].size() >= 2) {
            float x = cmd["value"][0].get<float>();
            float y = cmd["value"][1].get<float>();
            ImGui::PushStyleVar(idx, ImVec2(x, y));
        } else {
            float v = cmd.value("value", 0.0f);
            ImGui::PushStyleVar(idx, v);
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"var_idx", idx}});

    } else if (type == "pop_style_var") {
        int count = cmd.value("count", 1);
        ImGui::PopStyleVar(count);
        emit_json({{"type", "ack"}, {"cmd", type}, {"count", count}});

    } else if (type == "set_background") {
        if (cmd.contains("color") && cmd["color"].is_array()) {
            const json& c = cmd["color"];
            for (int i = 0; i < 3 && i < (int)c.size(); i++)
                g_bg_color[i] = c[i].get<float>();
        }
        emit_json({{"type", "ack"}, {"cmd", type}});

    } else if (type == "show_demo") {
        g_show_demo = cmd.value("show", true);
        emit_json({{"type", "ack"}, {"cmd", type}, {"show", g_show_demo}});

    } else if (type == "show_metrics") {
        g_show_metrics = cmd.value("show", true);
        emit_json({{"type", "ack"}, {"cmd", type}, {"show", g_show_metrics}});

    } else if (type == "show_about") {
        g_show_about = cmd.value("show", true);
        emit_json({{"type", "ack"}, {"cmd", type}, {"show", g_show_about}});

    } else if (type == "show_style_editor") {
        g_show_style_editor = cmd.value("show", true);
        emit_json({{"type", "ack"}, {"cmd", type}, {"show", g_show_style_editor}});

    } else if (type == "show_debug_log") {
        g_show_debug_log = cmd.value("show", true);
        emit_json({{"type", "ack"}, {"cmd", type}, {"show", g_show_debug_log}});

    } else if (type == "show_id_stack_tool") {
        g_show_id_stack_tool = cmd.value("show", true);
        emit_json({{"type", "ack"}, {"cmd", type}, {"show", g_show_id_stack_tool}});

    } else if (type == "window_control") {
        std::string id = cmd.value("id", "");
        std::string action = cmd.value("action", "");

        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(id);
        // Named-window ImGui functions key off the window title.
        std::string name = (it != g_windows.end()) ? it->second.title : id;

        // Read up to two numeric operands from "values" array or x/y keys.
        float vx = 0.0f, vy = 0.0f;
        if (cmd.contains("values") && cmd["values"].is_array() && cmd["values"].size() >= 1) {
            vx = cmd["values"][0].get<float>();
            if (cmd["values"].size() >= 2) vy = cmd["values"][1].get<float>();
        } else {
            vx = cmd.value("x", 0.0f);
            vy = cmd.value("y", 0.0f);
        }

        if (action == "set_pos") {
            ImGui::SetWindowPos(name.c_str(), ImVec2(vx, vy));
            if (it != g_windows.end()) { it->second.x = vx; it->second.y = vy; }
        } else if (action == "set_size") {
            ImGui::SetWindowSize(name.c_str(), ImVec2(vx, vy));
            if (it != g_windows.end()) { it->second.w = vx; it->second.h = vy; }
        } else if (action == "set_collapsed") {
            bool collapsed = cmd.value("collapsed", vx != 0.0f);
            ImGui::SetWindowCollapsed(name.c_str(), collapsed);
            if (it != g_windows.end()) it->second.collapsed = collapsed;
        } else if (action == "set_focus") {
            ImGui::SetWindowFocus(name.c_str());
        } else if (action == "set_bg_alpha") {
            float alpha = cmd.value("alpha", vx);
            ImGui::SetNextWindowBgAlpha(alpha);
        } else if (action == "set_scroll_x") {
            ImGui::SetNextWindowScroll(ImVec2(vx, -1.0f));
        } else if (action == "set_scroll_y") {
            ImGui::SetNextWindowScroll(ImVec2(-1.0f, vx));
        } else {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "unknown action: " + action}});
            return;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id}, {"action", action}});

    } else if (type == "get_input_state") {
        ImGuiIO& io = ImGui::GetIO();
        json s;
        s["type"] = "input_state";
        s["mouse_pos"] = {io.MousePos.x, io.MousePos.y};
        s["mouse_down"] = {io.MouseDown[0], io.MouseDown[1], io.MouseDown[2]};
        s["is_any_item_hovered"] = ImGui::IsAnyItemHovered();
        s["is_any_item_active"] = ImGui::IsAnyItemActive();
        s["is_any_item_focused"] = ImGui::IsAnyItemFocused();
        s["frame_count"] = g_frame_count;
        s["time"] = ImGui::GetTime();
        emit_json(s);

    } else if (type == "clipboard_get") {
        const char* text = ImGui::GetClipboardText();
        emit_json({{"type", "clipboard"}, {"text", text ? text : ""}});

    } else if (type == "clipboard_set") {
        std::string text = cmd.value("text", "");
        ImGui::SetClipboardText(text.c_str());
        emit_json({{"type", "ack"}, {"cmd", type}});

    } else if (type == "draw") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "draw_list");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        auto& win = it->second;
        Widget* w = find_widget_in_window(win, wid);
        if (!w) {
            Widget nw;
            nw.id = wid;
            nw.type = WidgetType::DrawList;
            nw.label = wid;
            win.widget_order.push_back(wid);
            win.widgets[wid] = nw;
            w = &win.widgets[wid];
        }
        w->draw_commands.clear();
        if (cmd.contains("commands") && cmd["commands"].is_array()) {
            for (const auto& c : cmd["commands"])
                w->draw_commands.push_back(parse_draw_command(c));
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}});

    } else if (type == "open_popup") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it != g_windows.end()) {
            Widget* w = find_widget_in_window(it->second, wid);
            if (!w) {
                Widget nw;
                nw.id = wid;
                nw.type = WidgetType::Popup;
                nw.label = wid;
                it->second.widget_order.push_back(wid);
                it->second.widgets[wid] = nw;
                w = &it->second.widgets[wid];
            }
            w->popup_open = true;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}});

    } else if (type == "close_popup") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it != g_windows.end()) {
            Widget* w = find_widget_in_window(it->second, wid);
            if (w) w->popup_open = false;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}});

    } else if (type == "screenshot") {
        std::string path = cmd.value("path", "/tmp/imgui_screenshot.bmp");
        int quality = cmd.value("quality", 90);
        g_screenshot_path = path;
        g_screenshot_quality = quality;
        g_screenshot_annotated = false;
        g_screenshot_requested = true;
        emit_json({{"type", "ack"}, {"cmd", type}, {"path", path}});

    } else if (type == "screenshot_widget") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("widget", "");
        std::string path = cmd.value("path", "/tmp/imgui_screenshot.bmp");
        json bounds_info = json::object();
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_windows.find(win_id);
            if (it != g_windows.end()) {
                Widget* w = find_widget_in_window(it->second, wid);
                if (w) {
                    bounds_info["found"] = true;
                    bounds_info["widget"] = wid;
                    bounds_info["window"] = win_id;
                } else {
                    bounds_info["found"] = false;
                    bounds_info["error"] = "widget not found: " + wid;
                }
            } else {
                bounds_info["found"] = false;
                bounds_info["error"] = "window not found: " + win_id;
            }
        }
        g_screenshot_path = path;
        g_screenshot_quality = 90;
        g_screenshot_annotated = false;
        g_screenshot_requested = true;
        emit_json({{"type", "ack"}, {"cmd", type}, {"path", path}, {"widget_info", bounds_info}});

    } else if (type == "screenshot_annotated") {
        std::string path = cmd.value("path", "/tmp/imgui_screenshot.bmp");
        g_screenshot_path = path;
        g_screenshot_quality = 90;
        g_screenshot_annotated = true;
        g_screenshot_requested = true;
        emit_json({{"type", "ack"}, {"cmd", type}, {"path", path}, {"annotated", true}});


    // ── Window/Widget manipulation commands ─────────────────────────────────

    } else if (type == "move_window") {
        std::string id = cmd.value("id", "");
        float x = cmd.value("x", 0.0f);
        float y = cmd.value("y", 0.0f);
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + id}});
            return;
        }
        it->second.x = x;
        it->second.y = y;
        ImGui::SetWindowPos(it->second.title.c_str(), ImVec2(x, y));
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id}, {"x", x}, {"y", y}});

    } else if (type == "resize_window") {
        std::string id = cmd.value("id", "");
        float w = cmd.value("w", 400.0f);
        float h = cmd.value("h", 300.0f);
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + id}});
            return;
        }
        it->second.w = w;
        it->second.h = h;
        ImGui::SetWindowSize(it->second.title.c_str(), ImVec2(w, h));
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id}, {"w", w}, {"h", h}});

    } else if (type == "move_widget") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        float ox = cmd.value("offset_x", 0.0f);
        float oy = cmd.value("offset_y", 0.0f);
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        Widget* w = find_widget_in_window(it->second, wid);
        if (!w) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
            return;
        }
        w->cursor_offset[0] = ox;
        w->cursor_offset[1] = oy;
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}, {"offset_x", ox}, {"offset_y", oy}});

    } else if (type == "get_widget_rect") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        Widget* w = find_widget_in_window(it->second, wid);
        if (!w) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
            return;
        }
        float rw = w->rect_max[0] - w->rect_min[0];
        float rh = w->rect_max[1] - w->rect_min[1];
        emit_json({
            {"type", "rect"}, {"window", win_id}, {"id", wid},
            {"rect_min", {w->rect_min[0], w->rect_min[1]}},
            {"rect_max", {w->rect_max[0], w->rect_max[1]}},
            {"width", rw}, {"height", rh}
        });

    } else if (type == "get_window_rect") {
        std::string id = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + id}});
            return;
        }
        emit_json({
            {"type", "rect"}, {"id", id},
            {"x", it->second.x}, {"y", it->second.y},
            {"w", it->second.w}, {"h", it->second.h}
        });

    } else if (type == "bring_to_front") {
        std::string id = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(id);
        std::string name = (it != g_windows.end()) ? it->second.title : id;
        ImGui::SetWindowFocus(name.c_str());
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id}});

    } else if (type == "set_widget_size") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        float width = cmd.value("width", 0.0f);
        float height = cmd.value("height", 0.0f);
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        Widget* w = find_widget_in_window(it->second, wid);
        if (!w) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
            return;
        }
        w->size_x = width;
        w->size_y = height;
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}, {"width", width}, {"height", height}});

    } else if (type == "snap_to_grid") {
        std::string win_id = cmd.value("window", "");
        int grid_size = cmd.value("grid_size", 8);
        if (grid_size < 1) grid_size = 1;
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        for (auto& [gid, w] : it->second.widgets) {
            w.cursor_offset[0] = std::round(w.cursor_offset[0] / grid_size) * grid_size;
            w.cursor_offset[1] = std::round(w.cursor_offset[1] / grid_size) * grid_size;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"grid_size", grid_size}});

    } else if (type == "get_layout") {
        std::string win_id = cmd.value("window", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        json widgets_json = json::object();
        for (auto& [gid, w] : it->second.widgets) {
            widgets_json[gid] = {
                {"x", w.rect_min[0]}, {"y", w.rect_min[1]},
                {"w", w.rect_max[0] - w.rect_min[0]},
                {"h", w.rect_max[1] - w.rect_min[1]}
            };
        }
        emit_json({{"type", "layout"}, {"window", win_id}, {"widgets", widgets_json}});


    } else if (type == "click_widget") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        int button = cmd.value("button", 0);
        float cx = 0, cy = 0;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_windows.find(win_id);
            if (it == g_windows.end()) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
                return;
            }
            Widget* w = find_widget_in_window(it->second, wid);
            if (!w) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
                return;
            }
            cx = (w->rect_min[0] + w->rect_max[0]) * 0.5f;
            cy = (w->rect_min[1] + w->rect_max[1]) * 0.5f;
            w->clicked = true;
            emit_event(win_id, wid, "click", {{"button", button}});
        }
        {
            std::lock_guard<std::mutex> lock(g_input_mutex);
            g_input_queue.push_back({{"type", "mouse_move"}, {"x", cx}, {"y", cy}});
            g_input_queue.push_back({{"type", "mouse_click"}, {"x", cx}, {"y", cy}, {"button", button}});
            g_input_queue.push_back({{"type", "mouse_release"}, {"button", button}});
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}, {"x", cx}, {"y", cy}});

    } else if (type == "type_text") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        std::string text = cmd.value("text", "");
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_windows.find(win_id);
            if (it == g_windows.end()) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
                return;
            }
            Widget* w = find_widget_in_window(it->second, wid);
            if (!w) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
                return;
            }
            w->request_focus = true;
        }
        {
            std::lock_guard<std::mutex> lock(g_input_mutex);
            g_input_queue.push_back({{"type", "text_input"}, {"text", text}});
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}, {"text", text}});

    } else if (type == "hover_widget") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        float cx = 0, cy = 0;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_windows.find(win_id);
            if (it == g_windows.end()) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
                return;
            }
            Widget* w = find_widget_in_window(it->second, wid);
            if (!w) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
                return;
            }
            cx = (w->rect_min[0] + w->rect_max[0]) * 0.5f;
            cy = (w->rect_min[1] + w->rect_max[1]) * 0.5f;
        }
        {
            std::lock_guard<std::mutex> lock(g_input_mutex);
            g_input_queue.push_back({{"type", "mouse_move"}, {"x", cx}, {"y", cy}});
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}, {"x", cx}, {"y", cy}});

    } else if (type == "scroll_window") {
        std::string win_id = cmd.value("window", "");
        float delta_y = cmd.value("delta_y", 1.0f);
        {
            std::lock_guard<std::mutex> lock(g_input_mutex);
            g_input_queue.push_back({{"type", "mouse_scroll"}, {"x", 0.0f}, {"y", delta_y}});
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"delta_y", delta_y}});

    } else if (type == "press_key") {
        std::string key_name = cmd.value("key", "");
        ImGuiKey k = parse_key_name(key_name);
        bool ctrl  = parse_modifier(key_name, "Ctrl");
        bool shift = parse_modifier(key_name, "Shift");
        bool alt   = parse_modifier(key_name, "Alt");
        bool super = parse_modifier(key_name, "Super");
        {
            std::lock_guard<std::mutex> lock(g_input_mutex);
            if (ctrl)  g_input_queue.push_back({{"type", "key_press"}, {"key", (int)ImGuiKey_LeftCtrl}});
            if (shift) g_input_queue.push_back({{"type", "key_press"}, {"key", (int)ImGuiKey_LeftShift}});
            if (alt)   g_input_queue.push_back({{"type", "key_press"}, {"key", (int)ImGuiKey_LeftAlt}});
            if (super) g_input_queue.push_back({{"type", "key_press"}, {"key", (int)ImGuiKey_LeftSuper}});
            g_input_queue.push_back({{"type", "key_press"}, {"key", (int)k}});
            g_input_queue.push_back({{"type", "key_release"}, {"key", (int)k}});
            if (super) g_input_queue.push_back({{"type", "key_release"}, {"key", (int)ImGuiKey_LeftSuper}});
            if (alt)   g_input_queue.push_back({{"type", "key_release"}, {"key", (int)ImGuiKey_LeftAlt}});
            if (shift) g_input_queue.push_back({{"type", "key_release"}, {"key", (int)ImGuiKey_LeftShift}});
            if (ctrl)  g_input_queue.push_back({{"type", "key_release"}, {"key", (int)ImGuiKey_LeftCtrl}});
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"key", key_name}});

    } else if (type == "set_mouse_pos") {
        float x = cmd.value("x", 0.0f);
        float y = cmd.value("y", 0.0f);
        {
            std::lock_guard<std::mutex> lock(g_input_mutex);
            g_input_queue.push_back({{"type", "mouse_move"}, {"x", x}, {"y", y}});
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"x", x}, {"y", y}});

    } else if (type == "focus_widget") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_windows.find(win_id);
            if (it == g_windows.end()) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
                return;
            }
            Widget* w = find_widget_in_window(it->second, wid);
            if (!w) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
                return;
            }
            w->request_focus = true;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}});

    } else if (type == "animate") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("widget", cmd.value("id", ""));
        std::string prop = cmd.value("property", "value");
        float from = cmd.value("from", 0.0f);
        float to = cmd.value("to", 1.0f);
        float duration = cmd.value("duration", 1.0f);
        std::string ease_str = cmd.value("ease", "linear");
        bool loop = cmd.value("loop", false);
        Animation anim;
        anim.window_id = win_id;
        anim.widget_id = wid;
        anim.property = prop;
        anim.from = from;
        anim.to = to;
        anim.duration = duration > 0.0f ? duration : 0.001f;
        anim.ease = parse_ease_type(ease_str);
        anim.loop = loop;
        anim.active = true;
        anim.elapsed = 0.0f;
        {
            std::lock_guard<std::mutex> lock(g_anim_mutex);
            g_animations.push_back(anim);
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"widget", wid},
                   {"property", prop}, {"duration", duration}, {"ease", ease_str}, {"loop", loop}});

    } else if (type == "stop_animation") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("widget", cmd.value("id", ""));
        {
            std::lock_guard<std::mutex> lock(g_anim_mutex);
            if (wid.empty()) {
                g_animations.erase(
                    std::remove_if(g_animations.begin(), g_animations.end(),
                        [&](const Animation& a) { return a.window_id == win_id; }),
                    g_animations.end());
            } else {
                g_animations.erase(
                    std::remove_if(g_animations.begin(), g_animations.end(),
                        [&](const Animation& a) { return a.window_id == win_id && a.widget_id == wid; }),
                    g_animations.end());
            }
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"widget", wid}});

    } else if (type == "stop_all_animations") {
        {
            std::lock_guard<std::mutex> lock(g_anim_mutex);
            g_animations.clear();
        }
        emit_json({{"type", "ack"}, {"cmd", type}});

    } else if (type == "set_anchor") {
        std::string win_id = cmd.value("window", "");
        std::string anchor_str = cmd.value("anchor", "none");
        float ox = cmd.value("offset_x", 0.0f);
        float oy = cmd.value("offset_y", 0.0f);
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_windows.find(win_id);
            if (it == g_windows.end()) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
                return;
            }
            it->second.window_anchor = parse_anchor(anchor_str);
            it->second.window_anchor_offset[0] = ox;
            it->second.window_anchor_offset[1] = oy;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"anchor", anchor_str}});

    } else if (type == "set_widget_anchor") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("widget", "");
        std::string anchor_str = cmd.value("anchor", "none");
        float ox = cmd.value("offset_x", 0.0f);
        float oy = cmd.value("offset_y", 0.0f);
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_windows.find(win_id);
            if (it == g_windows.end()) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
                return;
            }
            Widget* w = find_widget_in_window(it->second, wid);
            if (!w) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
                return;
            }
            w->anchor = parse_anchor(anchor_str);
            w->anchor_offset[0] = ox;
            w->anchor_offset[1] = oy;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"widget", wid}, {"anchor", anchor_str}});

    } else if (type == "set_viewport_size") {
        int w = cmd.value("width", 1280);
        int h = cmd.value("height", 720);
        if (g_sdl_window) {
            SDL_SetWindowSize(g_sdl_window, w, h);
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"width", w}, {"height", h}});

    } else if (type == "get_viewport_size") {
        int w = 0, h = 0;
        if (g_sdl_window) {
            SDL_GetWindowSize(g_sdl_window, &w, &h);
        }
        emit_json({{"type", "viewport"}, {"width", w}, {"height", h}});

    } else if (type == "set_resolution_preset") {
        std::string preset = cmd.value("preset", "1080p");
        int w = 1920, h = 1080;
        if (preset == "720p")          { w = 1280; h = 720; }
        else if (preset == "1080p")    { w = 1920; h = 1080; }
        else if (preset == "1440p")    { w = 2560; h = 1440; }
        else if (preset == "4k")       { w = 3840; h = 2160; }
        else if (preset == "ultrawide"){ w = 3440; h = 1440; }
        else if (preset == "mobile")   { w = 390;  h = 844; }
        else if (preset == "tablet")   { w = 1024; h = 768; }
        if (g_sdl_window) {
            SDL_SetWindowSize(g_sdl_window, w, h);
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"preset", preset}, {"width", w}, {"height", h}});

    } else if (type == "set_dpi_scale") {
        float scale = cmd.value("scale", 1.0f);
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scale);
        emit_json({{"type", "ack"}, {"cmd", type}, {"scale", scale}});

    } else if (type == "set_safe_area") {
        float top    = cmd.value("top", 0.0f);
        float bottom = cmd.value("bottom", 0.0f);
        float left   = cmd.value("left", 0.0f);
        float right  = cmd.value("right", 0.0f);
        g_safe_area[0] = top;
        g_safe_area[1] = bottom;
        g_safe_area[2] = left;
        g_safe_area[3] = right;
        emit_json({{"type", "ack"}, {"cmd", type}, {"top", top}, {"bottom", bottom}, {"left", left}, {"right", right}});

    } else if (type == "create_scene") {
        std::string name = cmd.value("name", "");
        Scene scene;
        scene.name = name;
        if (cmd.contains("windows") && cmd["windows"].is_array()) {
            for (const auto& wid : cmd["windows"])
                scene.window_ids.push_back(wid.get<std::string>());
        }
        std::lock_guard<std::mutex> lock(g_mutex);
        g_scenes[name] = scene;
        emit_json({{"type", "ack"}, {"cmd", type}, {"name", name}});

    } else if (type == "delete_scene") {
        std::string name = cmd.value("name", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        g_scenes.erase(name);
        if (g_active_scene == name) g_active_scene.clear();
        emit_json({{"type", "ack"}, {"cmd", type}, {"name", name}});

    } else if (type == "switch_scene") {
        std::string name = cmd.value("name", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto sit = g_scenes.find(name);
        if (sit == g_scenes.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "scene not found: " + name}});
            return;
        }
        g_active_scene = name;
        json windows_json = json::array();
        for (auto& wid : sit->second.window_ids)
            windows_json.push_back(wid);
        emit_json({{"type", "ack"}, {"cmd", type}, {"name", name}, {"windows", windows_json}});

    } else if (type == "list_scenes") {
        std::lock_guard<std::mutex> lock(g_mutex);
        json scenes_json = json::object();
        for (auto& [sname, scene] : g_scenes) {
            json wlist = json::array();
            for (auto& wid : scene.window_ids)
                wlist.push_back(wid);
            scenes_json[sname] = wlist;
        }
        emit_json({{"type", "scenes"}, {"cmd", type}, {"scenes", scenes_json}, {"active", g_active_scene}});

    } else if (type == "set_window_layer") {
        std::string id = cmd.value("id", "");
        int layer = cmd.value("layer", 0);
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + id}});
            return;
        }
        it->second.layer = layer;
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id}, {"layer", layer}});

    } else if (type == "set_widget_visibility") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        std::string condition = cmd.value("condition", "always");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        Widget* w = find_widget_in_window(it->second, wid);
        if (!w) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "widget not found: " + wid}});
            return;
        }
        w->visible_condition = condition;
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"id", wid}, {"condition", condition}});

    } else if (type == "set_window_visibility") {
        std::string id = cmd.value("id", "");
        bool visible = cmd.value("visible", true);
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + id}});
            return;
        }
        it->second.open = visible;
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id}, {"visible", visible}});

    } else if (type == "push_undo") {
        std::lock_guard<std::mutex> lock(g_mutex);
        push_undo_state();
        emit_json({{"type", "ack"}, {"cmd", type}, {"undo_depth", (int)g_undo_stack.size()}});

    } else if (type == "undo") {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_undo_stack.empty()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "undo stack is empty"}});
            return;
        }
        g_redo_stack.push_back(serialize_state());
        json prev = g_undo_stack.back();
        g_undo_stack.pop_back();
        restore_state(prev);
        emit_json({{"type", "ack"}, {"cmd", type}, {"undo_depth", (int)g_undo_stack.size()}, {"redo_depth", (int)g_redo_stack.size()}});

    } else if (type == "redo") {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_redo_stack.empty()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "redo stack is empty"}});
            return;
        }
        g_undo_stack.push_back(serialize_state());
        if ((int)g_undo_stack.size() > g_max_undo)
            g_undo_stack.erase(g_undo_stack.begin());
        json next = g_redo_stack.back();
        g_redo_stack.pop_back();
        restore_state(next);
        emit_json({{"type", "ack"}, {"cmd", type}, {"undo_depth", (int)g_undo_stack.size()}, {"redo_depth", (int)g_redo_stack.size()}});

    } else if (type == "save_snapshot") {
        std::string name = cmd.value("name", "snapshot_" + std::to_string(g_snapshots.size()));
        std::lock_guard<std::mutex> lock(g_mutex);
        Snapshot snap;
        snap.name = name;
        snap.frame = g_frame_count;
        snap.data = serialize_state();
        g_snapshots.push_back(std::move(snap));
        emit_json({{"type", "ack"}, {"cmd", type}, {"name", name}, {"snapshot_count", (int)g_snapshots.size()}});

    } else if (type == "load_snapshot") {
        std::string name = cmd.value("name", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        Snapshot* found = nullptr;
        for (auto& s : g_snapshots) {
            if (s.name == name) { found = &s; break; }
        }
        if (!found) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "snapshot not found: " + name}});
            return;
        }
        push_undo_state();
        restore_state(found->data);
        emit_json({{"type", "ack"}, {"cmd", type}, {"name", name}});

    } else if (type == "list_snapshots") {
        std::lock_guard<std::mutex> lock(g_mutex);
        json list = json::array();
        for (const auto& s : g_snapshots) {
            json entry;
            entry["name"] = s.name;
            entry["frame"] = s.frame;
            int wc = 0;
            if (s.data.contains("windows") && s.data["windows"].is_object()) {
                for (const auto& [wid, wj] : s.data["windows"].items()) {
                    if (wj.contains("widgets") && wj["widgets"].is_object())
                        wc += (int)wj["widgets"].size();
                }
            }
            entry["widget_count"] = wc;
            list.push_back(entry);
        }
        emit_json({{"type", "snapshots"}, {"cmd", type}, {"snapshots", list}, {"count", (int)g_snapshots.size()}});

    } else if (type == "delete_snapshot") {
        std::string name = cmd.value("name", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        bool found = false;
        for (auto it = g_snapshots.begin(); it != g_snapshots.end(); ++it) {
            if (it->name == name) { g_snapshots.erase(it); found = true; break; }
        }
        if (!found) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "snapshot not found: " + name}});
            return;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"name", name}, {"snapshot_count", (int)g_snapshots.size()}});

    } else if (type == "export_cpp") {
        std::string path = cmd.value("path", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        std::ostringstream ss;
        ss << "void render_ui() {\n";
        for (auto& wid : g_window_order) {
            auto wit = g_windows.find(wid);
            if (wit == g_windows.end()) continue;
            auto& win = wit->second;
            ss << "    // Window: " << win.id << "\n";
            ss << "    if (ImGui::Begin(\"" << cpp_escape(win.title) << "\")) {\n";
            for (auto& wname : win.widget_order) {
                auto wit2 = win.widgets.find(wname);
                if (wit2 == win.widgets.end()) continue;
                generate_cpp_widget(ss, wit2->second, 2);
            }
            ss << "    }\n";
            ss << "    ImGui::End();\n";
        }
        ss << "}\n";
        std::string code = ss.str();
        json resp;
        resp["type"] = "export";
        resp["format"] = "cpp";
        resp["code"] = code;
        if (!path.empty()) {
            std::ofstream f(path);
            if (f.is_open()) {
                f << code;
                f.close();
                resp["path"] = path;
            } else {
                resp["error"] = "failed to write file: " + path;
            }
        }
        emit_json(resp);

    } else if (type == "export_lua") {
        std::string path = cmd.value("path", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        std::ostringstream ss;
        ss << "function render_ui()\n";
        for (auto& wid : g_window_order) {
            auto wit = g_windows.find(wid);
            if (wit == g_windows.end()) continue;
            auto& win = wit->second;
            ss << "    -- Window: " << win.id << "\n";
            ss << "    imgui.Begin(\"" << lua_escape(win.title) << "\")\n";
            for (auto& wname : win.widget_order) {
                auto wit2 = win.widgets.find(wname);
                if (wit2 == win.widgets.end()) continue;
                generate_lua_widget(ss, wit2->second, 1);
            }
            ss << "    imgui.End()\n";
        }
        ss << "end\n";
        std::string code = ss.str();
        json resp;
        resp["type"] = "export";
        resp["format"] = "lua";
        resp["code"] = code;
        if (!path.empty()) {
            std::ofstream f(path);
            if (f.is_open()) {
                f << code;
                f.close();
                resp["path"] = path;
            } else {
                resp["error"] = "failed to write file: " + path;
            }
        }
        emit_json(resp);

    } else if (type == "export_json") {
        std::string path = cmd.value("path", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        json layout;
        layout["version"] = "1.0";
        layout["imgui_version"] = IMGUI_VERSION;
        json wins = json::array();
        for (auto& wid : g_window_order) {
            auto wit = g_windows.find(wid);
            if (wit == g_windows.end()) continue;
            auto& win = wit->second;
            json wj;
            wj["id"] = win.id;
            wj["title"] = win.title;
            wj["x"] = win.x;
            wj["y"] = win.y;
            wj["w"] = win.w;
            wj["h"] = win.h;
            wj["open"] = win.open;
            wj["collapsed"] = win.collapsed;
            wj["flags"] = win.flags;
            json widgets = json::array();
            for (auto& wname : win.widget_order) {
                auto wit2 = win.widgets.find(wname);
                if (wit2 == win.widgets.end()) continue;
                widgets.push_back(widget_export_json(wit2->second));
            }
            wj["widgets"] = widgets;
            wins.push_back(wj);
        }
        layout["windows"] = wins;
        json resp;
        resp["type"] = "export";
        resp["format"] = "json";
        resp["json"] = layout;
        if (!path.empty()) {
            std::ofstream f(path);
            if (f.is_open()) {
                f << layout.dump(2);
                f.close();
                resp["path"] = path;
            } else {
                resp["error"] = "failed to write file: " + path;
            }
        }
        emit_json(resp);

    } else if (type == "import_json") {
        json layout;
        if (cmd.contains("json")) {
            layout = cmd["json"];
        } else if (cmd.contains("path")) {
            std::string path = cmd["path"].get<std::string>();
            std::ifstream f(path);
            if (!f.is_open()) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", "failed to open file: " + path}});
                return;
            }
            try {
                layout = json::parse(f);
            } catch (const std::exception& e) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", std::string("JSON parse error: ") + e.what()}});
                return;
            }
        } else {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "missing 'json' or 'path' field"}});
            return;
        }

        std::lock_guard<std::mutex> lock(g_mutex);
        // Clear existing state
        g_windows.clear();
        g_window_order.clear();

        if (layout.contains("windows") && layout["windows"].is_array()) {
            for (const auto& wj : layout["windows"]) {
                WindowState win;
                win.id = wj.value("id", "window");
                win.title = wj.value("title", win.id);
                win.x = wj.value("x", 100.0f);
                win.y = wj.value("y", 100.0f);
                win.w = wj.value("w", 400.0f);
                win.h = wj.value("h", 300.0f);
                win.open = wj.value("open", true);
                win.collapsed = wj.value("collapsed", false);
                win.flags = wj.value("flags", 0);

                if (wj.contains("widgets") && wj["widgets"].is_array()) {
                    for (const auto& widget_j : wj["widgets"]) {
                        std::string wid = widget_j.value("id", "");
                        if (wid.empty()) continue;
                        Widget w;
                        w.id = wid;
                        w.label = widget_j.value("label", wid);
                        std::string wtype = widget_j.value("type", "text");
                        w.type = parse_widget_type(wtype);
                        w.content = widget_j.value("content", "");
                        w.hint = widget_j.value("hint", "");
                        w.tooltip = widget_j.value("tooltip", "");
                        w.overlay = widget_j.value("overlay", "");
                        w.format = widget_j.value("format", "");
                        w.float_val[0] = widget_j.value("value", 0.0f);
                        w.float_min = widget_j.value("min", 0.0f);
                        w.float_max = widget_j.value("max", 1.0f);
                        w.int_val[0] = widget_j.value("int_value", 0);
                        w.int_min = widget_j.value("int_min", 0);
                        w.int_max = widget_j.value("int_max", 100);
                        w.bool_val = widget_j.value("checked", false);
                        if (widget_j.contains("text")) {
                            std::string t = widget_j["text"].get<std::string>();
                            std::strncpy(w.text_buf, t.c_str(), sizeof(w.text_buf) - 1);
                            w.text_buf[sizeof(w.text_buf) - 1] = '\0';
                        }
                        w.selected = widget_j.value("selected", -1);
                        if (widget_j.contains("items") && widget_j["items"].is_array())
                            w.items = widget_j["items"].get<std::vector<std::string>>();
                        if (widget_j.contains("color") && widget_j["color"].is_array()) {
                            const auto& c = widget_j["color"];
                            for (int i = 0; i < 4 && i < (int)c.size(); i++)
                                w.color[i] = c[i].get<float>();
                        }
                        w.size_x = widget_j.value("size_x", 0.0f);
                        w.size_y = widget_j.value("size_y", 0.0f);
                        w.flags = widget_j.value("flags", 0);
                        w.enabled = widget_j.value("enabled", true);
                        w.double_val = widget_j.value("double_value", 0.0);
                        // Import children recursively using deserialize_widget_full
                        if (widget_j.contains("children") && widget_j["children"].is_array()) {
                            for (const auto& child_j : widget_j["children"]) {
                                Widget child = parse_widget_json(child_j);
                                w.children.push_back(child);
                            }
                        }
                        // Table data
                        if (widget_j.contains("columns")) w.columns = widget_j["columns"].get<int>();
                        if (widget_j.contains("headers") && widget_j["headers"].is_array())
                            w.headers = widget_j["headers"].get<std::vector<std::string>>();
                        if (widget_j.contains("rows") && widget_j["rows"].is_array())
                            w.rows = widget_j["rows"].get<std::vector<std::vector<std::string>>>();

                        win.widget_order.push_back(wid);
                        win.widgets[wid] = w;
                    }
                }
                g_window_order.push_back(win.id);
                g_windows[win.id] = win;
            }
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"windows", (int)g_window_order.size()}});

    } else if (type == "load_texture") {
        std::string id = cmd.value("id", "");
        std::string path = cmd.value("path", "");
        LoadedTexture tex; tex.id = id; tex.path = path;
        bool loaded = false;
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            char magic[2] = {0,0}; file.read(magic, 2);
            if (magic[0]=='B' && magic[1]=='M') {
                file.seekg(0, std::ios::beg);
                uint8_t hdr[54]; file.read(reinterpret_cast<char*>(hdr), 54);
                int off=*reinterpret_cast<int*>(&hdr[10]), w=*reinterpret_cast<int*>(&hdr[18]);
                int h=*reinterpret_cast<int*>(&hdr[22]), bpp=*reinterpret_cast<short*>(&hdr[28]);
                if (w>0 && h>0 && (bpp==24||bpp==32)) {
                    int ch=bpp/8, rs=((w*ch+3)/4)*4;
                    std::vector<uint8_t> px(rs*h); file.seekg(off);
                    file.read(reinterpret_cast<char*>(px.data()), px.size());
                    std::vector<uint8_t> rgba(w*h*4);
                    for(int y=0;y<h;y++)for(int x=0;x<w;x++){
                        int si=y*rs+x*ch, di=((h-1-y)*w+x)*4;
                        rgba[di]=px[si+2];rgba[di+1]=px[si+1];rgba[di+2]=px[si];rgba[di+3]=(ch==4)?px[si+3]:255;}
                    glGenTextures(1,&tex.tex_id);glBindTexture(GL_TEXTURE_2D,tex.tex_id);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
                    tex.width=w;tex.height=h;tex.valid=true;loaded=true;
                }
            } else if (magic[0]=='P'&&(magic[1]=='6'||magic[1]=='3')) {
                file.seekg(0,std::ios::beg); std::string hl; std::getline(file,hl);
                bool bin=(hl.size()>1&&hl[1]=='6');
                auto rdi=[&]()->int{std::string t;char c;while(file.get(c)){
                    if(c=='#'){while(file.get(c)&&c!='\n');continue;}
                    if(isspace((unsigned char)c)){if(!t.empty())return std::stoi(t);continue;}t+=c;}
                    return t.empty()?0:std::stoi(t);};
                int w=rdi(),h=rdi(),mv=rdi();
                if(w>0&&h>0&&mv>0){
                    std::vector<uint8_t> rgb(w*h*3);
                    if(bin)file.read(reinterpret_cast<char*>(rgb.data()),rgb.size());
                    else for(int i=0;i<w*h*3;i++)rgb[i]=(uint8_t)(rdi()*255/mv);
                    std::vector<uint8_t> rgba(w*h*4);
                    for(int i=0;i<w*h;i++){rgba[i*4]=rgb[i*3];rgba[i*4+1]=rgb[i*3+1];rgba[i*4+2]=rgb[i*3+2];rgba[i*4+3]=255;}
                    glGenTextures(1,&tex.tex_id);glBindTexture(GL_TEXTURE_2D,tex.tex_id);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
                    tex.width=w;tex.height=h;tex.valid=true;loaded=true;
                }
            }
            file.close();
        }
        if(!loaded){
            const int PW=64,PH=64; std::vector<uint8_t> px(PW*PH*4);
            for(int y=0;y<PH;y++)for(int x=0;x<PW;x++){int i=(y*PW+x)*4;bool ck=((x/8)+(y/8))%2==0;
                px[i]=ck?200:80;px[i+1]=ck?80:200;px[i+2]=ck?200:80;px[i+3]=255;}
            glGenTextures(1,&tex.tex_id);glBindTexture(GL_TEXTURE_2D,tex.tex_id);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,PW,PH,0,GL_RGBA,GL_UNSIGNED_BYTE,px.data());
            tex.width=PW;tex.height=PH;tex.valid=true;
        }
        g_textures[id]=tex;
        emit_json({{"type","ack"},{"cmd",type},{"id",id},{"width",tex.width},{"height",tex.height},{"placeholder",!loaded}});
    } else if (type == "unload_texture") {
        std::string id = cmd.value("id", "");
        auto it = g_textures.find(id);
        if (it != g_textures.end()) {
            if (it->second.tex_id) glDeleteTextures(1, &it->second.tex_id);
            g_textures.erase(it);
        }
        emit_json({{"type","ack"},{"cmd",type},{"id",id}});
    } else if (type == "set_theme") {
        std::string theme = cmd.value("theme", "dark");
        ImGuiStyle& style = ImGui::GetStyle();
        if (theme=="dark"){ImGui::StyleColorsDark(&style);}
        else if (theme=="light"){ImGui::StyleColorsLight(&style);}
        else if (theme=="classic"){ImGui::StyleColorsClassic(&style);}
        else if (theme=="fantasy"){
            ImGui::StyleColorsDark(&style);
            style.WindowRounding=6;style.FrameRounding=4;style.GrabRounding=4;
            style.Colors[ImGuiCol_WindowBg]=ImVec4(.12f,.08f,.04f,1);
            style.Colors[ImGuiCol_Button]=ImVec4(.72f,.52f,.04f,1);
            style.Colors[ImGuiCol_ButtonHovered]=ImVec4(.85f,.65f,.10f,1);
            style.Colors[ImGuiCol_ButtonActive]=ImVec4(.90f,.75f,.20f,1);
            style.Colors[ImGuiCol_Header]=ImVec4(.80f,.65f,.10f,.6f);
            style.Colors[ImGuiCol_HeaderHovered]=ImVec4(.85f,.70f,.15f,.8f);
            style.Colors[ImGuiCol_HeaderActive]=ImVec4(.90f,.75f,.20f,1);
            style.Colors[ImGuiCol_Text]=ImVec4(.95f,.90f,.75f,1);
            style.Colors[ImGuiCol_FrameBg]=ImVec4(.18f,.12f,.06f,1);
            style.Colors[ImGuiCol_Border]=ImVec4(.60f,.45f,.10f,.5f);
        } else if (theme=="scifi"){
            ImGui::StyleColorsDark(&style);
            style.WindowRounding=2;style.FrameRounding=1;style.GrabRounding=1;
            style.Colors[ImGuiCol_WindowBg]=ImVec4(.02f,.04f,.08f,.95f);
            style.Colors[ImGuiCol_Button]=ImVec4(0,.6f,.7f,.8f);
            style.Colors[ImGuiCol_ButtonHovered]=ImVec4(0,.75f,.85f,.9f);
            style.Colors[ImGuiCol_ButtonActive]=ImVec4(0,.9f,1,1);
            style.Colors[ImGuiCol_Header]=ImVec4(0,.5f,.6f,.5f);
            style.Colors[ImGuiCol_HeaderHovered]=ImVec4(0,.65f,.75f,.7f);
            style.Colors[ImGuiCol_HeaderActive]=ImVec4(0,.8f,.9f,.9f);
            style.Colors[ImGuiCol_Text]=ImVec4(0,.95f,1,1);
            style.Colors[ImGuiCol_FrameBg]=ImVec4(.03f,.08f,.12f,1);
            style.Colors[ImGuiCol_Border]=ImVec4(0,.8f,1,.4f);
            style.Colors[ImGuiCol_CheckMark]=ImVec4(0,1,1,1);
            style.Colors[ImGuiCol_SliderGrab]=ImVec4(0,.8f,.9f,1);
            style.Colors[ImGuiCol_SliderGrabActive]=ImVec4(0,1,1,1);
        } else if (theme=="retro"){
            ImGui::StyleColorsDark(&style);
            style.WindowRounding=0;style.FrameRounding=0;style.GrabRounding=0;
            style.ChildRounding=0;style.PopupRounding=0;style.ScrollbarRounding=0;
            style.Colors[ImGuiCol_WindowBg]=ImVec4(.05f,.05f,.05f,1);
            style.Colors[ImGuiCol_Text]=ImVec4(0,.9f,.2f,1);
            style.Colors[ImGuiCol_TextDisabled]=ImVec4(0,.5f,.1f,1);
            style.Colors[ImGuiCol_Button]=ImVec4(0,.3f,.08f,1);
            style.Colors[ImGuiCol_ButtonHovered]=ImVec4(0,.45f,.12f,1);
            style.Colors[ImGuiCol_ButtonActive]=ImVec4(0,.6f,.15f,1);
            style.Colors[ImGuiCol_FrameBg]=ImVec4(.03f,.10f,.03f,1);
            style.Colors[ImGuiCol_Header]=ImVec4(0,.3f,.08f,.6f);
            style.Colors[ImGuiCol_Border]=ImVec4(0,.6f,.15f,.5f);
            style.Colors[ImGuiCol_CheckMark]=ImVec4(0,1,.2f,1);
            style.Colors[ImGuiCol_SliderGrab]=ImVec4(0,.7f,.15f,1);
            style.Colors[ImGuiCol_SliderGrabActive]=ImVec4(0,.9f,.2f,1);
        } else if (theme=="minimal"){
            ImGui::StyleColorsLight(&style);
            style.WindowRounding=0;style.FrameRounding=0;style.GrabRounding=0;
            style.ChildRounding=0;style.PopupRounding=0;style.ScrollbarRounding=0;
            style.FrameBorderSize=1;style.WindowBorderSize=1;
            style.Colors[ImGuiCol_WindowBg]=ImVec4(1,1,1,1);
            style.Colors[ImGuiCol_Text]=ImVec4(.15f,.15f,.15f,1);
            style.Colors[ImGuiCol_Button]=ImVec4(.92f,.92f,.92f,1);
            style.Colors[ImGuiCol_ButtonHovered]=ImVec4(.85f,.85f,.85f,1);
            style.Colors[ImGuiCol_ButtonActive]=ImVec4(.78f,.78f,.78f,1);
            style.Colors[ImGuiCol_FrameBg]=ImVec4(.96f,.96f,.96f,1);
            style.Colors[ImGuiCol_Header]=ImVec4(.90f,.90f,.90f,.6f);
            style.Colors[ImGuiCol_Border]=ImVec4(.75f,.75f,.75f,.6f);
            style.Colors[ImGuiCol_CheckMark]=ImVec4(.3f,.3f,.3f,1);
        } else {ImGui::StyleColorsDark(&style);}
        g_theme_preset = theme;
        emit_json({{"type","ack"},{"cmd",type},{"theme",theme}});
    } else if (type == "set_theme_color") {
        std::string col_name = cmd.value("col_name", "");
        int idx = parse_style_color_index(col_name);
        if (idx>=0 && cmd.contains("color") && cmd["color"].is_array() && cmd["color"].size()>=4) {
            auto& c=cmd["color"];
            ImGui::GetStyle().Colors[idx]=ImVec4(c[0].get<float>(),c[1].get<float>(),c[2].get<float>(),c[3].get<float>());
            emit_json({{"type","ack"},{"cmd",type},{"col_name",col_name},{"col_idx",idx}});
        } else {
            emit_json({{"type","error"},{"cmd",type},{"message","invalid col_name or color array"}});
        }
    } else if (type == "set_style_var") {
        std::string var_name = cmd.value("var_name", "");
        int idx = parse_style_var_index(var_name);
        if (idx>=0 && cmd.contains("value")) {
            ImGuiStyle& style=ImGui::GetStyle();
            const auto& val=cmd["value"];
            if (val.is_array()&&val.size()>=2) {
                float x=val[0].get<float>(),y=val[1].get<float>();
                switch(idx){
                    case ImGuiStyleVar_FramePadding:style.FramePadding=ImVec2(x,y);break;
                    case ImGuiStyleVar_ItemSpacing:style.ItemSpacing=ImVec2(x,y);break;
                    case ImGuiStyleVar_ItemInnerSpacing:style.ItemInnerSpacing=ImVec2(x,y);break;
                    case ImGuiStyleVar_CellPadding:style.CellPadding=ImVec2(x,y);break;
                    case ImGuiStyleVar_WindowTitleAlign:style.WindowTitleAlign=ImVec2(x,y);break;
                    case ImGuiStyleVar_WindowPadding:style.WindowPadding=ImVec2(x,y);break;
                    case ImGuiStyleVar_WindowMinSize:style.WindowMinSize=ImVec2(x,y);break;
                    case ImGuiStyleVar_ButtonTextAlign:style.ButtonTextAlign=ImVec2(x,y);break;
                    case ImGuiStyleVar_SelectableTextAlign:style.SelectableTextAlign=ImVec2(x,y);break;
                    default:break;}
                emit_json({{"type","ack"},{"cmd",type},{"var_name",var_name},{"value",{x,y}}});
            } else if (val.is_number()) {
                float f=val.get<float>();
                switch(idx){
                    case ImGuiStyleVar_Alpha:style.Alpha=f;break;
                    case ImGuiStyleVar_DisabledAlpha:style.DisabledAlpha=f;break;
                    case ImGuiStyleVar_WindowRounding:style.WindowRounding=f;break;
                    case ImGuiStyleVar_WindowBorderSize:style.WindowBorderSize=f;break;
                    case ImGuiStyleVar_ChildRounding:style.ChildRounding=f;break;
                    case ImGuiStyleVar_ChildBorderSize:style.ChildBorderSize=f;break;
                    case ImGuiStyleVar_PopupRounding:style.PopupRounding=f;break;
                    case ImGuiStyleVar_PopupBorderSize:style.PopupBorderSize=f;break;
                    case ImGuiStyleVar_FrameRounding:style.FrameRounding=f;break;
                    case ImGuiStyleVar_FrameBorderSize:style.FrameBorderSize=f;break;
                    case ImGuiStyleVar_IndentSpacing:style.IndentSpacing=f;break;
                    case ImGuiStyleVar_ScrollbarSize:style.ScrollbarSize=f;break;
                    case ImGuiStyleVar_ScrollbarRounding:style.ScrollbarRounding=f;break;
                    case ImGuiStyleVar_GrabMinSize:style.GrabMinSize=f;break;
                    case ImGuiStyleVar_GrabRounding:style.GrabRounding=f;break;
                    case ImGuiStyleVar_TabRounding:style.TabRounding=f;break;
                    case ImGuiStyleVar_TabBorderSize:style.TabBorderSize=f;break;
                    default:break;}
                emit_json({{"type","ack"},{"cmd",type},{"var_name",var_name},{"value",f}});
            } else {
                emit_json({{"type","error"},{"cmd",type},{"message","value must be number or [x,y]"}});
            }
        } else {
            emit_json({{"type","error"},{"cmd",type},{"message","invalid var_name or missing value"}});
        }

    } else if (type == "quit") {
        g_running = false;
        emit_json({{"type", "ack"}, {"cmd", type}});

    } else {
        emit_json({{"type", "error"}, {"message", "unknown command: " + type}});
    }
}
