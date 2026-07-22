// commands.cpp - Command processing for the ImGui MCP application.
// Implements process_command() which handles every JSON command received
// over stdin, plus helpers for parsing widget types and widget definitions.

#include "types.h"

#include "imgui.h"
#include <SDL2/SDL.h>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <utility>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <GL/gl.h>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

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

enum class NumericFormatKind { None, Float, Integer };

static constexpr size_t MAX_NUMERIC_FORMAT_BYTES = 128;
static constexpr size_t MAX_NUMERIC_FIELD_WIDTH = 1024;
static constexpr size_t MAX_NUMERIC_PRECISION = 64;
static constexpr size_t MAX_LAYOUT_WINDOWS = 256;
static constexpr size_t MAX_LAYOUT_WIDGETS = 4096;
static constexpr size_t MAX_WIDGET_NESTING = 32;
static constexpr size_t MAX_STATE_STRING_BYTES = 8u * 1024u * 1024u;
static constexpr size_t MAX_SINGLE_STATE_STRING_BYTES = 4u * 1024u * 1024u;
static constexpr size_t MAX_SERIALIZED_STATE_BYTES = 16u * 1024u * 1024u;
static constexpr size_t MAX_HISTORY_BYTES = 64u * 1024u * 1024u;
static constexpr size_t MAX_SNAPSHOT_BYTES = 64u * 1024u * 1024u;
static constexpr size_t MAX_SNAPSHOTS = 64;
static constexpr size_t MAX_SNAPSHOT_NAME_BYTES = 256;
static constexpr size_t MAX_IMPORT_FILE_BYTES = 4u * 1024u * 1024u;
static constexpr size_t MAX_WIDGET_ITEMS = 1024;
static constexpr size_t MAX_PLOT_VALUES = 4096;
static constexpr size_t MAX_TABLE_HEADERS = 64;
static constexpr size_t MAX_TABLE_ROWS = 1024;
static constexpr size_t MAX_TABLE_CELLS = 16384;
static constexpr int MAX_TABLE_COLUMNS = 64;
static constexpr size_t MAX_DRAW_COMMANDS = 1024;
static constexpr size_t MAX_DRAW_POINTS = 4;
static constexpr int MAX_DRAW_SEGMENTS = 256;
static constexpr int MAX_INVENTORY_DIMENSION = 64;
static constexpr size_t MAX_INVENTORY_CELLS = 4096;
static constexpr int MAX_SKILL_SLOTS = 64;
static constexpr size_t MAX_LAYOUT_ITEMS = 16384;
static constexpr size_t MAX_LAYOUT_PLOT_VALUES = 65536;
static constexpr size_t MAX_LAYOUT_TABLE_HEADERS = 1024;
static constexpr size_t MAX_LAYOUT_TABLE_ROWS = 4096;
static constexpr size_t MAX_LAYOUT_TABLE_CELLS = 65536;
static constexpr size_t MAX_LAYOUT_DRAW_COMMANDS = 4096;
static constexpr size_t MAX_LAYOUT_INVENTORY_CELLS = 16384;
static constexpr size_t MAX_LAYOUT_SKILL_SLOTS = 4096;
static constexpr int MAX_JSON_NESTING_DEPTH = 64;

static NumericFormatKind numeric_format_kind(WidgetType type) {
    switch (type) {
    case WidgetType::InputFloat:
    case WidgetType::InputFloat2:
    case WidgetType::InputFloat3:
    case WidgetType::InputFloat4:
    case WidgetType::InputDouble:
    case WidgetType::SliderFloat:
    case WidgetType::SliderFloat2:
    case WidgetType::SliderFloat3:
    case WidgetType::SliderFloat4:
    case WidgetType::SliderAngle:
    case WidgetType::VSliderFloat:
    case WidgetType::DragFloat:
    case WidgetType::DragFloat2:
    case WidgetType::DragFloat3:
    case WidgetType::DragFloat4:
    case WidgetType::DragFloatRange2:
    case WidgetType::ValueFloat:
        return NumericFormatKind::Float;
    case WidgetType::SliderInt:
    case WidgetType::SliderInt2:
    case WidgetType::SliderInt3:
    case WidgetType::SliderInt4:
    case WidgetType::VSliderInt:
    case WidgetType::DragInt:
    case WidgetType::DragInt2:
    case WidgetType::DragInt3:
    case WidgetType::DragInt4:
    case WidgetType::DragIntRange2:
        return NumericFormatKind::Integer;
    default:
        return NumericFormatKind::None;
    }
}

static bool consume_bounded_decimal(const std::string& value, size_t& offset,
                                    size_t maximum) {
    size_t parsed = 0;
    while (offset < value.size() &&
           std::isdigit(static_cast<unsigned char>(value[offset]))) {
        const size_t digit = static_cast<size_t>(value[offset] - '0');
        if (parsed > (maximum - digit) / 10)
            return false;
        parsed = parsed * 10 + digit;
        ++offset;
    }
    return true;
}

static bool is_safe_numeric_format(const std::string& format, NumericFormatKind kind) {
    if (format.empty() || kind == NumericFormatKind::None)
        return true;
    if (format.size() > MAX_NUMERIC_FORMAT_BYTES)
        return false;

    const std::string conversions = kind == NumericFormatKind::Float
        ? "aAeEfFgG" : "diuoxX";
    int conversion_count = 0;
    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%')
            continue;
        if (i + 1 < format.size() && format[i + 1] == '%') {
            ++i;
            continue;
        }

        ++i;
        while (i < format.size() && std::strchr("-+ #0", format[i]))
            ++i;
        if (i < format.size() && format[i] == '*')
            return false;
        if (!consume_bounded_decimal(format, i, MAX_NUMERIC_FIELD_WIDTH))
            return false;
        if (i < format.size() && format[i] == '.') {
            ++i;
            if (i < format.size() && format[i] == '*')
                return false;
            if (!consume_bounded_decimal(format, i, MAX_NUMERIC_PRECISION))
                return false;
        }
        if (i >= format.size() || conversions.find(format[i]) == std::string::npos)
            return false;
        ++conversion_count;
    }
    return conversion_count == 1;
}

bool is_safe_widget_format(const Widget& widget) {
    return is_safe_numeric_format(widget.format, numeric_format_kind(widget.type));
}

