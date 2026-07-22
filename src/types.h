#pragma once
#ifndef TYPES_H
#define TYPES_H

#include "imgui.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <queue>
#include <GL/gl.h>
#include "json.hpp"

using json = nlohmann::json;

// ─── Widget type enumeration ────────────────────────────────────────────────

enum class WidgetType {
    // Buttons
    Button, SmallButton, InvisibleButton, ArrowButton,
    // Toggles / flags
    Checkbox, CheckboxFlags, RadioButton,
    // Progress / misc
    ProgressBar, Bullet,
    // Links
    TextLink, TextLinkOpenURL,

    // Text
    Text, TextColored, TextDisabled, TextWrapped, LabelText, BulletText, SeparatorText,

    // Layout
    Separator, SameLine, NewLine, Spacing, Dummy, Indent, Unindent, AlignTextToFramePadding,

    // Input
    InputText, InputTextMultiline, InputTextWithHint,
    InputFloat, InputFloat2, InputFloat3, InputFloat4,
    InputInt, InputInt2, InputInt3, InputInt4, InputDouble,

    // Sliders
    SliderFloat, SliderFloat2, SliderFloat3, SliderFloat4,
    SliderInt, SliderInt2, SliderInt3, SliderInt4,
    SliderAngle, VSliderFloat, VSliderInt,

    // Drag
    DragFloat, DragFloat2, DragFloat3, DragFloat4, DragFloatRange2,
    DragInt, DragInt2, DragInt3, DragInt4, DragIntRange2,

    // Color
    ColorEdit3, ColorEdit4, ColorPicker3, ColorPicker4, ColorButton,

    // Combo / list
    Combo, ListBox,

    // Tree / selectable
    TreeNode, TreeNodeEx, CollapsingHeader, Selectable,

    // Table
    Table,

    // Menu
    MenuBar, MainMenuBar, Menu, MenuItem,

    // Popup
    Popup, PopupModal,

    // Plots
    PlotLines, PlotHistogram,

    // Containers
    ChildWindow, TabBar, TabItem, Group,

    // Value helpers
    ValueBool, ValueInt, ValueFloat,

    // Tooltip
    Tooltip, SetItemTooltip,

    // Draw list
    DrawList,

    // Game UI patterns
    HealthBar, ManaBar, InventoryGrid, DialogueBox, Minimap,
    TooltipCard, CooldownRadial, NotificationToast, SkillBar,
    QuestTracker, CharacterSheet,

    // Image / texture
    Image, ImageButton
};

// ─── Draw commands (for DrawList widget) ────────────────────────────────────

struct DrawCommand {
    int    type         = 0;       // 0..13=legacy primitives, 14=Ellipse,15=EllipseFilled,16=RectGradient,17=Arc
    float  p1[2]       = {0, 0};
    float  p2[2]       = {0, 0};
    float  p3[2]       = {0, 0};
    float  p4[2]       = {0, 0};
    float  color[4]    = {1, 1, 1, 1};
    float  color2[4]   = {1, 1, 1, 1};
    float  color3[4]   = {1, 1, 1, 1};
    float  color4[4]   = {1, 1, 1, 1};
    float  thickness   = 1.0f;
    bool   filled      = false;
    int    num_segments = 0;
    std::string text;
};

// ─── Anchor / Responsive Layout system ──────────────────────────────────────

enum class Anchor { None, TopLeft, TopCenter, TopRight, CenterLeft, Center, CenterRight, BottomLeft, BottomCenter, BottomRight };

// ─── Widget ─────────────────────────────────────────────────────────────────

struct Widget {
    WidgetType type = WidgetType::Text;

    // Strings
    std::string id;
    std::string label;
    std::string content;
    std::string hint;
    std::string tooltip;
    std::string shortcut;
    std::string overlay;
    std::string format;

    // Float values (up to 4 components: float, float2, float3, float4)
    float float_val[4]  = {0, 0, 0, 0};
    float float_min     = 0.0f;
    float float_max     = 1.0f;
    float float_speed   = 1.0f;

    // Int values
    int int_val[4]  = {0, 0, 0, 0};
    int int_min     = 0;
    int int_max     = 100;

    // Double
    double double_val = 0.0;

    // Bool values (bool_val2 for range2 widgets)
    bool bool_val  = false;
    bool bool_val2 = false;

    // Text buffer for InputText
    char text_buf[1024] = {};

    // Color
    float color[4] = {0, 0, 0, 1};

    // Items for Combo / ListBox
    std::vector<std::string> items;
    int selected = -1;

    // Plot values
    std::vector<float> plot_values;

    // Children (nested containers: ChildWindow, TabBar, TabItem, Group, TreeNode, Table, etc.)
    std::vector<Widget> children;