static void validate_widget_format(const Widget& widget) {
    const NumericFormatKind kind = numeric_format_kind(widget.type);
    if (!widget.format.empty() && kind != NumericFormatKind::None &&
        !is_safe_widget_format(widget)) {
        throw std::invalid_argument(
            kind == NumericFormatKind::Float
                ? "format must contain exactly one floating-point conversion "
                  "(max 128 bytes, width 1024, precision 64)"
                : "format must contain exactly one integer conversion "
                  "(max 128 bytes, width 1024, precision 64)");
    }
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
    if (v.is_number_integer()) {
        const int index = v.get<int>();
        return index >= 0 && index < ImGuiCol_COUNT ? index : -1;
    }
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
    if (v.is_number_integer()) {
        const int index = v.get<int>();
        return index >= 0 && index < ImGuiStyleVar_COUNT ? index : -1;
    }
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

static bool style_var_is_vec2(int index) {
    switch (index) {
    case ImGuiStyleVar_WindowPadding:
    case ImGuiStyleVar_WindowMinSize:
    case ImGuiStyleVar_WindowTitleAlign:
    case ImGuiStyleVar_FramePadding:
    case ImGuiStyleVar_ItemSpacing:
    case ImGuiStyleVar_ItemInnerSpacing:
    case ImGuiStyleVar_CellPadding:
    case ImGuiStyleVar_TableAngledHeadersTextAlign:
    case ImGuiStyleVar_ButtonTextAlign:
    case ImGuiStyleVar_SelectableTextAlign:
    case ImGuiStyleVar_SeparatorTextAlign:
    case ImGuiStyleVar_SeparatorTextPadding:
        return true;
    default:
        return false;
    }
}

static int g_style_color_depth = 0;
static int g_style_var_depth = 0;
static float g_dpi_scale = 1.0f;

// ─── Widget JSON parsing ────────────────────────────────────────────────────

static Widget parse_widget_json_at_depth(const json& j, size_t depth,
                                         size_t& widget_count);

// Apply every optional field present in `j` onto widget `w`. Used both when
// creating a widget (add_widget) and when patching one (update_widget).
static void apply_widget_fields(Widget& w, const json& j, size_t depth = 1,
                                size_t* parsed_widget_count = nullptr) {
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
        if (v.size() > MAX_PLOT_VALUES)
            throw std::length_error("plot value count exceeds the limit of 4096");
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
        if (j["items"].size() > MAX_WIDGET_ITEMS)
            throw std::length_error("widget item count exceeds the limit of 1024");
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
        if (j["headers"].size() > MAX_TABLE_HEADERS)
            throw std::length_error("table header count exceeds the limit of 64");
        w.headers.clear();
        for (const auto& h : j["headers"])
            w.headers.push_back(h.get<std::string>());
    }

    // rows[][]
    if (j.contains("rows") && j["rows"].is_array()) {
        if (j["rows"].size() > MAX_TABLE_ROWS)
            throw std::length_error("table row count exceeds the limit of 1024");
        w.rows.clear();
        size_t cell_count = 0;
        for (const auto& r : j["rows"]) {
            std::vector<std::string> row;
            if (r.is_array()) {
                if (r.size() > MAX_TABLE_CELLS - cell_count)
                    throw std::length_error("table cell count exceeds the limit of 16384");
                cell_count += r.size();
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
        size_t local_widget_count = 1;
        size_t& widget_count = parsed_widget_count
            ? *parsed_widget_count : local_widget_count;
        for (const auto& c : j["children"])
            w.children.push_back(parse_widget_json_at_depth(c, depth + 1,
                                                             widget_count));
    }

    if (j.contains("open") && j["open"].is_boolean()) w.tree_open = j["open"].get<bool>();
    if (j.contains("popup_open") && j["popup_open"].is_boolean()) w.popup_open = j["popup_open"].get<bool>();
}

// Parse a full widget definition from JSON (used by add_widget and the
// recursive "children" arrays).
static Widget parse_widget_json_at_depth(const json& j, size_t depth,
                                         size_t& widget_count) {
    if (depth > MAX_WIDGET_NESTING)
        throw std::length_error("widget nesting exceeds the limit of 32");
    if (widget_count >= MAX_LAYOUT_WIDGETS)
        throw std::length_error("widget count exceeds the limit of 4096");
    ++widget_count;

    Widget w;
    w.id = j.value("id", "");
    w.type = parse_widget_type(j.value("widget_type", "text"));
    w.label = j.value("label", w.id);
    apply_widget_fields(w, j, depth, &widget_count);
    validate_widget_format(w);
    return w;
}

Widget parse_widget_json(const json& j) {
    size_t widget_count = 0;
    return parse_widget_json_at_depth(j, 1, widget_count);
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
Widget* find_widget_in_window(WindowState& win, const std::string& id) {
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
    if (s == "convex_poly_filled" || s == "convex_poly") return 8;
    if (s == "quad") return 9;
    if (s == "quad_filled") return 10;
    if (s == "text") return 11;
    if (s == "bezier_cubic") return 12;
    if (s == "bezier_quadratic") return 13;
    if (s == "ellipse") return 14;
    if (s == "ellipse_filled") return 15;
    if (s == "rect_gradient") return 16;
    if (s == "arc") return 17;
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
        if (pts.size() > MAX_DRAW_POINTS)
            throw std::length_error("draw point count exceeds the limit of 4");
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
    auto read_color = [&](const char* key, float dst[4]) {
        if (!j.contains(key) || !j[key].is_array()) return;
        const json& c = j[key];
        for (int i = 0; i < 4 && i < (int)c.size(); ++i)
            dst[i] = c[i].get<float>();
    };
    std::copy(std::begin(dc.color), std::end(dc.color), dc.color2);
    std::copy(std::begin(dc.color), std::end(dc.color), dc.color3);
    std::copy(std::begin(dc.color), std::end(dc.color), dc.color4);
    read_color("color2", dc.color2);
    read_color("color3", dc.color3);
    read_color("color4", dc.color4);
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

static bool is_ease_type(const std::string& ease) {
    static const std::vector<std::string> names = {
        "linear", "ease_in", "ease_out", "ease_in_out", "bounce", "elastic", "back"};
    return std::find(names.begin(), names.end(), ease) != names.end();
}

static const char* ease_type_name(EaseType ease) {
    switch (ease) {
        case EaseType::EaseIn: return "ease_in";
        case EaseType::EaseOut: return "ease_out";
        case EaseType::EaseInOut: return "ease_in_out";
        case EaseType::Bounce: return "bounce";
        case EaseType::Elastic: return "elastic";
        case EaseType::Back: return "back";
        default: return "linear";
    }
}

static bool is_animation_property(const std::string& property) {
    static const std::vector<std::string> properties = {
        "value", "opacity", "pos_x", "pos_y", "size_x", "size_y", "int_value"};
    return std::find(properties.begin(), properties.end(), property) != properties.end();
}

static const ImWchar* font_glyph_ranges(const std::string& preset) {
    ImFontAtlas* atlas = ImGui::GetIO().Fonts;
    if (preset == "default" || preset == "unicode" || preset == "symbols")
        return nullptr; // ImGui 1.92 resolves these dynamically across merged sources.
    if (preset == "cyrillic") return atlas->GetGlyphRangesCyrillic();
    if (preset == "greek") return atlas->GetGlyphRangesGreek();
    if (preset == "vietnamese") return atlas->GetGlyphRangesVietnamese();
    if (preset == "thai") return atlas->GetGlyphRangesThai();
    if (preset == "japanese") return atlas->GetGlyphRangesJapanese();
    if (preset == "korean") return atlas->GetGlyphRangesKorean();
    if (preset == "chinese_full") return atlas->GetGlyphRangesChineseFull();
    if (preset == "chinese_simplified")
        return atlas->GetGlyphRangesChineseSimplifiedCommon();
    throw std::invalid_argument("unknown font glyph_ranges preset: " + preset);
}

static ImFont* load_font_source(const std::string& id, const std::string& path,
                                float size_pixels, const std::string& glyph_ranges,
                                const std::string& merge_into) {
    if (id.empty() || id.size() > 128)
        throw std::invalid_argument("font id must contain 1-128 characters");
    if (id == "default")
        throw std::invalid_argument("font id 'default' is reserved");
    if (g_fonts.size() >= 64)
        throw std::length_error("font count exceeds the limit of 64");
    if (g_fonts.count(id) != 0)
        throw std::invalid_argument("font id is already loaded: " + id);
    if (size_pixels < 8.0f || size_pixels > 96.0f)
        throw std::invalid_argument("font size_pixels must be between 8 and 96");
    std::error_code font_path_error;
    if (path.empty() || !std::filesystem::is_regular_file(path, font_path_error) ||
        font_path_error)
        throw std::invalid_argument("font file does not exist: " + path);

    ImFontConfig config;
    if (!merge_into.empty()) {
        const auto target = g_fonts.find(merge_into);
        if (target == g_fonts.end())
            throw std::invalid_argument("merge target font is not loaded: " + merge_into);
        if (!target->second.merge_into.empty())
            throw std::invalid_argument("merge target must be a base font: " + merge_into);
        config.MergeMode = true;
        config.DstFont = target->second.font;
    }
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(
        path.c_str(), size_pixels, merge_into.empty() ? nullptr : &config,
        font_glyph_ranges(glyph_ranges));
    if (!font)
        throw std::runtime_error("failed to load font: " + path);
    g_fonts.emplace(id, LoadedFont{
        id, path, size_pixels, glyph_ranges, merge_into, font});
    return font;
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
        case WidgetType::Image: return "image";
        case WidgetType::ImageButton: return "image_button";
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
            dj["color2"] = {dc.color2[0], dc.color2[1], dc.color2[2], dc.color2[3]};
            dj["color3"] = {dc.color3[0], dc.color3[1], dc.color3[2], dc.color3[3]};
            dj["color4"] = {dc.color4[0], dc.color4[1], dc.color4[2], dc.color4[3]};
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
static Widget deserialize_widget_full_at_depth(const json& j, size_t depth,
                                               size_t& widget_count) {
    if (depth > MAX_WIDGET_NESTING)
        throw std::length_error("widget nesting exceeds the limit of 32");
    if (widget_count >= MAX_LAYOUT_WIDGETS)
        throw std::length_error("widget count exceeds the limit of 4096");
    ++widget_count;

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
        if (j["items"].size() > MAX_WIDGET_ITEMS)
            throw std::length_error("widget item count exceeds the limit of 1024");
        w.items.clear();
        for (const auto& it : j["items"])
            w.items.push_back(it.get<std::string>());
    }
    w.selected = j.value("selected", -1);
    if (j.contains("plot_values") && j["plot_values"].is_array()) {
        if (j["plot_values"].size() > MAX_PLOT_VALUES)
            throw std::length_error("plot value count exceeds the limit of 4096");
        w.plot_values.clear();
        for (const auto& v : j["plot_values"])
            w.plot_values.push_back(v.get<float>());
    }
    w.columns = j.value("columns", 0);
    if (j.contains("headers") && j["headers"].is_array()) {
        if (j["headers"].size() > MAX_TABLE_HEADERS)
            throw std::length_error("table header count exceeds the limit of 64");
        w.headers.clear();
        for (const auto& h : j["headers"])
            w.headers.push_back(h.get<std::string>());
    }
    if (j.contains("rows") && j["rows"].is_array()) {
        if (j["rows"].size() > MAX_TABLE_ROWS)
            throw std::length_error("table row count exceeds the limit of 1024");
        w.rows.clear();
        size_t cell_count = 0;
        for (const auto& r : j["rows"]) {
            std::vector<std::string> row;
            if (r.is_array()) {
                if (r.size() > MAX_TABLE_CELLS - cell_count)
                    throw std::length_error("table cell count exceeds the limit of 16384");
                cell_count += r.size();
                for (const auto& cell : r)
                    row.push_back(cell.is_string() ? cell.get<std::string>() : cell.dump());
            }
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
            w.children.push_back(deserialize_widget_full_at_depth(
                c, depth + 1, widget_count));
    }
    if (j.contains("draw_commands") && j["draw_commands"].is_array()) {
        if (j["draw_commands"].size() > MAX_DRAW_COMMANDS)
            throw std::length_error("draw command count exceeds the limit of 1024");
        w.draw_commands.clear();
        for (const auto& dj : j["draw_commands"]) {
            DrawCommand dc;
            dc.type = dj.value("type", 0);
            if (dj.contains("p1")) { dc.p1[0] = dj["p1"][0].get<float>(); dc.p1[1] = dj["p1"][1].get<float>(); }
            if (dj.contains("p2")) { dc.p2[0] = dj["p2"][0].get<float>(); dc.p2[1] = dj["p2"][1].get<float>(); }
            if (dj.contains("p3")) { dc.p3[0] = dj["p3"][0].get<float>(); dc.p3[1] = dj["p3"][1].get<float>(); }
            if (dj.contains("p4")) { dc.p4[0] = dj["p4"][0].get<float>(); dc.p4[1] = dj["p4"][1].get<float>(); }
            if (dj.contains("color")) { for (int i = 0; i < 4 && i < (int)dj["color"].size(); i++) dc.color[i] = dj["color"][i].get<float>(); }
            auto restore_color = [&](const char* key, float dst[4]) {
                std::copy(std::begin(dc.color), std::end(dc.color), dst);
                if (dj.contains(key))
                    for (int i = 0; i < 4 && i < (int)dj[key].size(); ++i)
                        dst[i] = dj[key][i].get<float>();
            };
            restore_color("color2", dc.color2);
            restore_color("color3", dc.color3);
            restore_color("color4", dc.color4);
            dc.thickness = dj.value("thickness", 1.0f);
            dc.filled = dj.value("filled", false);
            dc.num_segments = dj.value("num_segments", 0);
            dc.text = dj.value("text", "");
            w.draw_commands.push_back(dc);
        }
    }
    validate_widget_format(w);
    return w;
}

struct StateUsage {
    size_t widget_count = 0;
    size_t string_bytes = 0;
    size_t item_count = 0;
    size_t plot_value_count = 0;
    size_t table_header_count = 0;
    size_t table_row_count = 0;
    size_t table_cell_count = 0;
    size_t draw_command_count = 0;
    size_t inventory_cell_count = 0;
    size_t skill_slot_count = 0;
};

static void account_state_string(StateUsage& usage, const std::string& value) {
    if (value.size() > MAX_SINGLE_STATE_STRING_BYTES)
        throw std::length_error("state string exceeds the 4 MiB limit");
    if (value.size() > MAX_STATE_STRING_BYTES - usage.string_bytes)
        throw std::length_error("aggregate state strings exceed the 8 MiB limit");
    usage.string_bytes += value.size();
}

static void account_state_elements(size_t count, size_t per_widget_limit,
                                   size_t& aggregate, size_t aggregate_limit,
                                   const char* per_widget_error,
                                   const char* aggregate_error) {
    if (count > per_widget_limit)
        throw std::length_error(per_widget_error);
    if (count > aggregate_limit - aggregate)
        throw std::length_error(aggregate_error);
    aggregate += count;
}

static void measure_widget_state(const Widget& widget, size_t depth,
                                 StateUsage& usage) {
    if (depth > MAX_WIDGET_NESTING)
        throw std::length_error("widget nesting exceeds the limit of 32");
    if (usage.widget_count >= MAX_LAYOUT_WIDGETS)
        throw std::length_error("widget count exceeds the limit of 4096");
    ++usage.widget_count;

    account_state_elements(
        widget.items.size(), MAX_WIDGET_ITEMS, usage.item_count, MAX_LAYOUT_ITEMS,
        "widget item count exceeds the limit of 1024",
        "aggregate widget item count exceeds the limit of 16384");
    account_state_elements(
        widget.plot_values.size(), MAX_PLOT_VALUES, usage.plot_value_count,
        MAX_LAYOUT_PLOT_VALUES,
        "plot value count exceeds the limit of 4096",
        "aggregate plot value count exceeds the limit of 65536");
    account_state_elements(
        widget.headers.size(), MAX_TABLE_HEADERS, usage.table_header_count,
        MAX_LAYOUT_TABLE_HEADERS,
        "table header count exceeds the limit of 64",
        "aggregate table header count exceeds the limit of 1024");
    account_state_elements(
        widget.rows.size(), MAX_TABLE_ROWS, usage.table_row_count,
        MAX_LAYOUT_TABLE_ROWS,
        "table row count exceeds the limit of 1024",
        "aggregate table row count exceeds the limit of 4096");

    size_t widget_cell_count = 0;
    for (const auto& row : widget.rows) {
        if (row.size() > MAX_TABLE_CELLS - widget_cell_count)
            throw std::length_error("table cell count exceeds the limit of 16384");
        widget_cell_count += row.size();
    }
    account_state_elements(
        widget_cell_count, MAX_TABLE_CELLS, usage.table_cell_count,
        MAX_LAYOUT_TABLE_CELLS,
        "table cell count exceeds the limit of 16384",
        "aggregate table cell count exceeds the limit of 65536");

    if (widget.columns > MAX_TABLE_COLUMNS)
        throw std::length_error("table column count exceeds the limit of 64");
    account_state_elements(
        widget.draw_commands.size(), MAX_DRAW_COMMANDS,
        usage.draw_command_count, MAX_LAYOUT_DRAW_COMMANDS,
        "draw command count exceeds the limit of 1024",
        "aggregate draw command count exceeds the limit of 4096");
    for (const auto& command : widget.draw_commands) {
        if (command.num_segments < 0)
            throw std::length_error("draw segment count cannot be negative");
        if (command.num_segments > MAX_DRAW_SEGMENTS)
            throw std::length_error("draw segment count exceeds the limit of 256");
    }

    if (widget.type == WidgetType::InventoryGrid) {
        const int columns = widget.int_val[0] > 0 ? widget.int_val[0] : 4;
        const int rows = widget.int_val[1] > 0 ? widget.int_val[1] : 4;
        if (columns > MAX_INVENTORY_DIMENSION ||
            rows > MAX_INVENTORY_DIMENSION ||
            static_cast<size_t>(columns) * static_cast<size_t>(rows) >
                MAX_INVENTORY_CELLS) {
            throw std::length_error(
                "inventory_grid dimensions exceed the limit of 64 columns, "
                "64 rows, or 4096 cells");
        }
        account_state_elements(
            static_cast<size_t>(columns) * static_cast<size_t>(rows),
            MAX_INVENTORY_CELLS, usage.inventory_cell_count,
            MAX_LAYOUT_INVENTORY_CELLS,
            "inventory_grid cell count exceeds the limit of 4096",
            "aggregate inventory_grid cell count exceeds the limit of 16384");
    }
    if (widget.type == WidgetType::SkillBar) {
        const int slots = widget.int_val[0] > 0 ? widget.int_val[0] : 4;
        if (slots > MAX_SKILL_SLOTS)
            throw std::length_error("skill_bar count exceeds the limit of 64");
        account_state_elements(
            static_cast<size_t>(slots), MAX_SKILL_SLOTS,
            usage.skill_slot_count, MAX_LAYOUT_SKILL_SLOTS,
            "skill_bar count exceeds the limit of 64",
            "aggregate skill_bar slot count exceeds the limit of 4096");
    }

    account_state_string(usage, widget.id);
    account_state_string(usage, widget.label);
    account_state_string(usage, widget.content);
    account_state_string(usage, widget.hint);
    account_state_string(usage, widget.tooltip);
    account_state_string(usage, widget.shortcut);
    account_state_string(usage, widget.overlay);
    account_state_string(usage, widget.format);
    account_state_string(usage, std::string(widget.text_buf));
    account_state_string(usage, widget.visible_condition);
    for (const auto& item : widget.items)
        account_state_string(usage, item);
    for (const auto& header : widget.headers)
        account_state_string(usage, header);
    for (const auto& row : widget.rows)
        for (const auto& cell : row)
            account_state_string(usage, cell);
    for (const auto& command : widget.draw_commands)
        account_state_string(usage, command.text);
    for (const auto& child : widget.children)
        measure_widget_state(child, depth + 1, usage);
}

static void validate_state_structure(
    const std::map<std::string, WindowState>& windows,
    const std::vector<std::string>& window_order) {
    if (windows.size() > MAX_LAYOUT_WINDOWS ||
        window_order.size() > MAX_LAYOUT_WINDOWS) {
        throw std::length_error("window count exceeds the limit of 256");
    }

    StateUsage usage;
    for (const auto& id : window_order)
        account_state_string(usage, id);
    for (const auto& [window_key, window] : windows) {
        account_state_string(usage, window_key);
        account_state_string(usage, window.id);
        account_state_string(usage, window.title);
        for (const auto& id : window.widget_order)
            account_state_string(usage, id);
        for (const auto& [widget_key, widget] : window.widgets) {
            account_state_string(usage, widget_key);
            measure_widget_state(widget, 1, usage);
        }
    }
}

// Serialize the entire layout state (all windows and widgets).
static json serialize_state_unchecked(
    const std::map<std::string, WindowState>& windows,
    const std::vector<std::string>& window_order) {
    json state = json::object();
    state["window_order"] = window_order;
    state["windows"] = json::object();
    for (const auto& [wid, win] : windows) {
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
        for (const auto& [gid, w] : win.widgets)
            wj["widgets"][gid] = serialize_widget_full(w);
        state["windows"][wid] = wj;
    }
    return state;
}

static json serialize_validated_state(
    const std::map<std::string, WindowState>& windows,
    const std::vector<std::string>& window_order) {
    validate_state_structure(windows, window_order);
    json state = serialize_state_unchecked(windows, window_order);
    if (state.dump().size() > MAX_SERIALIZED_STATE_BYTES)
        throw std::length_error("serialized state exceeds the 16 MiB limit");
    return state;
}

static void validate_state_limits(
    const std::map<std::string, WindowState>& windows,
    const std::vector<std::string>& window_order) {
    (void)serialize_validated_state(windows, window_order);
}

// Caller MUST hold g_mutex.
static json serialize_state() {
    return serialize_validated_state(g_windows, g_window_order);
}

// Restore the entire layout state from a serialized snapshot.
// Caller MUST hold g_mutex.
static void restore_state(const json& state) {
    std::map<std::string, WindowState> restored_windows;
    std::vector<std::string> restored_order;
    if (state.contains("window_order") && state["window_order"].is_array()) {
        if (state["window_order"].size() > MAX_LAYOUT_WINDOWS)
            throw std::length_error("window count exceeds the limit of 256");
        for (const auto& wid : state["window_order"])
            restored_order.push_back(wid.get<std::string>());
    }
    size_t widget_count = 0;
    if (state.contains("windows") && state["windows"].is_object()) {
        if (state["windows"].size() > MAX_LAYOUT_WINDOWS)
            throw std::length_error("window count exceeds the limit of 256");
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
                    win.widgets[gid] = deserialize_widget_full_at_depth(
                        wjson, 1, widget_count);
            }
            restored_windows[wid] = std::move(win);
        }
    }
    validate_state_limits(restored_windows, restored_order);
    g_windows.swap(restored_windows);
    g_window_order.swap(restored_order);
}

static size_t g_undo_bytes = 0;
static size_t g_redo_bytes = 0;
static size_t g_snapshot_bytes = 0;

static size_t serialized_json_bytes(const json& value) {
    return value.dump().size();
}

static void remove_oldest_history_entry(std::vector<json>& stack,
                                        size_t& byte_count) {
    if (stack.empty())
        return;
    const size_t bytes = serialized_json_bytes(stack.front());
    byte_count = bytes > byte_count ? 0 : byte_count - bytes;
    stack.erase(stack.begin());
}

static void trim_history_storage() {
    while (static_cast<int>(g_undo_stack.size()) > g_max_undo)
        remove_oldest_history_entry(g_undo_stack, g_undo_bytes);
    while (g_undo_bytes + g_redo_bytes > MAX_HISTORY_BYTES) {
        if (!g_undo_stack.empty())
            remove_oldest_history_entry(g_undo_stack, g_undo_bytes);
        else if (!g_redo_stack.empty())
            remove_oldest_history_entry(g_redo_stack, g_redo_bytes);
        else
            break;
    }
}

// Push current state onto the undo stack. Caller MUST hold g_mutex.
static void push_undo_state() {
    json state = serialize_state();
    const size_t state_bytes = serialized_json_bytes(state);
    g_undo_stack.push_back(std::move(state));
    g_undo_bytes += state_bytes;
    g_redo_stack.clear(); // new action invalidates redo history
    g_redo_bytes = 0;
    trim_history_storage();
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

static bool is_protocol_stdout_path(const std::filesystem::path& path) {
    std::string normalized = path.lexically_normal().generic_string();
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalized == "-" || normalized == "/dev/stdout" ||
        normalized == "/dev/fd/1" || normalized == "/proc/self/fd/1") {
        return true;
    }

#ifdef _WIN32
    std::string leaf = path.filename().string();
    std::transform(leaf.begin(), leaf.end(), leaf.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    while (!leaf.empty() && (leaf.back() == ' ' || leaf.back() == '.' ||
                             leaf.back() == ':'))
        leaf.pop_back();
    const size_t dot = leaf.find('.');
    const std::string device = leaf.substr(0, dot);
    const bool numbered_device = device.size() == 4 &&
        (device.compare(0, 3, "com") == 0 ||
         device.compare(0, 3, "lpt") == 0) &&
        device[3] >= '1' && device[3] <= '9';
    if (device == "con" || device == "conin$" || device == "conout$" ||
        device == "prn" || device == "aux" || device == "nul" ||
        device == "clock$" || numbered_device) {
        return true;
    }
#else
    for (const char* descriptor_path : {"/proc/self/fd/1", "/dev/fd/1"}) {
        std::error_code error;
        if (std::filesystem::equivalent(path, descriptor_path, error) && !error)
            return true;
    }
#endif
    return false;
}

static void validate_regular_file_path(const std::string& raw_path,
                                       bool must_exist) {
    if (raw_path.empty())
        throw std::invalid_argument("file path cannot be empty");

    const std::filesystem::path path(raw_path);
    if (is_protocol_stdout_path(path)) {
        throw std::invalid_argument(
            "file path must be a regular file and must not target protocol stdout");
    }

    std::error_code error;
    const std::filesystem::file_status status =
        std::filesystem::symlink_status(path, error);
    if (error && error != std::errc::no_such_file_or_directory)
        throw std::runtime_error("failed to inspect file path: " + raw_path);

    const bool exists = !error && std::filesystem::exists(status);
    if (must_exist && !exists)
        throw std::runtime_error("file does not exist: " + raw_path);
    if (exists && (std::filesystem::is_symlink(status) ||
                   !std::filesystem::is_regular_file(status))) {
        throw std::invalid_argument("file path must be a regular file: " + raw_path);
    }
}

#ifndef _WIN32
static int open_posix_regular_file(const std::string& path, int access_flags,
                                   bool truncate_file, off_t& file_size,
                                   const char* purpose) {
    int flags = access_flags | O_NONBLOCK;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
    if (access_flags != O_RDONLY)
        flags |= O_CREAT;

    const int descriptor = access_flags == O_RDONLY
        ? open(path.c_str(), flags)
        : open(path.c_str(), flags, 0666);
    if (descriptor < 0)
        throw std::runtime_error(std::string("failed to open ") + purpose +
                                 " file: " + path);

    struct stat status {};
    if (fstat(descriptor, &status) != 0 || !S_ISREG(status.st_mode)) {
        close(descriptor);
        throw std::invalid_argument("file path must be a regular file: " + path);
    }

    struct stat stdout_status {};
    if (fstat(STDOUT_FILENO, &stdout_status) == 0 &&
        status.st_dev == stdout_status.st_dev &&
        status.st_ino == stdout_status.st_ino) {
        close(descriptor);
        throw std::invalid_argument(
            "file path must not target the protocol stdout stream: " + path);
    }

    file_size = status.st_size;
    if (truncate_file && ftruncate(descriptor, 0) != 0) {
        close(descriptor);
        throw std::runtime_error("failed to truncate export file: " + path);
    }
    return descriptor;
}
#endif

static void write_export_file(const std::string& path,
                              const std::string& contents) {
    validate_regular_file_path(path, false);
#ifdef _WIN32
    if (contents.size() > static_cast<size_t>(
            std::numeric_limits<std::streamsize>::max())) {
        throw std::length_error("export is too large to write");
    }

    std::ofstream file(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file.is_open())
        throw std::runtime_error("failed to open export file: " + path);
    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file)
        throw std::runtime_error("failed to write export file: " + path);
    file.flush();
    if (!file)
        throw std::runtime_error("failed to flush export file: " + path);
    file.close();
    if (file.fail())
        throw std::runtime_error("failed to close export file: " + path);
#else
    off_t ignored_size = 0;
    const int descriptor = open_posix_regular_file(
        path, O_WRONLY, true, ignored_size, "export");
    FILE* file = fdopen(descriptor, "wb");
    if (!file) {
        close(descriptor);
        throw std::runtime_error("failed to open export file stream: " + path);
    }

    if (std::fwrite(contents.data(), 1, contents.size(), file) != contents.size()) {
        std::fclose(file);
        throw std::runtime_error("failed to write export file: " + path);
    }
    if (std::fflush(file) != 0) {
        std::fclose(file);
        throw std::runtime_error("failed to flush export file: " + path);
    }
    if (std::fclose(file) != 0)
        throw std::runtime_error("failed to close export file: " + path);
#endif
}

static std::string read_import_file(const std::string& path) {
    validate_regular_file_path(path, true);

#ifdef _WIN32
    std::error_code error;
    const uintmax_t expected_size = std::filesystem::file_size(path, error);
    if (error)
        throw std::runtime_error("failed to determine import file size: " + path);
    if (expected_size > MAX_IMPORT_FILE_BYTES)
        throw std::length_error("import file exceeds the 4 MiB limit");

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("failed to open import file: " + path);

    std::string contents;
    contents.reserve(static_cast<size_t>(expected_size));
    char buffer[64 * 1024];
    while (file) {
        file.read(buffer, sizeof(buffer));
        const std::streamsize count = file.gcount();
        if (count > 0) {
            const size_t bytes = static_cast<size_t>(count);
            if (bytes > MAX_IMPORT_FILE_BYTES - contents.size())
                throw std::length_error("import file exceeds the 4 MiB limit");
            contents.append(buffer, bytes);
        }
    }
    if (!file.eof())
        throw std::runtime_error("failed to read import file: " + path);
    file.clear();
    file.close();
    if (file.fail())
        throw std::runtime_error("failed to close import file: " + path);
    return contents;
#else
    off_t descriptor_size = 0;
    const int descriptor = open_posix_regular_file(
        path, O_RDONLY, false, descriptor_size, "import");
    if (descriptor_size < 0 ||
        static_cast<uintmax_t>(descriptor_size) > MAX_IMPORT_FILE_BYTES) {
        close(descriptor);
        throw std::length_error("import file exceeds the 4 MiB limit");
    }

    FILE* file = fdopen(descriptor, "rb");
    if (!file) {
        close(descriptor);
        throw std::runtime_error("failed to open import file stream: " + path);
    }

    std::string contents;
    contents.reserve(static_cast<size_t>(descriptor_size));
    char buffer[64 * 1024];
    while (true) {
        const size_t count = std::fread(buffer, 1, sizeof(buffer), file);
        if (count > 0) {
            if (count > MAX_IMPORT_FILE_BYTES - contents.size()) {
                std::fclose(file);
                throw std::length_error("import file exceeds the 4 MiB limit");
            }
            contents.append(buffer, count);
        }
        if (count < sizeof(buffer)) {
            if (std::ferror(file)) {
                std::fclose(file);
                throw std::runtime_error("failed to read import file: " + path);
            }
            break;
        }
    }
    if (std::fclose(file) != 0)
        throw std::runtime_error("failed to close import file: " + path);
    return contents;
#endif
}

static json parse_json_with_depth_limit(const std::string& contents) {
    bool depth_exceeded = false;
    auto depth_callback = [&depth_exceeded](int depth, json::parse_event_t event,
                                            json&) {
        const bool starts_container =
            event == json::parse_event_t::object_start ||
            event == json::parse_event_t::array_start;
        if (starts_container && depth >= MAX_JSON_NESTING_DEPTH) {
            depth_exceeded = true;
            return false;
        }
        return true;
    };
    json parsed = json::parse(contents, depth_callback);
    if (depth_exceeded)
        throw std::length_error("JSON nesting exceeds the limit of 64");
    return parsed;
}

// Make a valid C++ identifier from a widget id (replace non-alnum with _).
static std::string sanitize_id(const std::string& id) {
    std::string out;
    uint32_t hash = 2166136261u;
    for (char c : id) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 16777619u;
        out += (std::isalnum(static_cast<unsigned char>(c)) || c == '_') ? c : '_';
    }
    if (!out.empty() && std::isdigit(static_cast<unsigned char>(out[0])))
        out = "_" + out;
    if (out.empty())
        out = "w";
    return out + "_" + std::to_string(hash);
}

// Escape a string for use inside C++ double-quoted string literals.
static std::string cpp_escape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\v') out += "\\v";
        else if (c < 0x20 || c == 0x7f) {
            char escaped[4] = {
                '\\',
                static_cast<char>('0' + ((c >> 6) & 0x07)),
                static_cast<char>('0' + ((c >> 3) & 0x07)),
                static_cast<char>('0' + (c & 0x07)),
            };
            out.append(escaped, sizeof(escaped));
        } else {
            out += static_cast<char>(c);
        }
    }
    return out;
}

// Escape a string for use inside Lua double-quoted string literals.
static std::string lua_escape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\v') out += "\\v";
        else if (c < 0x20 || c == 0x7f) {
            char escaped[4] = {
                '\\',
                static_cast<char>('0' + c / 100),
                static_cast<char>('0' + (c / 10) % 10),
                static_cast<char>('0' + c % 10),
            };
            out.append(escaped, sizeof(escaped));
        } else {
            out += static_cast<char>(c);
        }
    }
    return out;
}

// Keep generated line comments single-line and ASCII-only. Backslashes and
// question marks are encoded as well, preventing C++ line splicing and legacy
// trigraphs from turning attacker-controlled text into source syntax.
static std::string code_comment_escape(const std::string& s) {
    static constexpr char HEX[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if (c >= 0x20 && c <= 0x7e && c != '\\' && c != '?') {
            out += static_cast<char>(c);
        } else {
            out += "<0x";
            out += HEX[c >> 4];
            out += HEX[c & 0x0f];
            out += '>';
        }
    }
    return out;
}

// Generate C++ imgui code for a single widget.
static void generate_cpp_widget(std::ostringstream& ss, const Widget& w, int indent) {
    std::string pad(indent * 4, ' ');
    std::string sid = sanitize_id(w.id);
    std::string label = cpp_escape(w.label.empty() ? w.id : w.label);
    std::string comment_label = code_comment_escape(w.label.empty() ? w.id : w.label);
    std::string text = cpp_escape(w.content.empty() ? (w.label.empty() ? w.id : w.label) : w.content);

    switch (w.type) {
        case WidgetType::Text:
            ss << pad << "ImGui::TextUnformatted(\"" << text << "\");\n";
            break;
        case WidgetType::TextColored:
            ss << pad << "ImGui::TextColored(ImVec4(" << w.color[0] << "f, " << w.color[1]
               << "f, " << w.color[2] << "f, " << w.color[3] << "f), \"%s\", \"" << text << "\");\n";
            break;
        case WidgetType::TextDisabled:
            ss << pad << "ImGui::TextDisabled(\"%s\", \"" << text << "\");\n";
            break;
        case WidgetType::TextWrapped:
            ss << pad << "ImGui::TextWrapped(\"%s\", \"" << text << "\");\n";
            break;
        case WidgetType::LabelText:
            ss << pad << "ImGui::LabelText(\"" << label << "\", \"%s\", \"" << text << "\");\n";
            break;
        case WidgetType::BulletText:
            ss << pad << "ImGui::BulletText(\"%s\", \"" << text << "\");\n";
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
            ss << pad << "}\n";
            ss << pad << "ImGui::EndChild();\n";
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
            ss << pad << "    ImGui::SetTooltip(\"%s\", \""
               << cpp_escape(w.tooltip.empty() ? w.content : w.tooltip)
               << "\");\n";
            ss << pad << "}\n";
            break;
        default:
            ss << pad << "// " << widget_type_to_string(w.type) << ": \""
               << comment_label << "\" (not exported)\n";
            break;
    }
}