    // Table data
    std::vector<std::string>              headers;
    std::vector<std::vector<std::string>> rows;
    int columns = 0;

    // Flags
    int flags = 0;

    // State
    bool enabled    = true;
    bool tree_open  = false;
    bool popup_open = false;

    // Size
    float size_x = 0.0f;
    float size_y = 0.0f;

    // Actual rendered bounding box (filled during render)
    float rect_min[2] = {0, 0};
    float rect_max[2] = {0, 0};
    int last_rendered_frame = -1;

    // Cursor offset for move_widget (applied via SetCursorPos before rendering)
    float cursor_offset[2] = {0, 0};

    // Anchor for responsive positioning
    Anchor anchor = Anchor::None;
    float anchor_offset[2] = {0, 0}; // offset from anchor point

    // Arrow direction: 0=left, 1=right, 2=up, 3=down
    int dir = 0;

    // Step values for input / drag widgets
    double step      = 0.0;
    double step_fast = 0.0;

    // Draw commands (for DrawList widget)
    std::vector<DrawCommand> draw_commands;

    // Per-frame state (reset each frame before rendering)
    bool clicked  = false;
    bool changed  = false;
    bool hovered  = false;
    bool active   = false;
    bool focused  = false;
    bool request_focus = false;  // request keyboard focus next frame

    // Conditional visibility / layer (multi-window scene system)
    std::string visible_condition; // "always", "never", "widget_id.checked==true", "widget_id.value>0.5"
    int wlayer = 0; // render layer (higher = on top)
};

// ─── Window state ───────────────────────────────────────────────────────────

struct WindowState {
    std::string id;
    std::string title;
    float  x = 100.0f;
    float  y = 100.0f;
    float  w = 400.0f;
    float  h = 300.0f;
    bool   open      = true;
    bool   collapsed = false;
    int    flags     = 0;
    bool   has_menubar = false;
    float  next_bg_alpha = -1.0f;
    float  next_scroll[2] = {-1.0f, -1.0f};
    bool   apply_geometry = true;

    // Window anchor for responsive positioning
    Anchor window_anchor = Anchor::None;
    float window_anchor_offset[2] = {0, 0};

    std::vector<std::string>          widget_order;
    std::map<std::string, Widget>     widgets;
    int layer = 0; // window render layer for scene ordering
};

struct ScreenshotRequest {
    bool pending = false;
    std::string path = "imgui_screenshot.bmp";
    bool annotated = false;
    json request_id;
    bool has_request_id = false;
    std::string command;
    json metadata = json::object();
    std::string target_window;
    std::string target_widget;
};

struct QueuedCommand {
    json command;
    size_t serialized_bytes = 0;
};

// ─── Extern globals ─────────────────────────────────────────────────────────

extern std::mutex                        g_mutex;        // protects g_windows / g_window_order
extern std::mutex                        g_cmd_mutex;     // protects g_commands queue
extern std::queue<QueuedCommand>         g_commands;
extern std::atomic<bool>                 g_running;
extern std::map<std::string, WindowState> g_windows;
extern std::vector<std::string>          g_window_order;
extern std::vector<json>                 g_events;
extern std::mutex                        g_events_mutex;
extern int                               g_frame_count;

extern ScreenshotRequest g_screenshot;
extern std::vector<json>                 g_input_queue;   // queued input events to inject
extern std::mutex                        g_input_mutex;    // protects g_input_queue

// Dear ImGui overlay windows
extern bool  g_show_demo;
extern bool  g_show_metrics;
extern bool  g_show_about;
extern bool  g_show_style_editor;
extern bool  g_show_debug_log;
extern bool  g_show_id_stack_tool;

// Background clear color
extern float g_bg_color[3];

// ─── Animation / Tween system ────────────────────────────────────────────────

enum class EaseType { Linear, EaseIn, EaseOut, EaseInOut, Bounce, Elastic, Back };

struct Animation {
    std::string widget_id;
    std::string window_id;
    std::string property; // "value", "color", "opacity", "pos_x", "pos_y", "size_x", "size_y"
    float from = 0, to = 1;
    float duration = 1.0f; // seconds
    float elapsed = 0.0f;
    EaseType ease = EaseType::Linear;
    bool active = true;
    bool loop = false;
};

extern std::vector<Animation> g_animations;
extern std::mutex g_anim_mutex;

// ─── Undo/Redo & Snapshot system ───────────────────────────────────────────

struct Snapshot {
    std::string name;
    int frame;
    json data; // serialized layout state
};

extern std::vector<Snapshot> g_snapshots;
extern std::vector<json>     g_undo_stack;
extern std::vector<json>     g_redo_stack;
extern int                   g_max_undo; // default 50