// Generate Lua imgui code for a single widget.
static void generate_lua_widget(std::ostringstream& ss, const Widget& w, int indent) {
    std::string pad(indent * 4, ' ');
    std::string sid = sanitize_id(w.id);
    std::string label = lua_escape(w.label.empty() ? w.id : w.label);
    std::string comment_label = code_comment_escape(w.label.empty() ? w.id : w.label);

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
            ss << pad << "end\n";
            ss << pad << "imgui.EndChild()\n";
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
            ss << pad << "-- " << widget_type_to_string(w.type) << ": \""
               << comment_label << "\" (not exported)\n";
            break;
    }
}

// ─── Command processing ─────────────────────────────────────────────────────

static int parse_window_flags(const json& cmd) {
    if (!cmd.contains("flags"))
        return ImGuiWindowFlags_None;

    static constexpr int supported_flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_HorizontalScrollbar;

    const json& flags = cmd["flags"];
    if (flags.is_number_integer()) {
        const int bitmask = flags.get<int>();
        if (bitmask < 0 || (bitmask & ~supported_flags) != 0)
            throw std::invalid_argument("flags integer contains unsupported window flag bits");
        return bitmask;
    }
    if (!flags.is_array())
        throw std::invalid_argument("flags must be an integer bitmask or an array of names");

    static const std::map<std::string, ImGuiWindowFlags> names = {
        {"no_title_bar", ImGuiWindowFlags_NoTitleBar},
        {"no_resize", ImGuiWindowFlags_NoResize},
        {"no_move", ImGuiWindowFlags_NoMove},
        {"no_collapse", ImGuiWindowFlags_NoCollapse},
        {"always_auto_resize", ImGuiWindowFlags_AlwaysAutoResize},
        {"menubar", ImGuiWindowFlags_MenuBar},
        {"horizontal_scrollbar", ImGuiWindowFlags_HorizontalScrollbar},
    };

    int bitmask = ImGuiWindowFlags_None;
    for (const auto& flag : flags) {
        if (!flag.is_string())
            throw std::invalid_argument("window flag names must be strings");
        auto it = names.find(flag.get<std::string>());
        if (it == names.end())
            throw std::invalid_argument("unknown window flag: " + flag.dump());
        bitmask |= it->second;
    }
    return bitmask;
}

std::string imgui_window_name(const WindowState& window) {
    return window.title + "###" + window.id;
}

static std::string default_screenshot_path() {
    try {
        return (std::filesystem::temp_directory_path() / "imgui_screenshot.bmp").string();
    } catch (const std::filesystem::filesystem_error&) {
        return "imgui_screenshot.bmp";
    }
}

static void queue_screenshot(const json& command, const std::string& path,
                             bool annotated, json metadata = json::object()) {
    g_screenshot = ScreenshotRequest{};
    g_screenshot.pending = true;
    g_screenshot.path = path;
    g_screenshot.annotated = annotated;
    g_screenshot.command = command.value("cmd", "screenshot");
    g_screenshot.metadata = std::move(metadata);
    g_screenshot.has_request_id = command.contains("request_id");
    if (g_screenshot.has_request_id)
        g_screenshot.request_id = command["request_id"];
}

static size_t utf8_prefix_size(const std::string& text, size_t maximum) {
    size_t offset = 0;
    while (offset < text.size() && offset < maximum) {
        const unsigned char lead = static_cast<unsigned char>(text[offset]);
        size_t length = 1;
        if (lead >= 0xc2u && lead <= 0xdfu) length = 2;
        else if (lead >= 0xe0u && lead <= 0xefu) length = 3;
        else if (lead >= 0xf0u && lead <= 0xf4u) length = 4;
        if (offset + length > text.size() || offset + length > maximum)
            break;
        bool valid = true;
        for (size_t index = 1; index < length; ++index) {
            const unsigned char continuation = static_cast<unsigned char>(text[offset + index]);
            if ((continuation & 0xc0u) != 0x80u) {
                valid = false;
                break;
            }
        }
        offset += valid ? length : 1u;
    }
    return offset;
}

static constexpr size_t MAX_TEXTURE_PIXELS = 16u * 1024u * 1024u;

static uint16_t read_le16(const uint8_t* bytes) {
    return static_cast<uint16_t>(bytes[0]) |
           static_cast<uint16_t>(static_cast<uint16_t>(bytes[1]) << 8u);
}

static uint32_t read_le32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8u) |
           (static_cast<uint32_t>(bytes[2]) << 16u) |
           (static_cast<uint32_t>(bytes[3]) << 24u);
}

static int64_t signed_le32(const uint8_t* bytes) {
    const uint32_t value = read_le32(bytes);
    return value <= 0x7fffffffu
        ? static_cast<int64_t>(value)
        : static_cast<int64_t>(value) - 0x100000000ll;
}