// ─── Texture / Asset system ─────────────────────────────────────────────────

struct LoadedTexture {
    std::string id;
    std::string path;
    GLuint tex_id = 0;
    int width = 0, height = 0;
    bool valid = false;
};

extern std::map<std::string, LoadedTexture> g_textures;
extern std::string g_theme_preset; // current theme name

struct LoadedFont {
    std::string id;
    std::string path;
    float size_pixels = 0.0f;
    std::string glyph_ranges = "unicode";
    std::string merge_into;
    ImFont* font = nullptr;
};

extern std::map<std::string, LoadedFont> g_fonts;

// Safe area insets (top, bottom, left, right) in pixels
extern float g_safe_area[4];

// SDL window pointer (set in main, used by commands for resizing)
struct SDL_Window;
extern SDL_Window* g_sdl_window;

// ─── Forward declarations ───────────────────────────────────────────────────

void emit_json(json response);
void emit_event(const std::string& window_id, const std::string& widget_id, const std::string& event_type, const json& data = json{});
Widget* find_widget_in_window(WindowState& win, const std::string& id);
void render_widget(const std::string& win_id, Widget& w);
bool is_safe_widget_format(const Widget& widget);
std::string imgui_window_name(const WindowState& window);

// ─── Scene system ──────────────────────────────────────────────────────────

struct Scene {
    std::string name;
    std::vector<std::string> window_ids; // windows visible in this scene
};

extern std::map<std::string, Scene> g_scenes;
extern std::string g_active_scene;
extern int g_layer_order; // z-order counter


// ─── Interactive Canvas: Hit Regions ────────────────────────────────────────

enum class HitShape { Rect, Circle, Ellipse, Polygon };

struct HitRegion {
    std::string id;
    std::string window_id;
    std::string draw_layer; // optional associated draw_list widget id
    HitShape shape = HitShape::Rect;

    // Shape geometry (screen-space coordinates)
    // Rect: p1=min corner, p2=max corner
    // Circle: p1=center, p2[0]=radius
    // Ellipse: p1=center, p2=[rx, ry]
    // Polygon: polygon_points as x,y pairs
    float p1[2] = {0, 0};
    float p2[2] = {0, 0};
    std::vector<float> polygon_points;

    // Transform (applied to shape geometry)
    float position[2] = {0, 0};
    float rotation = 0.0f; // radians
    float scale[2] = {1, 1};

    // Interaction flags
    bool hover_enabled = true;
    bool click_enabled = true;
    bool drag_enabled = false;
    bool double_click_enabled = false;
    bool wheel_enabled = false;

    // Per-frame state
    bool hovered = false;
    bool dragging = false;

    // Drag tracking
    float drag_start[2] = {0, 0};
    float last_mouse[2] = {0, 0};
};

// ─── Interactive Canvas: Draw Layer Transforms ──────────────────────────────

struct DrawLayerTransform {
    float translate[2] = {0, 0};
    float rotation = 0.0f; // radians
    float scale[2] = {1, 1};
    float opacity = 1.0f;
};

// ─── Interactive Canvas: Bindings ──────────────────────────────────────────

struct Binding {
    std::string id;
    std::string window_id;
    std::string source_widget;
    std::string source_property; // "value", "checked", "color_r", etc.
    std::string target_type;     // "draw_layer", "hit_region", "widget"
    std::string target_id;
    std::string target_property; // "opacity", "position_x", "scale_x", etc.
    float bind_scale = 1.0f;
    float bind_offset = 0.0f;
};

// ─── Interactive Canvas: Declarative Callbacks ─────────────────────────────

struct CallbackAction {
    std::string id;
    std::string window_id;
    std::string trigger_widget;
    std::string trigger_event; // "clicked", "changed", etc.
    std::string action_type;   // "toggle_visibility", "set_visibility", "switch_scene", "set_value"
    json action_params = json::object();
};

// ─── Interactive Canvas: Globals ───────────────────────────────────────────

extern std::map<std::string, HitRegion>          g_hit_regions;
extern std::mutex                                g_hit_regions_mutex;
extern std::vector<json>                         g_canvas_events;
extern std::mutex                                g_canvas_events_mutex;
extern std::map<std::string, DrawLayerTransform> g_draw_layer_transforms; // key: "window_id/layer_id"
extern std::vector<Binding>                      g_bindings;
extern std::vector<CallbackAction>               g_callbacks;

// Helper: emit a canvas event
void emit_canvas_event(const std::string& window_id, const std::string& region_id,
                       const std::string& event_type, const json& data = json{});
#endif // TYPES_H