static bool upload_rgba_texture(LoadedTexture& texture, int width, int height,
                                const std::vector<uint8_t>& rgba) {
    if (width <= 0 || height <= 0 ||
        rgba.size() != static_cast<size_t>(width) * static_cast<size_t>(height) * 4u)
        return false;

    glGenTextures(1, &texture.tex_id);
    if (texture.tex_id == 0)
        return false;
    glBindTexture(GL_TEXTURE_2D, texture.tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    texture.width = width;
    texture.height = height;
    texture.valid = true;
    return true;
}

static bool load_bmp_texture(std::ifstream& file, LoadedTexture& texture) {
    uint8_t header[54] = {};
    file.clear();
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    if (file.gcount() != static_cast<std::streamsize>(sizeof(header)) ||
        header[0] != 'B' || header[1] != 'M')
        return false;

    const uint32_t data_offset = read_le32(&header[10]);
    const uint32_t dib_size = read_le32(&header[14]);
    const int64_t width64 = signed_le32(&header[18]);
    const int64_t signed_height = signed_le32(&header[22]);
    const uint16_t planes = read_le16(&header[26]);
    const uint16_t bits_per_pixel = read_le16(&header[28]);
    const uint32_t compression = read_le32(&header[30]);
    if (dib_size < 40u || width64 <= 0 || signed_height == 0 ||
        planes != 1u || compression != 0u ||
        (bits_per_pixel != 24u && bits_per_pixel != 32u))
        return false;

    const uint64_t height64 = signed_height < 0
        ? static_cast<uint64_t>(-signed_height)
        : static_cast<uint64_t>(signed_height);
    if (width64 > std::numeric_limits<int>::max() ||
        height64 > static_cast<uint64_t>(std::numeric_limits<int>::max()))
        return false;
    const size_t width = static_cast<size_t>(width64);
    const size_t height = static_cast<size_t>(height64);
    if (width > MAX_TEXTURE_PIXELS / height)
        return false;

    const size_t channels = bits_per_pixel / 8u;
    if (width > (std::numeric_limits<size_t>::max() - 3u) / channels)
        return false;
    const size_t row_size = ((width * channels + 3u) / 4u) * 4u;
    if (height > std::numeric_limits<size_t>::max() / row_size)
        return false;
    const size_t source_size = row_size * height;

    file.clear();
    file.seekg(0, std::ios::end);
    const std::streamoff file_size = file.tellg();
    if (file_size < 0 || static_cast<uint64_t>(data_offset) + source_size >
            static_cast<uint64_t>(file_size) ||
        source_size > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
        return false;

    std::vector<uint8_t> source(source_size);
    file.seekg(static_cast<std::streamoff>(data_offset), std::ios::beg);
    file.read(reinterpret_cast<char*>(source.data()),
              static_cast<std::streamsize>(source.size()));
    if (file.gcount() != static_cast<std::streamsize>(source.size()))
        return false;

    std::vector<uint8_t> rgba(width * height * 4u);
    for (size_t source_y = 0; source_y < height; ++source_y) {
        const size_t destination_y = signed_height > 0 ? height - 1u - source_y : source_y;
        for (size_t x = 0; x < width; ++x) {
            const size_t source_index = source_y * row_size + x * channels;
            const size_t destination_index = (destination_y * width + x) * 4u;
            rgba[destination_index] = source[source_index + 2u];
            rgba[destination_index + 1u] = source[source_index + 1u];
            rgba[destination_index + 2u] = source[source_index];
            rgba[destination_index + 3u] = channels == 4u ? source[source_index + 3u] : 255u;
        }
    }
    return upload_rgba_texture(texture, static_cast<int>(width),
                               static_cast<int>(height), rgba);
}

static bool read_ppm_token(std::ifstream& file, std::string& token) {
    token.clear();
    char character = 0;
    while (file.get(character)) {
        if (character == '#') {
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(character))) {
            token.push_back(character);
            break;
        }
    }
    while (file.get(character)) {
        if (std::isspace(static_cast<unsigned char>(character)))
            break;
        if (token.size() >= 32u)
            return false;
        token.push_back(character);
    }
    return !token.empty();
}

static bool parse_decimal(const std::string& token, int& value) {
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} && result.ptr == token.data() + token.size();
}

static bool load_ppm_texture(std::ifstream& file, LoadedTexture& texture) {
    file.clear();
    file.seekg(0, std::ios::beg);
    std::string magic, token;
    if (!read_ppm_token(file, magic) || (magic != "P6" && magic != "P3"))
        return false;
    int width = 0, height = 0, maximum = 0;
    if (!read_ppm_token(file, token) || !parse_decimal(token, width) ||
        !read_ppm_token(file, token) || !parse_decimal(token, height) ||
        !read_ppm_token(file, token) || !parse_decimal(token, maximum) ||
        width <= 0 || height <= 0 || maximum <= 0 || maximum > 255)
        return false;

    const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (pixel_count > MAX_TEXTURE_PIXELS ||
        static_cast<size_t>(width) > MAX_TEXTURE_PIXELS / static_cast<size_t>(height))
        return false;
    std::vector<uint8_t> rgb(pixel_count * 3u);
    if (magic == "P6") {
        file.read(reinterpret_cast<char*>(rgb.data()),
                  static_cast<std::streamsize>(rgb.size()));
        if (file.gcount() != static_cast<std::streamsize>(rgb.size()))
            return false;
        if (maximum != 255) {
            for (uint8_t& component : rgb)
                component = static_cast<uint8_t>(static_cast<int>(component) * 255 / maximum);
        }
    } else {
        for (uint8_t& component : rgb) {
            int value = 0;
            if (!read_ppm_token(file, token) || !parse_decimal(token, value) ||
                value < 0 || value > maximum)
                return false;
            component = static_cast<uint8_t>(value * 255 / maximum);
        }
    }

    std::vector<uint8_t> rgba(pixel_count * 4u);
    for (size_t pixel = 0; pixel < pixel_count; ++pixel) {
        rgba[pixel * 4u] = rgb[pixel * 3u];
        rgba[pixel * 4u + 1u] = rgb[pixel * 3u + 1u];
        rgba[pixel * 4u + 2u] = rgb[pixel * 3u + 2u];
        rgba[pixel * 4u + 3u] = 255u;
    }
    return upload_rgba_texture(texture, width, height, rgba);
}

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
        win.flags = parse_window_flags(cmd);
        win.has_menubar = cmd.value("has_menubar", false);
        win.open = cmd.value("open", true);

        std::lock_guard<std::mutex> lock(g_mutex);
        auto candidate_windows = g_windows;
        auto candidate_order = g_window_order;
        if (candidate_windows.find(id) == candidate_windows.end())
            candidate_order.push_back(id);
        candidate_windows[id] = std::move(win);
        validate_state_limits(candidate_windows, candidate_order);
        g_windows.swap(candidate_windows);
        g_window_order.swap(candidate_order);
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
        Widget w = parse_widget_json(cmd);
        auto candidate_windows = g_windows;
        auto candidate_order = g_window_order;
        auto& candidate_window = candidate_windows.at(win_id);
        if (candidate_window.widgets.find(w.id) == candidate_window.widgets.end())
            candidate_window.widget_order.push_back(w.id);
        candidate_window.widgets[w.id] = std::move(w);
        validate_state_limits(candidate_windows, candidate_order);
        push_undo_state();
        g_windows.swap(candidate_windows);
        g_window_order.swap(candidate_order);
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id},
                   {"id", cmd.value("id", "")}});

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
        auto candidate_windows = g_windows;
        auto candidate_order = g_window_order;
        Widget* updated = find_widget_in_window(candidate_windows.at(win_id), wid);
        if (!updated)
            throw std::logic_error("candidate widget lookup failed");
        if (cmd.contains("widget_type") && cmd["widget_type"].is_string())
            updated->type = parse_widget_type(cmd["widget_type"].get<std::string>());
        apply_widget_fields(*updated, cmd);
        validate_widget_format(*updated);
        validate_state_limits(candidate_windows, candidate_order);
        push_undo_state();
        g_windows.swap(candidate_windows);
        g_window_order.swap(candidate_order);
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
        std::vector<std::pair<int, ImVec4>> overrides;
        if (cmd.contains("colors")) {
            if (!cmd["colors"].is_object())
                throw std::invalid_argument("colors must be an object");
            for (const auto& [name, value] : cmd["colors"].items()) {
                const int index = parse_style_color_index(json(name));
                if (index < 0 || !value.is_array() || value.size() != 4 ||
                    !std::all_of(value.begin(), value.end(),
                        [](const json& component) { return component.is_number(); })) {
                    throw std::invalid_argument("invalid custom style color: " + name);
                }
                overrides.emplace_back(index, ImVec4(
                    value[0].get<float>(), value[1].get<float>(),
                    value[2].get<float>(), value[3].get<float>()));
            }
        }
        if (theme == "light") ImGui::StyleColorsLight();
        else if (theme == "classic") ImGui::StyleColorsClassic();
        else ImGui::StyleColorsDark();
        for (const auto& [index, color] : overrides)
            ImGui::GetStyle().Colors[index] = color;
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
        ++g_style_color_depth;
        emit_json({{"type", "ack"}, {"cmd", type}, {"col_idx", idx}});

    } else if (type == "pop_style_color") {
        int count = cmd.value("count", 1);
        if (count < 0 || count > g_style_color_depth) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "style color pop exceeds current stack depth"}});
            return;
        }
        ImGui::PopStyleColor(count);
        g_style_color_depth -= count;
        emit_json({{"type", "ack"}, {"cmd", type}, {"count", count}});

    } else if (type == "push_style_var") {
        int idx = -1;
        if (cmd.contains("var_idx")) idx = parse_style_var_index(cmd["var_idx"]);
        if (idx < 0) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "unknown style var index"}});
            return;
        }
        const bool has_vec2 = cmd.contains("value") && cmd["value"].is_array() &&
                              cmd["value"].size() == 2 &&
                              cmd["value"][0].is_number() && cmd["value"][1].is_number();
        if (style_var_is_vec2(idx) != has_vec2) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", style_var_is_vec2(idx)
                           ? "style variable requires a two-number array"
                           : "style variable requires a number"}});
            return;
        }
        if (has_vec2) {
            float x = cmd["value"][0].get<float>();
            float y = cmd["value"][1].get<float>();
            ImGui::PushStyleVar(idx, ImVec2(x, y));
        } else {
            if (!cmd.contains("value") || !cmd["value"].is_number()) {
                emit_json({{"type", "error"}, {"cmd", type},
                           {"message", "style variable requires a numeric value"}});
                return;
            }
            float v = cmd["value"].get<float>();
            ImGui::PushStyleVar(idx, v);
        }
        ++g_style_var_depth;
        emit_json({{"type", "ack"}, {"cmd", type}, {"var_idx", idx}});

    } else if (type == "pop_style_var") {
        int count = cmd.value("count", 1);
        if (count < 0 || count > g_style_var_depth) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "style variable pop exceeds current stack depth"}});
            return;
        }
        ImGui::PopStyleVar(count);
        g_style_var_depth -= count;
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
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "window not found: " + id}});
            return;
        }
        // Named-window ImGui functions key off the full visible###stable ID.
        std::string name = imgui_window_name(it->second);

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
            it->second.next_bg_alpha = std::clamp(alpha, 0.0f, 1.0f);
        } else if (action == "set_scroll") {
            it->second.next_scroll[0] = vx;
            it->second.next_scroll[1] = vy;
        } else if (action == "set_scroll_x") {
            it->second.next_scroll[0] = vx;
        } else if (action == "set_scroll_y") {
            it->second.next_scroll[1] = vx;
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
        auto candidate_windows = g_windows;
        auto candidate_order = g_window_order;
        auto& win = candidate_windows.at(win_id);
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
        const std::string mode = cmd.value("mode", "append");
        if (mode != "append" && mode != "replace") {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "draw mode must be 'append' or 'replace'"}});
            return;
        }
        if (mode == "replace") w->draw_commands.clear();
        if (cmd.contains("commands") && cmd["commands"].is_array()) {
            if (cmd["commands"].size() > MAX_DRAW_COMMANDS - w->draw_commands.size())
                throw std::length_error("draw command count exceeds the limit of 1024");
            for (const auto& c : cmd["commands"])
                w->draw_commands.push_back(parse_draw_command(c));
        }
        validate_state_limits(candidate_windows, candidate_order);
        const size_t command_count = w->draw_commands.size();
        g_windows.swap(candidate_windows);
        g_window_order.swap(candidate_order);
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id},
                   {"id", wid}, {"mode", mode},
                   {"command_count", command_count}});

    } else if (type == "open_popup") {
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it != g_windows.end()) {
            auto candidate_windows = g_windows;
            auto candidate_order = g_window_order;
            auto& candidate_window = candidate_windows.at(win_id);
            Widget* w = find_widget_in_window(candidate_window, wid);
            if (!w) {
                Widget nw;
                nw.id = wid;
                nw.type = WidgetType::Popup;
                nw.label = wid;
                candidate_window.widget_order.push_back(wid);
                candidate_window.widgets[wid] = nw;
                w = &candidate_window.widgets[wid];
            }
            w->popup_open = true;
            validate_state_limits(candidate_windows, candidate_order);
            g_windows.swap(candidate_windows);
            g_window_order.swap(candidate_order);
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
        if (g_screenshot.pending) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "a screenshot is already pending"}});
            return;
        }
        std::string path = cmd.value("path", default_screenshot_path());
        queue_screenshot(cmd, path, false);
        emit_json({{"type", "ack"}, {"cmd", type}, {"path", path}});

    } else if (type == "screenshot_widget") {
        if (g_screenshot.pending) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "a screenshot is already pending"}});
            return;
        }
        std::string win_id = cmd.value("window", "");
        std::string wid = cmd.value("widget", "");
        std::string path = cmd.value("path", default_screenshot_path());
        queue_screenshot(cmd, path, false);
        g_screenshot.target_window = std::move(win_id);
        g_screenshot.target_widget = std::move(wid);
        emit_json({{"type", "ack"}, {"cmd", type}, {"path", path}});

    } else if (type == "screenshot_annotated") {
        if (g_screenshot.pending) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "a screenshot is already pending"}});
            return;
        }
        std::string path = cmd.value("path", default_screenshot_path());
        queue_screenshot(cmd, path, true);
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
        ImGui::SetWindowPos(imgui_window_name(it->second).c_str(), ImVec2(x, y));
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
        ImGui::SetWindowSize(imgui_window_name(it->second).c_str(), ImVec2(w, h));
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
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "window not found: " + id}});
            return;
        }
        ImGui::SetWindowFocus(imgui_window_name(it->second).c_str());
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
        const float grid = static_cast<float>(grid_size);
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_windows.find(win_id);
        if (it == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        for (auto& [gid, w] : it->second.widgets) {
            w.cursor_offset[0] = std::round(w.cursor_offset[0] / grid) * grid;
            w.cursor_offset[1] = std::round(w.cursor_offset[1] / grid) * grid;
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
            if (w->type != WidgetType::InputText &&
                w->type != WidgetType::InputTextMultiline &&
                w->type != WidgetType::InputTextWithHint) {
                emit_json({{"type", "error"}, {"cmd", type},
                           {"message", "target widget does not accept text input"}});
                return;
            }
            w->request_focus = true;
            std::string combined(w->text_buf);
            combined += text;
            combined.resize(utf8_prefix_size(combined, sizeof(w->text_buf) - 1u));
            std::memcpy(w->text_buf, combined.data(), combined.size());
            w->text_buf[combined.size()] = '\0';
            emit_event(win_id, wid, "changed", {{"text", combined}});
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
        float center_x = 0.0f, center_y = 0.0f;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_windows.find(win_id);
            if (it == g_windows.end()) {
                emit_json({{"type", "error"}, {"cmd", type},
                           {"message", "window not found: " + win_id}});
                return;
            }
            center_x = it->second.x + it->second.w * 0.5f;
            center_y = it->second.y + it->second.h * 0.5f;
        }
        {
            std::lock_guard<std::mutex> lock(g_input_mutex);
            g_input_queue.push_back({{"type", "mouse_move"}, {"x", center_x}, {"y", center_y}});
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
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            const auto window = g_windows.find(win_id);
            if (window == g_windows.end() ||
                find_widget_in_window(window->second, wid) == nullptr)
                throw std::invalid_argument("animation target does not exist");
        }
        if (!is_animation_property(prop))
            throw std::invalid_argument("unsupported animation property: " + prop);
        if (!is_ease_type(ease_str))
            throw std::invalid_argument("unsupported animation easing: " + ease_str);
        if (!std::isfinite(duration) || duration <= 0.0f)
            throw std::invalid_argument("animation duration must be positive");
        Animation anim;
        anim.window_id = win_id;
        anim.widget_id = wid;
        anim.property = prop;
        anim.from = from;
        anim.to = to;
        anim.duration = duration;
        anim.ease = parse_ease_type(ease_str);
        anim.loop = loop;
        anim.active = true;
        anim.elapsed = 0.0f;
        {
            std::lock_guard<std::mutex> lock(g_anim_mutex);
            g_animations.erase(
                std::remove_if(g_animations.begin(), g_animations.end(),
                    [&](const Animation& existing) {
                        return existing.window_id == win_id &&
                               existing.widget_id == wid &&
                               existing.property == prop;
                    }),
                g_animations.end());
            if (g_animations.size() >= 1024)
                throw std::length_error("animation count exceeds the limit of 1024");
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
        if (!std::isfinite(scale) || scale < 0.25f || scale > 4.0f) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "scale must be between 0.25 and 4.0"}});
            return;
        }
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scale / g_dpi_scale);
        g_dpi_scale = scale;
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
        auto candidate_windows = g_windows;
        auto candidate_order = g_window_order;
        Widget* updated = find_widget_in_window(candidate_windows.at(win_id), wid);
        if (!updated)
            throw std::logic_error("candidate widget lookup failed");
        updated->visible_condition = condition;
        validate_state_limits(candidate_windows, candidate_order);
        g_windows.swap(candidate_windows);
        g_window_order.swap(candidate_order);
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
        json current = serialize_state();
        const size_t current_bytes = serialized_json_bytes(current);
        g_redo_stack.push_back(std::move(current));
        g_redo_bytes += current_bytes;
        json prev = g_undo_stack.back();
        const size_t previous_bytes = serialized_json_bytes(g_undo_stack.back());
        g_undo_stack.pop_back();
        g_undo_bytes = previous_bytes > g_undo_bytes
            ? 0 : g_undo_bytes - previous_bytes;
        restore_state(prev);
        trim_history_storage();
        emit_json({{"type", "ack"}, {"cmd", type}, {"undo_depth", (int)g_undo_stack.size()}, {"redo_depth", (int)g_redo_stack.size()}});

    } else if (type == "redo") {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_redo_stack.empty()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "redo stack is empty"}});
            return;
        }
        json current = serialize_state();
        const size_t current_bytes = serialized_json_bytes(current);
        g_undo_stack.push_back(std::move(current));
        g_undo_bytes += current_bytes;
        json next = g_redo_stack.back();
        const size_t next_bytes = serialized_json_bytes(g_redo_stack.back());
        g_redo_stack.pop_back();
        g_redo_bytes = next_bytes > g_redo_bytes
            ? 0 : g_redo_bytes - next_bytes;
        restore_state(next);
        trim_history_storage();
        emit_json({{"type", "ack"}, {"cmd", type}, {"undo_depth", (int)g_undo_stack.size()}, {"redo_depth", (int)g_redo_stack.size()}});

    } else if (type == "save_snapshot") {
        std::string name = cmd.value("name", "snapshot_" + std::to_string(g_snapshots.size()));
        if (name.size() > MAX_SNAPSHOT_NAME_BYTES)
            throw std::length_error("snapshot name exceeds the 256 byte limit");
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_snapshots.size() >= MAX_SNAPSHOTS)
            throw std::length_error("snapshot count exceeds the limit of 64");
        Snapshot snap;
        snap.name = name;
        snap.frame = g_frame_count;
        snap.data = serialize_state();
        const size_t snapshot_bytes = name.size() + serialized_json_bytes(snap.data);
        if (snapshot_bytes > MAX_SNAPSHOT_BYTES - g_snapshot_bytes)
            throw std::length_error("snapshot storage exceeds the 64 MiB limit");
        g_snapshots.push_back(std::move(snap));
        g_snapshot_bytes += snapshot_bytes;
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
            if (it->name == name) {
                const size_t snapshot_bytes =
                    it->name.size() + serialized_json_bytes(it->data);
                g_snapshot_bytes = snapshot_bytes > g_snapshot_bytes
                    ? 0 : g_snapshot_bytes - snapshot_bytes;
                g_snapshots.erase(it);
                found = true;
                break;
            }
        }
        if (!found) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "snapshot not found: " + name}});
            return;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"name", name}, {"snapshot_count", (int)g_snapshots.size()}});

    } else if (type == "export_cpp") {
        std::string path = cmd.value("path", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        validate_state_limits(g_windows, g_window_order);
        std::ostringstream ss;
        ss << "void render_ui() {\n";
        for (auto& wid : g_window_order) {
            auto wit = g_windows.find(wid);
            if (wit == g_windows.end()) continue;
            auto& win = wit->second;
            ss << "    // Window: " << code_comment_escape(win.id) << "\n";
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
            write_export_file(path, code);
            resp["path"] = path;
        }
        emit_json(resp);

    } else if (type == "export_lua") {
        std::string path = cmd.value("path", "");
        std::lock_guard<std::mutex> lock(g_mutex);
        validate_state_limits(g_windows, g_window_order);
        std::ostringstream ss;
        ss << "function render_ui()\n";
        for (auto& wid : g_window_order) {
            auto wit = g_windows.find(wid);
            if (wit == g_windows.end()) continue;
            auto& win = wit->second;
            ss << "    -- Window: " << code_comment_escape(win.id) << "\n";
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
            write_export_file(path, code);
            resp["path"] = path;
        }
        emit_json(resp);

    } else if (type == "export_json") {
        std::string path = cmd.value("path", "");
        std::scoped_lock lock(g_mutex, g_anim_mutex);
        validate_state_limits(g_windows, g_window_order);
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
                widgets.push_back(serialize_widget_full(wit2->second));
            }
            wj["widgets"] = widgets;
            wins.push_back(wj);
        }
        layout["windows"] = wins;
        json fonts = json::array();
        for (const auto& [id, font] : g_fonts) {
            fonts.push_back({{"id", id}, {"path", font.path},
                             {"size_pixels", font.size_pixels},
                             {"glyph_ranges", font.glyph_ranges},
                             {"merge_into", font.merge_into}});
        }
        layout["fonts"] = std::move(fonts);
        layout["active_font"] = "default";
        for (const auto& [id, font] : g_fonts) {
            if (font.merge_into.empty() && ImGui::GetIO().FontDefault == font.font) {
                layout["active_font"] = id;
                break;
            }
        }
        json animations = json::array();
        for (const auto& animation : g_animations) {
            animations.push_back({
                {"window", animation.window_id}, {"widget", animation.widget_id},
                {"property", animation.property}, {"from", animation.from},
                {"to", animation.to}, {"duration", animation.duration},
                {"elapsed", animation.elapsed}, {"ease", ease_type_name(animation.ease)},
                {"loop", animation.loop}, {"active", animation.active}});
        }
        layout["animations"] = std::move(animations);
        json resp;
        resp["type"] = "export";
        resp["format"] = "json";
        resp["json"] = layout;
        if (!path.empty()) {
            write_export_file(path, layout.dump(2));
            resp["path"] = path;
        }
        emit_json(resp);

    } else if (type == "import_json") {
        json layout;
        if (cmd.contains("json")) {
            layout = cmd["json"];
        } else if (cmd.contains("path")) {
            std::string path = cmd["path"].get<std::string>();
            const std::string contents = read_import_file(path);
            try {
                layout = parse_json_with_depth_limit(contents);
            } catch (const std::exception& e) {
                emit_json({{"type", "error"}, {"cmd", type}, {"message", std::string("JSON parse error: ") + e.what()}});
                return;
            }
        } else {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "missing 'json' or 'path' field"}});
            return;
        }

        if (!layout.is_object() || !layout.contains("windows") ||
            !layout["windows"].is_array()) {
            throw std::invalid_argument("layout must contain a windows array");
        }
        if (layout["windows"].size() > MAX_LAYOUT_WINDOWS)
            throw std::length_error("window count exceeds the limit of 256");

        std::map<std::string, WindowState> imported_windows;
        std::vector<std::string> imported_order;
        size_t imported_widget_count = 0;
        for (const auto& wj : layout["windows"]) {
            if (!wj.is_object())
                throw std::invalid_argument("every imported window must be an object");
            WindowState win;
            win.id = wj.value("id", "");
            if (win.id.empty())
                throw std::invalid_argument("imported window id cannot be empty");
            if (imported_windows.count(win.id) != 0)
                throw std::invalid_argument("duplicate imported window id: " + win.id);
            win.title = wj.value("title", win.id);
            win.x = wj.value("x", 100.0f);
            win.y = wj.value("y", 100.0f);
            win.w = wj.value("w", 400.0f);
            win.h = wj.value("h", 300.0f);
            win.open = wj.value("open", true);
            win.collapsed = wj.value("collapsed", false);
            win.flags = parse_window_flags(wj);

            if (wj.contains("widgets")) {
                if (!wj["widgets"].is_array())
                    throw std::invalid_argument("imported widgets must be an array");
                for (const auto& widget_j : wj["widgets"]) {
                    Widget widget = deserialize_widget_full_at_depth(
                        widget_j, 1, imported_widget_count);
                    if (widget.id.empty())
                        throw std::invalid_argument("imported widget id cannot be empty");
                    if (win.widgets.count(widget.id) != 0)
                        throw std::invalid_argument("duplicate imported widget id: " + widget.id);
                    win.widget_order.push_back(widget.id);
                    win.widgets.emplace(widget.id, std::move(widget));
                }
            }
            imported_order.push_back(win.id);
            imported_windows.emplace(win.id, std::move(win));
        }
        validate_state_limits(imported_windows, imported_order);

        struct ImportedFontSpec {
            std::string id;
            std::string path;
            float size_pixels;
            std::string glyph_ranges;
            std::string merge_into;
        };
        std::vector<ImportedFontSpec> imported_fonts;
        if (layout.contains("fonts")) {
            if (!layout["fonts"].is_array())
                throw std::invalid_argument("imported fonts must be an array");
            if (layout["fonts"].size() > 64)
                throw std::length_error("font count exceeds the limit of 64");
            std::map<std::string, bool> imported_font_ids;
            for (const auto& font_j : layout["fonts"]) {
                if (!font_j.is_object())
                    throw std::invalid_argument("every imported font must be an object");
                ImportedFontSpec font{
                    font_j.value("id", ""), font_j.value("path", ""),
                    font_j.value("size_pixels", 16.0f),
                    font_j.value("glyph_ranges", "unicode"),
                    font_j.value("merge_into", "")};
                if (font.id.empty() || font.id.size() > 128)
                    throw std::invalid_argument("font id must contain 1-128 characters");
                if (font.id == "default")
                    throw std::invalid_argument("font id 'default' is reserved");
                if (!imported_font_ids.emplace(font.id, true).second)
                    throw std::invalid_argument("duplicate imported font id: " + font.id);
                if (font.size_pixels < 8.0f || font.size_pixels > 96.0f)
                    throw std::invalid_argument("font size_pixels must be between 8 and 96");
                font_glyph_ranges(font.glyph_ranges);
                std::error_code path_error;
                if (!std::filesystem::is_regular_file(font.path, path_error) || path_error)
                    throw std::invalid_argument("font file does not exist: " + font.path);
                const auto existing = g_fonts.find(font.id);
                if (existing != g_fonts.end()) {
                    const LoadedFont& loaded = existing->second;
                    if (loaded.path != font.path || loaded.size_pixels != font.size_pixels ||
                        loaded.glyph_ranges != font.glyph_ranges ||
                        loaded.merge_into != font.merge_into)
                        throw std::invalid_argument(
                            "loaded font conflicts with imported font: " + font.id);
                }
                imported_fonts.push_back(std::move(font));
            }
            for (const auto& font : imported_fonts) {
                if (!font.merge_into.empty() && g_fonts.count(font.merge_into) == 0 &&
                    imported_font_ids.count(font.merge_into) == 0)
                    throw std::invalid_argument(
                        "merge target font is not loaded: " + font.merge_into);
                if (!font.merge_into.empty()) {
                    const auto loaded_target = g_fonts.find(font.merge_into);
                    if (loaded_target != g_fonts.end() &&
                        !loaded_target->second.merge_into.empty())
                        throw std::invalid_argument(
                            "merge target must be a base font: " + font.merge_into);
                    const auto imported_target = std::find_if(
                        imported_fonts.begin(), imported_fonts.end(),
                        [&](const ImportedFontSpec& candidate) {
                            return candidate.id == font.merge_into;
                        });
                    if (imported_target != imported_fonts.end() &&
                        !imported_target->merge_into.empty())
                        throw std::invalid_argument(
                            "merge target must be a base font: " + font.merge_into);
                }
            }
            const size_t new_font_count = static_cast<size_t>(std::count_if(
                imported_fonts.begin(), imported_fonts.end(),
                [](const ImportedFontSpec& font) {
                    return g_fonts.count(font.id) == 0;
                }));
            if (g_fonts.size() + new_font_count > 64)
                throw std::length_error("font count exceeds the limit of 64");
        }

        std::vector<Animation> imported_animations;
        if (layout.contains("animations")) {
            if (!layout["animations"].is_array())
                throw std::invalid_argument("imported animations must be an array");
            if (layout["animations"].size() > 1024)
                throw std::length_error("animation count exceeds the limit of 1024");
            for (const auto& animation_j : layout["animations"]) {
                if (!animation_j.is_object())
                    throw std::invalid_argument("every imported animation must be an object");
                Animation animation;
                animation.window_id = animation_j.value("window", "");
                animation.widget_id = animation_j.value("widget", "");
                animation.property = animation_j.value("property", "value");
                const auto window = imported_windows.find(animation.window_id);
                if (window == imported_windows.end() ||
                    find_widget_in_window(window->second, animation.widget_id) == nullptr)
                    throw std::invalid_argument("imported animation target does not exist");
                if (!is_animation_property(animation.property))
                    throw std::invalid_argument("unsupported imported animation property: " + animation.property);
                animation.from = animation_j.value("from", 0.0f);
                animation.to = animation_j.value("to", 1.0f);
                animation.duration = animation_j.value("duration", 1.0f);
                if (!std::isfinite(animation.from) || !std::isfinite(animation.to) ||
                    !std::isfinite(animation.duration) || animation.duration <= 0.0f)
                    throw std::invalid_argument("imported animation duration must be positive");
                animation.elapsed = std::max(0.0f, animation_j.value("elapsed", 0.0f));
                const std::string ease = animation_j.value("ease", "linear");
                if (!is_ease_type(ease))
                    throw std::invalid_argument("unsupported imported animation easing: " + ease);
                animation.ease = parse_ease_type(ease);
                animation.loop = animation_j.value("loop", false);
                animation.active = animation_j.value("active", true);
                imported_animations.push_back(std::move(animation));
            }
        }

        std::string active_font_id;
        if (layout.contains("active_font")) {
            active_font_id = layout.value("active_font", "default");
            const auto loaded_active = g_fonts.find(active_font_id);
            if (loaded_active != g_fonts.end() && !loaded_active->second.merge_into.empty())
                throw std::invalid_argument(
                    "merged font sources are not independently selectable: " + active_font_id);
            if (active_font_id != "default" && g_fonts.count(active_font_id) == 0 &&
                std::none_of(imported_fonts.begin(), imported_fonts.end(),
                    [&](const ImportedFontSpec& font) {
                        return font.id == active_font_id && font.merge_into.empty();
                    }))
                throw std::invalid_argument(
                    "imported active font is not loaded: " + active_font_id);
        }

        auto load_imported_fonts = [&](bool merged) {
            for (const auto& font : imported_fonts) {
                if ((!font.merge_into.empty() != merged) || g_fonts.count(font.id) != 0)
                    continue;
                load_font_source(font.id, font.path, font.size_pixels,
                                 font.glyph_ranges, font.merge_into);
            }
        };
        load_imported_fonts(false);
        load_imported_fonts(true);

        ImFont* imported_active_font = ImGui::GetIO().FontDefault;
        if (!active_font_id.empty()) {
            imported_active_font = nullptr;
            if (active_font_id != "default")
                imported_active_font = g_fonts.at(active_font_id).font;
        }

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            push_undo_state();
            g_windows.swap(imported_windows);
            g_window_order.swap(imported_order);
        }
        {
            std::lock_guard<std::mutex> lock(g_anim_mutex);
            g_animations.swap(imported_animations);
        }
        ImGui::GetIO().FontDefault = imported_active_font;
        emit_json({{"type", "ack"}, {"cmd", type},
                   {"windows", static_cast<int>(g_window_order.size())},
                   {"fonts", static_cast<int>(imported_fonts.size())},
                   {"animations", static_cast<int>(g_animations.size())}});

    } else if (type == "load_texture") {
        std::string id = cmd.value("id", "");
        std::string path = cmd.value("path", "");
        if (id.empty()) {
            emit_json({{"type", "error"}, {"cmd", type},
                       {"message", "texture id cannot be empty"}});
            return;
        }
        LoadedTexture tex;
        tex.id = id;
        tex.path = path;
        bool loaded = false;
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            char magic[2] = {0, 0};
            file.read(magic, 2);
            if (file.gcount() == 2) {
                if (magic[0] == 'B' && magic[1] == 'M')
                    loaded = load_bmp_texture(file, tex);
                else if (magic[0] == 'P' && (magic[1] == '6' || magic[1] == '3'))
                    loaded = load_ppm_texture(file, tex);
            }
        }
        if (!loaded) {
            constexpr int placeholder_width = 64;
            constexpr int placeholder_height = 64;
            std::vector<uint8_t> pixels(
                static_cast<size_t>(placeholder_width * placeholder_height * 4));
            for (int y = 0; y < placeholder_height; ++y) {
                for (int x = 0; x < placeholder_width; ++x) {
                    const size_t index = static_cast<size_t>(
                        (y * placeholder_width + x) * 4);
                    const bool checker = ((x / 8) + (y / 8)) % 2 == 0;
                    pixels[index] = checker ? 200u : 80u;
                    pixels[index + 1u] = checker ? 80u : 200u;
                    pixels[index + 2u] = checker ? 200u : 80u;
                    pixels[index + 3u] = 255u;
                }
            }
            if (!upload_rgba_texture(tex, placeholder_width, placeholder_height, pixels))
                throw std::runtime_error("failed to create placeholder texture");
        }
        auto existing = g_textures.find(id);
        if (existing != g_textures.end() && existing->second.tex_id != 0)
            glDeleteTextures(1, &existing->second.tex_id);
        g_textures[id] = tex;
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id},
                   {"width", tex.width}, {"height", tex.height},
                   {"placeholder", !loaded}});
    } else if (type == "unload_texture") {
        std::string id = cmd.value("id", "");
        auto it = g_textures.find(id);
        if (it != g_textures.end()) {
            if (it->second.tex_id) glDeleteTextures(1, &it->second.tex_id);
            g_textures.erase(it);
        }
        emit_json({{"type","ack"},{"cmd",type},{"id",id}});
    } else if (type == "load_font") {
        const std::string id = cmd.value("id", "");
        const std::string path = cmd.value("path", "");
        const float size_pixels = cmd.value("size_pixels", 16.0f);
        const std::string glyph_ranges = cmd.value("glyph_ranges", "unicode");
        const std::string merge_into = cmd.value("merge_into", "");
        load_font_source(id, path, size_pixels, glyph_ranges, merge_into);
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id},
                   {"path", path}, {"size_pixels", size_pixels},
                   {"glyph_ranges", glyph_ranges}, {"merge_into", merge_into}});
    } else if (type == "set_font") {
        const std::string id = cmd.value("id", "default");
        if (id == "default") {
            ImGui::GetIO().FontDefault = nullptr;
        } else {
            const auto it = g_fonts.find(id);
            if (it == g_fonts.end()) {
                emit_json({{"type", "error"}, {"cmd", type},
                           {"message", "font is not loaded: " + id}});
                return;
            }
            if (!it->second.merge_into.empty()) {
                emit_json({{"type", "error"}, {"cmd", type},
                           {"message", "merged font sources are not independently selectable: " + id}});
                return;
            }
            ImGui::GetIO().FontDefault = it->second.font;
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", id}});
    } else if (type == "list_fonts") {
        json fonts = json::array({
            {{"id", "default"}, {"path", ""}, {"size_pixels", 0.0f},
             {"active", ImGui::GetIO().FontDefault == nullptr}}
        });
        for (const auto& [id, font] : g_fonts) {
            fonts.push_back({{"id", id}, {"path", font.path},
                             {"size_pixels", font.size_pixels},
                             {"glyph_ranges", font.glyph_ranges},
                             {"merge_into", font.merge_into},
                             {"active", font.merge_into.empty() &&
                                        ImGui::GetIO().FontDefault == font.font}});
        }
        emit_json({{"type", "fonts"}, {"cmd", type}, {"fonts", fonts}});
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

    // ─── Interactive Canvas Commands ────────────────────────────────────────

    } else if (type == "add_hit_region") {
        std::string win_id = cmd.value("window", "");
        std::string region_id = cmd.value("id", "");
        if (region_id.empty()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "hit region id is required"}});
            return;
        }
        HitRegion region;
        region.id = region_id;
        region.window_id = win_id;
        region.draw_layer = cmd.value("draw_layer", "");

        std::string shape_str = cmd.value("shape", "rect");
        if (shape_str == "rect") region.shape = HitShape::Rect;
        else if (shape_str == "circle") region.shape = HitShape::Circle;
        else if (shape_str == "ellipse") region.shape = HitShape::Ellipse;
        else if (shape_str == "polygon") region.shape = HitShape::Polygon;
        else throw std::invalid_argument("unknown hit shape: " + shape_str);

        if (cmd.contains("p1") && cmd["p1"].is_array() && cmd["p1"].size() >= 2) {
            region.p1[0] = cmd["p1"][0].get<float>();
            region.p1[1] = cmd["p1"][1].get<float>();
        }
        if (cmd.contains("p2") && cmd["p2"].is_array() && cmd["p2"].size() >= 2) {
            region.p2[0] = cmd["p2"][0].get<float>();
            region.p2[1] = cmd["p2"][1].get<float>();
        }
        if (cmd.contains("points") && cmd["points"].is_array()) {
            for (const auto& pt : cmd["points"]) {
                if (pt.is_array() && pt.size() >= 2) {
                    region.polygon_points.push_back(pt[0].get<float>());
                    region.polygon_points.push_back(pt[1].get<float>());
                }
            }
        }
        if (cmd.contains("interactions") && cmd["interactions"].is_array()) {
            region.hover_enabled = false;
            region.click_enabled = false;
            region.drag_enabled = false;
            region.double_click_enabled = false;
            region.wheel_enabled = false;
            for (const auto& interaction : cmd["interactions"]) {
                std::string s = interaction.get<std::string>();
                if (s == "hover") region.hover_enabled = true;
                else if (s == "click") region.click_enabled = true;
                else if (s == "drag") region.drag_enabled = true;
                else if (s == "double_click") region.double_click_enabled = true;
                else if (s == "wheel") region.wheel_enabled = true;
            }
        }
        if (cmd.contains("transform") && cmd["transform"].is_object()) {
            const auto& t = cmd["transform"];
            if (t.contains("position") && t["position"].is_array() && t["position"].size() >= 2) {
                region.position[0] = t["position"][0].get<float>();
                region.position[1] = t["position"][1].get<float>();
            }
            region.rotation = t.value("rotation", 0.0f);
            if (t.contains("scale") && t["scale"].is_array() && t["scale"].size() >= 2) {
                region.scale[0] = t["scale"][0].get<float>();
                region.scale[1] = t["scale"][1].get<float>();
            }
        }

        std::lock_guard<std::mutex> lock(g_hit_regions_mutex);
        g_hit_regions[region_id] = std::move(region);
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", region_id}, {"window", win_id}});

    } else if (type == "remove_hit_region") {
        std::string region_id = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_hit_regions_mutex);
        g_hit_regions.erase(region_id);
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", region_id}});

    } else if (type == "update_hit_region") {
        std::string region_id = cmd.value("id", "");
        std::lock_guard<std::mutex> lock(g_hit_regions_mutex);
        auto it = g_hit_regions.find(region_id);
        if (it == g_hit_regions.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "hit region not found: " + region_id}});
            return;
        }
        auto& region = it->second;
        if (cmd.contains("p1") && cmd["p1"].is_array() && cmd["p1"].size() >= 2) {
            region.p1[0] = cmd["p1"][0].get<float>();
            region.p1[1] = cmd["p1"][1].get<float>();
        }
        if (cmd.contains("p2") && cmd["p2"].is_array() && cmd["p2"].size() >= 2) {
            region.p2[0] = cmd["p2"][0].get<float>();
            region.p2[1] = cmd["p2"][1].get<float>();
        }
        if (cmd.contains("points") && cmd["points"].is_array()) {
            region.polygon_points.clear();
            for (const auto& pt : cmd["points"]) {
                if (pt.is_array() && pt.size() >= 2) {
                    region.polygon_points.push_back(pt[0].get<float>());
                    region.polygon_points.push_back(pt[1].get<float>());
                }
            }
        }
        if (cmd.contains("transform") && cmd["transform"].is_object()) {
            const auto& t = cmd["transform"];
            if (t.contains("position") && t["position"].is_array() && t["position"].size() >= 2) {
                region.position[0] = t["position"][0].get<float>();
                region.position[1] = t["position"][1].get<float>();
            }
            if (t.contains("rotation")) region.rotation = t["rotation"].get<float>();
            if (t.contains("scale") && t["scale"].is_array() && t["scale"].size() >= 2) {
                region.scale[0] = t["scale"][0].get<float>();
                region.scale[1] = t["scale"][1].get<float>();
            }
        }
        if (cmd.contains("interactions") && cmd["interactions"].is_array()) {
            region.hover_enabled = false;
            region.click_enabled = false;
            region.drag_enabled = false;
            region.double_click_enabled = false;
            region.wheel_enabled = false;
            for (const auto& interaction : cmd["interactions"]) {
                std::string s = interaction.get<std::string>();
                if (s == "hover") region.hover_enabled = true;
                else if (s == "click") region.click_enabled = true;
                else if (s == "drag") region.drag_enabled = true;
                else if (s == "double_click") region.double_click_enabled = true;
                else if (s == "wheel") region.wheel_enabled = true;
            }
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"id", region_id}});

    } else if (type == "get_canvas_events") {
        int max_events = cmd.value("max_events", 100);
        if (max_events < 1) max_events = 1;
        if (max_events > 256) max_events = 256;
        std::lock_guard<std::mutex> lock(g_canvas_events_mutex);
        json result;
        result["type"] = "canvas_events";
        result["events"] = json::array();
        int count = 0;
        auto it = g_canvas_events.begin();
        while (it != g_canvas_events.end() && count < max_events) {
            result["events"].push_back(*it);
            it = g_canvas_events.erase(it);
            count++;
        }
        result["remaining"] = (int)g_canvas_events.size();
        emit_json(result);

    } else if (type == "update_draw_command") {
        std::string win_id = cmd.value("window", "");
        std::string layer_id = cmd.value("layer", "draw_list");
        int index = cmd.value("index", -1);
        if (index < 0) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "index is required and must be >= 0"}});
            return;
        }
        std::lock_guard<std::mutex> lock(g_mutex);
        auto wit = g_windows.find(win_id);
        if (wit == g_windows.end()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "window not found: " + win_id}});
            return;
        }
        Widget* layer = find_widget_in_window(wit->second, layer_id);
        if (!layer || layer->type != WidgetType::DrawList) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "draw layer not found: " + layer_id}});
            return;
        }
        if (static_cast<size_t>(index) >= layer->draw_commands.size()) {
            emit_json({{"type", "error"}, {"cmd", type}, {"message", "draw command index out of range"}});
            return;
        }
        auto& dc = layer->draw_commands[static_cast<size_t>(index)];
        if (cmd.contains("fields") && cmd["fields"].is_object()) {
            const auto& f = cmd["fields"];
            if (f.contains("p1") && f["p1"].is_array() && f["p1"].size() >= 2) {
                dc.p1[0] = f["p1"][0].get<float>();
                dc.p1[1] = f["p1"][1].get<float>();
            }
            if (f.contains("p2") && f["p2"].is_array() && f["p2"].size() >= 2) {
                dc.p2[0] = f["p2"][0].get<float>();
                dc.p2[1] = f["p2"][1].get<float>();
            }
            if (f.contains("p3") && f["p3"].is_array() && f["p3"].size() >= 2) {
                dc.p3[0] = f["p3"][0].get<float>();
                dc.p3[1] = f["p3"][1].get<float>();
            }
            if (f.contains("p4") && f["p4"].is_array() && f["p4"].size() >= 2) {
                dc.p4[0] = f["p4"][0].get<float>();
                dc.p4[1] = f["p4"][1].get<float>();
            }
            if (f.contains("color") && f["color"].is_array() && f["color"].size() >= 4) {
                for (int i = 0; i < 4; i++) dc.color[i] = f["color"][i].get<float>();
            }
            if (f.contains("thickness")) dc.thickness = f["thickness"].get<float>();
            if (f.contains("filled")) dc.filled = f["filled"].get<bool>();
            if (f.contains("num_segments")) dc.num_segments = f["num_segments"].get<int>();
            if (f.contains("text")) dc.text = f["text"].get<std::string>();
        }
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"layer", layer_id}, {"index", index}});

    } else if (type == "transform_draw_layer") {
        std::string win_id = cmd.value("window", "");
        std::string layer_id = cmd.value("layer", "draw_list");
        std::string key = win_id + "/" + layer_id;
        auto& t = g_draw_layer_transforms[key];
        if (cmd.contains("translate") && cmd["translate"].is_array() && cmd["translate"].size() >= 2) {
            t.translate[0] = cmd["translate"][0].get<float>();
            t.translate[1] = cmd["translate"][1].get<float>();
        }
        if (cmd.contains("rotation")) t.rotation = cmd["rotation"].get<float>();
        if (cmd.contains("scale") && cmd["scale"].is_array() && cmd["scale"].size() >= 2) {
            t.scale[0] = cmd["scale"][0].get<float>();
            t.scale[1] = cmd["scale"][1].get<float>();
        }
        if (cmd.contains("opacity")) t.opacity = cmd["opacity"].get<float>();
        emit_json({{"type", "ack"}, {"cmd", type}, {"window", win_id}, {"layer", layer_id}});

    } else if (type == "bind") {
        Binding binding;
        binding.window_id = cmd.value("window", "");
        binding.source_widget = cmd.value("source_widget", "");
        binding.source_property = cmd.value("source_property", "value");
        binding.target_type = cmd.value("target_type", "draw_layer");
        binding.target_id = cmd.value("target_id", "");
        binding.target_property = cmd.value("target_property", "opacity");
        binding.bind_scale = cmd.value("scale", 1.0f);
        binding.bind_offset = cmd.value("offset", 0.0f);
        binding.id = binding.source_widget + "->" + binding.target_id + "." + binding.target_property;

        // Remove existing binding with same id
        g_bindings.erase(
            std::remove_if(g_bindings.begin(), g_bindings.end(),
                [&](const Binding& b) { return b.id == binding.id; }),
            g_bindings.end());
        g_bindings.push_back(std::move(binding));
        emit_json({{"type", "ack"}, {"cmd", type},
                   {"source", cmd.value("source_widget", "")},
                   {"target", cmd.value("target_id", "")}});

    } else if (type == "unbind") {
        std::string source = cmd.value("source_widget", "");
        std::string target = cmd.value("target_id", "");
        g_bindings.erase(
            std::remove_if(g_bindings.begin(), g_bindings.end(),
                [&](const Binding& b) {
                    return (source.empty() || b.source_widget == source) &&
                           (target.empty() || b.target_id == target);
                }),
            g_bindings.end());
        emit_json({{"type", "ack"}, {"cmd", type}});

    } else if (type == "set_callback") {
        CallbackAction cb;
        cb.window_id = cmd.value("window", "");
        cb.trigger_widget = cmd.value("trigger_widget", "");
        cb.trigger_event = cmd.value("trigger_event", "clicked");
        cb.action_type = cmd.value("action", "");
        if (cmd.contains("params") && cmd["params"].is_object())
            cb.action_params = cmd["params"];
        cb.id = cb.trigger_widget + ":" + cb.trigger_event + ":" + cb.action_type;

        // Remove existing callback with same id
        g_callbacks.erase(
            std::remove_if(g_callbacks.begin(), g_callbacks.end(),
                [&](const CallbackAction& c) { return c.id == cb.id; }),
            g_callbacks.end());
        g_callbacks.push_back(std::move(cb));
        emit_json({{"type", "ack"}, {"cmd", type},
                   {"trigger", cmd.value("trigger_widget", "")},
                   {"action", cmd.value("action", "")}});

    } else if (type == "remove_callback") {
        std::string trigger = cmd.value("trigger_widget", "");
        std::string action = cmd.value("action", "");
        g_callbacks.erase(
            std::remove_if(g_callbacks.begin(), g_callbacks.end(),
                [&](const CallbackAction& c) {
                    return (trigger.empty() || c.trigger_widget == trigger) &&
                           (action.empty() || c.action_type == action);
                }),
            g_callbacks.end());
        emit_json({{"type", "ack"}, {"cmd", type}});

    } else if (type == "quit") {
        g_running = false;
        emit_json({{"type", "ack"}, {"cmd", type}});

    } else {
        emit_json({{"type", "error"}, {"message", "unknown command: " + type}});
    }
}
