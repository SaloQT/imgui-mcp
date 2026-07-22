// imgui_mcp_app - Interactive Dear ImGui application controlled via MCP
// Uses SDL2 + OpenGL3 backend, reads JSON commands from stdin, outputs JSON to stdout.
// Vendored Dear ImGui v1.92.8.
//
// This translation unit owns: global state definitions, output/event helpers,
// the stdin reader thread, and main(). Widget rendering lives in render.cpp;
// command handling lives in commands.cpp. All share types.h.

#include "types.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#ifdef _WIN32
#include <SDL2/SDL_syswm.h>
#include <dwmapi.h>
#include "../assets/windows/resource.h"
#endif
#include <GL/gl.h>

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <new>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// ─── Global state definitions (externs declared in types.h) ─────────────────

std::mutex                          g_mutex;          // protects g_windows / g_window_order
std::mutex                          g_cmd_mutex;      // protects g_commands queue only
std::queue<QueuedCommand>           g_commands;
std::atomic<bool>                   g_running{true};
std::map<std::string, WindowState>  g_windows;
std::vector<std::string>            g_window_order;
std::vector<json>                   g_events;
std::mutex                          g_events_mutex;   // protects g_events
int                                 g_frame_count = 0;
std::vector<json>                   g_input_queue;    // queued input events to inject
std::mutex                          g_input_mutex;    // protects g_input_queue
static std::atomic<bool>            g_stdin_closed{false};
static thread_local bool            g_has_request_id = false;
static thread_local json            g_request_id;

constexpr size_t MAX_COMMAND_BYTES = 4u * 1024u * 1024u;
constexpr size_t MAX_QUEUED_COMMANDS = 1024u;
constexpr size_t MAX_QUEUED_COMMAND_BYTES = 32u * 1024u * 1024u;
constexpr size_t MAX_SCREENSHOT_PIXEL_BYTES = 128u * 1024u * 1024u;
constexpr size_t MAX_NATIVE_RESPONSE_BYTES = 16u * 1024u * 1024u;
constexpr size_t MAX_RESPONSE_CORRELATION_STRING_BYTES = 256u;
constexpr int MAX_JSON_NESTING_DEPTH = 64;
static size_t g_queued_command_bytes = 0;

// Dear ImGui overlay windows
bool g_show_demo          = false;
bool g_show_metrics       = false;
bool g_show_about         = false;
bool g_show_style_editor  = false;
bool g_show_debug_log     = false;
bool g_show_id_stack_tool = false;

// Background clear color
float g_bg_color[3] = {0.06f, 0.06f, 0.08f};

ScreenshotRequest g_screenshot;

// Animation system
std::vector<Animation>              g_animations;
std::mutex                          g_anim_mutex;

// Undo/Redo & Snapshot system
std::vector<Snapshot>               g_snapshots;
std::vector<json>                   g_undo_stack;
std::vector<json>                   g_redo_stack;
int                                 g_max_undo = 50;

// Texture / Asset system
std::map<std::string, LoadedTexture> g_textures;
std::map<std::string, LoadedFont> g_fonts;
std::string                          g_theme_preset = "dark";

// Safe area insets (top, bottom, left, right)
float g_safe_area[4] = {0, 0, 0, 0};

// SDL window pointer for command-driven resizing
SDL_Window* g_sdl_window = nullptr;

// Scene system
std::map<std::string, Scene> g_scenes;
std::string                  g_active_scene;
int                          g_layer_order = 0;

// ─── Forward declarations for symbols defined in other translation units ─────

// Defined in commands.cpp; acquires g_mutex internally.
void process_command(const json& cmd);

static void annotate_widget(ImDrawList* draw_list, const Widget& widget,
                            int rendered_frame) {
    const ImVec2 minimum(widget.rect_min[0], widget.rect_min[1]);
    const ImVec2 maximum(widget.rect_max[0], widget.rect_max[1]);
    if (widget.last_rendered_frame == rendered_frame &&
        maximum.x > minimum.x && maximum.y > minimum.y) {
        constexpr ImU32 outline = IM_COL32(91, 220, 255, 255);
        constexpr ImU32 label_background = IM_COL32(12, 17, 30, 220);
        draw_list->AddRect(minimum, maximum, outline, 3.0f, 0, 2.0f);
        const ImVec2 text_size = ImGui::CalcTextSize(widget.id.c_str());
        const ImVec2 label_max(minimum.x + text_size.x + 8.0f,
                               minimum.y + text_size.y + 6.0f);
        draw_list->AddRectFilled(minimum, label_max, label_background, 3.0f);
        draw_list->AddText(ImVec2(minimum.x + 4.0f, minimum.y + 3.0f),
                           outline, widget.id.c_str());
    }
    for (const Widget& child : widget.children)
        annotate_widget(draw_list, child, rendered_frame);
}

#ifdef _WIN32
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif

// Match the native non-client area to the app's dark cyan/violet theme. Calls
// are intentionally best-effort so older Windows builds retain their defaults.
static void apply_windows_chrome(SDL_Window* window) {
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_WINDOWS)
        return;

    HWND hwnd = info.info.win.window;
    BOOL dark_mode = TRUE;
    const COLORREF caption = RGB(15, 18, 30);
    const COLORREF text = RGB(239, 242, 255);
    const COLORREF border = RGB(102, 92, 255);
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                          &dark_mode, sizeof(dark_mode));
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &text, sizeof(text));
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &border, sizeof(border));

    HINSTANCE instance = GetModuleHandleW(nullptr);
    HICON large_icon = static_cast<HICON>(LoadImageW(
        instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
    HICON small_icon = static_cast<HICON>(LoadImageW(
        instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
    if (large_icon)
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(large_icon));
    if (small_icon)
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(small_icon));
}
#endif

// ─── Easing functions ────────────────────────────────────────────────────────

static float apply_ease(float t, EaseType ease) {
    const float PI = 3.14159265358979323846f;
    switch (ease) {
    case EaseType::Linear:
        return t;
    case EaseType::EaseIn:
        return t * t;
    case EaseType::EaseOut:
        return 1.0f - (1.0f - t) * (1.0f - t);
    case EaseType::EaseInOut:
        return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    case EaseType::Bounce: {
        const float n1 = 7.5625f, d1 = 2.75f;
        if (t < 1.0f / d1) return n1 * t * t;
        else if (t < 2.0f / d1) { t -= 1.5f / d1; return n1 * t * t + 0.75f; }
        else if (t < 2.5f / d1) { t -= 2.25f / d1; return n1 * t * t + 0.9375f; }
        else { t -= 2.625f / d1; return n1 * t * t + 0.984375f; }
    }
    case EaseType::Elastic: {
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        const float c4 = (2.0f * PI) / 3.0f;
        return -powf(2.0f, 10.0f * t - 10.0f) * sinf((t * 10.0f - 10.75f) * c4);
    }
    case EaseType::Back: {
        const float c1 = 1.70158f, c3 = c1 + 1.0f;
        return c3 * t * t * t - c1 * t * t;
    }
    }
    return t;
}


// ─── Output helpers ──────────────────────────────────────────────────────────

static void write_json_frame(const json& response) {
    const std::string frame = response.dump();
    fwrite(frame.c_str(), 1, frame.size(), stdout);
    fputc('\n', stdout);
    fflush(stdout);
}

static void copy_response_correlation(json& error, const json& response) {
    const auto command = response.find("cmd");
    if (command != response.end() && command->is_string() &&
        command->get_ref<const std::string&>().size() <= MAX_RESPONSE_CORRELATION_STRING_BYTES)
        error["cmd"] = *command;

    const auto request_id = response.find("request_id");
    if (request_id == response.end())
        return;
    if (request_id->is_number() || request_id->is_boolean() || request_id->is_null()) {
        error["request_id"] = *request_id;
    } else if (request_id->is_string() &&
               request_id->get_ref<const std::string&>().size() <= MAX_RESPONSE_CORRELATION_STRING_BYTES) {
        error["request_id"] = *request_id;
    }
}

// Serialize a bounded JSON object to stdout as one newline-delimited line.
// A dedicated static mutex serializes all writers to stdout.
void emit_json(json response) {
    static std::mutex out_mutex;
    std::lock_guard<std::mutex> lock(out_mutex);
    if (g_has_request_id && !response.contains("request_id"))
        response["request_id"] = g_request_id;
    if (response.dump().size() > MAX_NATIVE_RESPONSE_BYTES) {
        json error = {{"type", "error"},
                      {"message", "native response exceeds the 16 MiB protocol limit"}};
        copy_response_correlation(error, response);
        write_json_frame(error);
        return;
    }
    write_json_frame(response);
}

static void finish_screenshot(json response) {
    if (g_screenshot.has_request_id)
        response["request_id"] = g_screenshot.request_id;
    emit_json(response);
    g_screenshot = ScreenshotRequest{};
}

static void remove_incomplete_screenshot(const std::string& path) {
    std::error_code error;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, error);
    if (!error && std::filesystem::is_regular_file(status))
        std::filesystem::remove(path, error);
}

static FILE* open_screenshot_file(const std::string& path, std::string& error_message) {
    std::error_code error;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, error);
    if (error && error != std::errc::no_such_file_or_directory) {
        error_message = "failed to inspect screenshot output path";
        return nullptr;
    }
    if (!error && (std::filesystem::is_symlink(status) ||
                   !std::filesystem::is_regular_file(status))) {
        error_message = "screenshot output path must be a regular file";
        return nullptr;
    }

#ifdef _WIN32
    FILE* file = fopen(path.c_str(), "wb");
    if (!file)
        error_message = "failed to open screenshot output path";
    return file;
#else
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
    const int descriptor = open(path.c_str(), flags, 0666);
    if (descriptor < 0) {
        error_message = "failed to open screenshot output path";
        return nullptr;
    }
    struct stat file_status {};
    if (fstat(descriptor, &file_status) != 0 || !S_ISREG(file_status.st_mode)) {
        close(descriptor);
        error_message = "screenshot output path must be a regular file";
        return nullptr;
    }
    FILE* file = fdopen(descriptor, "wb");
    if (!file) {
        close(descriptor);
        error_message = "failed to open screenshot output path";
    }
    return file;
#endif
}

static const Widget* find_widget_for_screenshot(const WindowState& window,
                                                 const std::string& widget_id) {
    const auto top_level = window.widgets.find(widget_id);
    if (top_level != window.widgets.end())
        return &top_level->second;
    for (const auto& [id, widget] : window.widgets) {
        (void)id;
        std::vector<const Widget*> pending{&widget};
        while (!pending.empty()) {
            const Widget* candidate = pending.back();
            pending.pop_back();
            if (candidate->id == widget_id)
                return candidate;
            for (const Widget& child : candidate->children)
                pending.push_back(&child);
        }
    }
    return nullptr;
}

static bool capture_widget_info(int captured_frame, json& widget_info) {
    if (g_screenshot.target_window.empty() || g_screenshot.target_widget.empty())
        return false;

    std::lock_guard<std::mutex> lock(g_mutex);
    const auto window = g_windows.find(g_screenshot.target_window);
    if (window == g_windows.end())
        return false;
    const Widget* widget = find_widget_for_screenshot(window->second, g_screenshot.target_widget);
    if (!widget || widget->last_rendered_frame != captured_frame)
        return false;

    const float x = widget->rect_min[0];
    const float y = widget->rect_min[1];
    const float width = widget->rect_max[0] - x;
    const float height = widget->rect_max[1] - y;
    if (width <= 0.0f || height <= 0.0f)
        return false;
    widget_info = {{"found", true}, {"window", g_screenshot.target_window},
                   {"widget", g_screenshot.target_widget}, {"x", x}, {"y", y},
                   {"width", width}, {"height", height}};
    return true;
}

static void emit_command_error(const json* command, const std::string& message) {
    json error = {{"type", "error"}, {"cmd", "parse"}, {"message", message}};
    if (command && command->is_object()) {
        const auto command_name = command->find("cmd");
        if (command_name != command->end() && command_name->is_string())
            error["cmd"] = command_name->get<std::string>();
        const auto request_id = command->find("request_id");
        if (request_id != command->end())
            error["request_id"] = *request_id;
    }
    emit_json(std::move(error));
}

static json parse_command_json(const std::string& line, bool& depth_exceeded) {
    depth_exceeded = false;
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
    return json::parse(line, depth_callback);
}

// Queue a UI event for emission at the end of the current frame.
void emit_event(const std::string& window_id, const std::string& widget_id,
                const std::string& event_type, const json& data) {
    json ev;
    ev["type"]   = "event";
    ev["window"] = window_id;
    ev["widget"] = widget_id;
    ev["event"]  = event_type;
    ev["frame"]  = g_frame_count;
    if (!data.empty()) ev["data"] = data;

    std::lock_guard<std::mutex> lock(g_events_mutex);
    g_events.push_back(ev);
}

// ─── Stdin Reader Thread ─────────────────────────────────────────────────────

// Reads newline-delimited JSON commands from stdin and queues them for the main
// loop. On EOF (stdin closed) the application is signalled to shut down.
static bool wait_for_stdin() {
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        return true;
    return WaitForSingleObject(handle, 100) == WAIT_OBJECT_0;
#else
    pollfd descriptor{STDIN_FILENO, POLLIN, 0};
    int result;
    do {
        result = poll(&descriptor, 1, 100);
    } while (result < 0 && errno == EINTR);
    return result > 0 && (descriptor.revents & (POLLIN | POLLHUP | POLLERR));
#endif
}

static void stdin_reader() {
    // One extra byte beyond the payload plus terminator lets an exactly-4 MiB
    // line consume its trailing newline; larger lines still set failbit.
    std::vector<char> buffer(MAX_COMMAND_BYTES + 2u);
    std::string line;
    while (g_running) {
        if (!wait_for_stdin())
            continue;
        std::cin.getline(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        if (std::cin.bad())
            break;
        if (std::cin.fail() && !std::cin.eof()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            emit_json({{"type", "error"}, {"cmd", "parse"},
                       {"message", "command exceeds the 4 MiB protocol limit"}});
            continue;
        }
        if (std::cin.gcount() == 0 && std::cin.eof())
            break;
        line.assign(buffer.data());
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;

        try {
            bool depth_exceeded = false;
            json cmd = parse_command_json(line, depth_exceeded);
            if (depth_exceeded) {
                emit_command_error(&cmd, "JSON nesting exceeds the limit of 64");
                continue;
            }
            if (!cmd.is_object()) {
                emit_command_error(nullptr, "command must be a JSON object");
                continue;
            }
            const auto command_name = cmd.find("cmd");
            if (command_name == cmd.end() || !command_name->is_string()) {
                emit_command_error(&cmd, "command cmd must be a string");
                continue;
            }
            const bool is_quit = command_name->get<std::string>() == "quit";
            {
                std::lock_guard<std::mutex> lock(g_cmd_mutex);
                if (g_commands.size() >= MAX_QUEUED_COMMANDS ||
                    line.size() > MAX_QUEUED_COMMAND_BYTES - g_queued_command_bytes) {
                    emit_command_error(&cmd, "command queue is full");
                    continue;
                }
                g_queued_command_bytes += line.size();
                g_commands.push({std::move(cmd), line.size()});
            }
            // Do not start another blocking read after a queued quit command.
            if (is_quit)
                break;
        } catch (const json::parse_error& e) {
            emit_command_error(nullptr, std::string("JSON parse error: ") + e.what());
        } catch (const std::exception& e) {
            emit_command_error(nullptr, std::string("command error: ") + e.what());
        }
    }
    g_stdin_closed = true;
}

// ─── Main ────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    // Parse command-line args
    int win_width = 1280, win_height = 720;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
            win_width = atoi(argv[++i]);
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
            win_height = atoi(argv[++i]);
    }

    SDL_SetMainReady();
    // Init SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_Window* window = SDL_CreateWindow("imgui-mcp - Live UI Studio",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_width, win_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    g_sdl_window = window;

#ifdef _WIN32
    apply_windows_chrome(window);
#endif

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // VSync

    // Init Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef _WIN32
    // Match the native Windows UI surface instead of falling back to ImGui's
    // intentionally tiny embedded bitmap font. ImGui 1.92 loads glyphs
    // dynamically, so this also covers the symbols used by game-tool UIs.
    if (ImFont* windows_font =
            io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f)) {
        io.FontDefault = windows_font;
        g_fonts.emplace("windows-ui", LoadedFont{
            "windows-ui", "C:\\Windows\\Fonts\\segoeui.ttf", 16.0f, windows_font});
    }
#endif

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding  = 4.0f;
    style.WindowRounding = 6.0f;
    style.GrabRounding   = 3.0f;

    // Setup platform / renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Start stdin reader thread
    std::thread reader(stdin_reader);

    // Signal readiness
    emit_json({{"type", "ready"},
               {"version", IMGUI_MCP_VERSION},
               {"imgui_version", IMGUI_VERSION},
               {"backend", "sdl2_opengl3"}});

    // Main loop
    while (g_running) {
        // (a) Poll and handle SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                g_running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                g_running = false;
        }

        // (b) Drain the command queue (under g_cmd_mutex), then process each
        //     command WITHOUT holding any lock; process_command locks g_mutex.
        {
            std::vector<json> pending;
            {
                std::lock_guard<std::mutex> lock(g_cmd_mutex);
                while (!g_commands.empty()) {
                    QueuedCommand queued = std::move(g_commands.front());
                    g_commands.pop();
                    g_queued_command_bytes -= queued.serialized_bytes;
                    pending.push_back(std::move(queued.command));
                }
            }
            for (auto& cmd : pending) {
                g_has_request_id = cmd.is_object() && cmd.contains("request_id");
                if (g_has_request_id)
                    g_request_id = cmd["request_id"];
                try {
                    if (!cmd.is_object())
                        throw std::invalid_argument("command must be a JSON object");
                    process_command(cmd);
                } catch (const std::exception& e) {
                    emit_json({{"type", "error"},
                               {"cmd", cmd.is_object() ? cmd.value("cmd", "") : ""},
                               {"message", e.what()}});
                } catch (...) {
                    emit_json({{"type", "error"},
                               {"cmd", cmd.is_object() ? cmd.value("cmd", "") : ""},
                               {"message", "unknown native command error"}});
                }
                g_has_request_id = false;
                g_request_id = nullptr;
            }

            // EOF means no further commands can arrive. Process everything
            // already queued before ending the render loop.
            if (g_stdin_closed) {
                std::lock_guard<std::mutex> lock(g_cmd_mutex);
                if (g_commands.empty())
                    g_running = false;
            }
        }

        // (b2) Update animations
        {
            std::lock_guard<std::mutex> lock(g_anim_mutex);
            float dt = ImGui::GetIO().DeltaTime;
            for (auto it = g_animations.begin(); it != g_animations.end(); ) {
                if (!it->active) { it = g_animations.erase(it); continue; }
                it->elapsed += dt;
                float t = std::min(it->elapsed / it->duration, 1.0f);
                float eased = apply_ease(t, it->ease);
                float value = it->from + (it->to - it->from) * eased;
                // Apply to widget
                {
                    std::lock_guard<std::mutex> wlock(g_mutex);
                    auto wit = g_windows.find(it->window_id);
                    if (wit != g_windows.end()) {
                        auto git = wit->second.widgets.find(it->widget_id);
                        if (git != wit->second.widgets.end()) {
                            auto& w = git->second;
                            if (it->property == "value") w.float_val[0] = value;
                            else if (it->property == "opacity") w.color[3] = value;
                        else if (it->property == "pos_x") w.cursor_offset[0] = value;
                        else if (it->property == "pos_y") w.cursor_offset[1] = value;
                            else if (it->property == "size_x") w.size_x = value;
                            else if (it->property == "size_y") w.size_y = value;
                            else if (it->property == "int_value") w.int_val[0] = (int)value;
                        }
                    }
                }
                if (t >= 1.0f) {
                    if (it->loop) { it->elapsed = 0; std::swap(it->from, it->to); }
                    else { it = g_animations.erase(it); continue; }
                }
                ++it;
            }
        }

        // (b2) Inject queued input events into ImGuiIO
        {
            std::lock_guard<std::mutex> lock(g_input_mutex);
            ImGuiIO& io_ref = ImGui::GetIO();
            for (auto& ev : g_input_queue) {
                std::string ev_type = ev.value("type", "");
                if (ev_type == "mouse_move") {
                    io_ref.AddMousePosEvent(ev["x"], ev["y"]);
                } else if (ev_type == "mouse_click") {
                    io_ref.AddMousePosEvent(ev["x"], ev["y"]);
                    io_ref.AddMouseButtonEvent(ev.value("button", 0), true);
                } else if (ev_type == "mouse_release") {
                    io_ref.AddMouseButtonEvent(ev.value("button", 0), false);
                } else if (ev_type == "mouse_scroll") {
                    io_ref.AddMouseWheelEvent(ev.value("x", 0.0f), ev.value("y", 1.0f));
                } else if (ev_type == "key_press") {
                    io_ref.AddKeyEvent((ImGuiKey)ev.value("key", 0), true);
                } else if (ev_type == "key_release") {
                    io_ref.AddKeyEvent((ImGuiKey)ev.value("key", 0), false);
                } else if (ev_type == "char_input") {
                    io_ref.AddInputCharacter(ev.value("char", (unsigned int)'a'));
                } else if (ev_type == "text_input") {
                    std::string text_str = ev.value("text", std::string(""));
                    io_ref.AddInputCharactersUTF8(text_str.c_str());
                }
            }
            g_input_queue.clear();
        }

        // (c) Start a new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // (d) Render application windows (scene-filtered, layer-sorted)
        {
            std::lock_guard<std::mutex> lock(g_mutex);

            // Build list of window ids to render
            std::vector<std::string> render_order;
            if (!g_active_scene.empty()) {
                auto sit = g_scenes.find(g_active_scene);
                if (sit != g_scenes.end()) {
                    // Only windows in the active scene
                    for (auto& wid : sit->second.window_ids) {
                        if (g_windows.find(wid) != g_windows.end())
                            render_order.push_back(wid);
                    }
                }
            } else {
                // No active scene: render all windows in normal order
                render_order = g_window_order;
            }

            // Sort by layer (lower layer renders first, higher on top)
            std::stable_sort(render_order.begin(), render_order.end(),
                [](const std::string& a, const std::string& b) {
                    auto ait = g_windows.find(a);
                    auto bit = g_windows.find(b);
                    int la = (ait != g_windows.end()) ? ait->second.layer : 0;
                    int lb = (bit != g_windows.end()) ? bit->second.layer : 0;
                    return la < lb;
                });

            for (auto& win_id : render_order) {
                auto it = g_windows.find(win_id);
                if (it == g_windows.end()) continue;
                auto& win = it->second;
                if (!win.open) continue;

                // Anchor-based positioning overrides manual x,y when set
                if (win.window_anchor != Anchor::None) {
                    ImVec2 viewport = ImGui::GetMainViewport()->Size;
                    ImVec2 vp_pos = ImGui::GetMainViewport()->Pos;
                    float ax = 0, ay = 0, px = 0, py = 0;
                    switch(win.window_anchor) {
                        case Anchor::TopLeft:      ax = g_safe_area[2]; ay = g_safe_area[0]; break;
                        case Anchor::TopCenter:    ax = viewport.x*0.5f; ay = g_safe_area[0]; px = 0.5f; break;
                        case Anchor::TopRight:     ax = viewport.x - g_safe_area[3]; ay = g_safe_area[0]; px = 1; break;
                        case Anchor::CenterLeft:   ax = g_safe_area[2]; ay = viewport.y*0.5f; py = 0.5f; break;
                        case Anchor::Center:       ax = viewport.x*0.5f; ay = viewport.y*0.5f; px = py = 0.5f; break;
                        case Anchor::CenterRight:  ax = viewport.x - g_safe_area[3]; ay = viewport.y*0.5f; px = 1; py = 0.5f; break;
                        case Anchor::BottomLeft:   ax = g_safe_area[2]; ay = viewport.y - g_safe_area[1]; py = 1; break;
                        case Anchor::BottomCenter: ax = viewport.x*0.5f; ay = viewport.y - g_safe_area[1]; px = 0.5f; py = 1; break;
                        case Anchor::BottomRight:  ax = viewport.x - g_safe_area[3]; ay = viewport.y - g_safe_area[1]; px = py = 1; break;
                        default: break;
                    }
                    ImGui::SetNextWindowPos(ImVec2(vp_pos.x + ax + win.window_anchor_offset[0],
                                                   vp_pos.y + ay + win.window_anchor_offset[1]),
                                           ImGuiCond_Always, ImVec2(px, py));
                } else if (win.apply_geometry) {
                    ImGui::SetNextWindowPos(ImVec2(win.x, win.y), ImGuiCond_Always);
                }
                if (win.apply_geometry)
                    ImGui::SetNextWindowSize(ImVec2(win.w, win.h), ImGuiCond_Always);
                if (win.next_bg_alpha >= 0.0f) {
                    ImGui::SetNextWindowBgAlpha(win.next_bg_alpha);
                    win.next_bg_alpha = -1.0f;
                }
                if (win.next_scroll[0] >= 0.0f || win.next_scroll[1] >= 0.0f) {
                    ImGui::SetNextWindowScroll(ImVec2(win.next_scroll[0], win.next_scroll[1]));
                    win.next_scroll[0] = win.next_scroll[1] = -1.0f;
                }

                const std::string imgui_name = imgui_window_name(win);
                if (ImGui::Begin(imgui_name.c_str(), &win.open, win.flags)) {
                    for (auto& wid : win.widget_order) {
                        auto wit = win.widgets.find(wid);
                        if (wit != win.widgets.end()) {
                            render_widget(win_id, wit->second);
                        }
                    }
                }
                const ImVec2 actual_pos = ImGui::GetWindowPos();
                const ImVec2 actual_size = ImGui::GetWindowSize();
                win.x = actual_pos.x;
                win.y = actual_pos.y;
                win.w = actual_size.x;
                win.h = actual_size.y;
                win.collapsed = ImGui::IsWindowCollapsed();
                win.apply_geometry = false;
                ImGui::End();
            }
        }

        // (e)-(j) Built-in Dear ImGui overlay windows
        if (g_show_demo)
            ImGui::ShowDemoWindow(&g_show_demo);

        if (g_show_metrics)
            ImGui::ShowMetricsWindow(&g_show_metrics);

        if (g_show_about)
            ImGui::ShowAboutWindow(&g_show_about);

        if (g_show_style_editor) {
            ImGui::Begin("Style Editor", &g_show_style_editor);
            ImGui::ShowStyleEditor();
            ImGui::End();
        }

        if (g_show_debug_log)
            ImGui::ShowDebugLogWindow(&g_show_debug_log);

        if (g_show_id_stack_tool)
            ImGui::ShowIDStackToolWindow(&g_show_id_stack_tool);

        if (g_screenshot.pending && g_screenshot.annotated) {
            std::lock_guard<std::mutex> lock(g_mutex);
            ImDrawList* draw_list = ImGui::GetForegroundDrawList();
            for (const auto& [window_id, app_window] : g_windows) {
                (void)window_id;
                for (const auto& [widget_id, widget] : app_window.widgets) {
                    (void)widget_id;
                    annotate_widget(draw_list, widget, g_frame_count);
                }
            }
        }

        // (k) Render and present
        ImGui::Render();
        int fb_w, fb_h;
        SDL_GL_GetDrawableSize(window, &fb_w, &fb_h);
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(g_bg_color[0], g_bg_color[1], g_bg_color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Screenshot capture: read pixels and write 24-bit uncompressed BMP.
        if (g_screenshot.pending) {
            g_screenshot.pending = false;
            json capture_metadata = g_screenshot.metadata;
            if (!g_screenshot.target_window.empty() || !g_screenshot.target_widget.empty()) {
                capture_metadata.erase("widget_info");
                json widget_info;
                if (!capture_widget_info(g_frame_count, widget_info)) {
                    finish_screenshot({{"type", "error"}, {"cmd", g_screenshot.command},
                                       {"message", "target widget was not rendered for screenshot capture"}});
                } else {
                    capture_metadata["widget_info"] = std::move(widget_info);
                }
            }
            if (!g_screenshot.command.empty()) {
                if (fb_w <= 0 || fb_h <= 0) {
                    finish_screenshot({{"type", "error"}, {"cmd", g_screenshot.command},
                                       {"message", "screenshot framebuffer is unavailable"}});
                } else {
                    const size_t width = static_cast<size_t>(fb_w);
                    const size_t height = static_cast<size_t>(fb_h);
                    const bool oversized = width > std::numeric_limits<size_t>::max() / height / 3u ||
                        width * height * 3u > MAX_SCREENSHOT_PIXEL_BYTES;
                    if (oversized) {
                        finish_screenshot({{"type", "error"}, {"cmd", g_screenshot.command},
                                           {"message", "screenshot dimensions are too large"}});
                    } else {
                        try {
                            std::vector<unsigned char> pixels(width * height * 3u);
                            GLint previous_pack_alignment = 4;
                            glGetIntegerv(GL_PACK_ALIGNMENT, &previous_pack_alignment);
                            glPixelStorei(GL_PACK_ALIGNMENT, 1);
                            glReadPixels(0, 0, fb_w, fb_h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
                            glPixelStorei(GL_PACK_ALIGNMENT, previous_pack_alignment);
                            const size_t row_size = ((width * 3u + 3u) / 4u) * 4u;
                            const size_t data_size = row_size * height;
                            const size_t file_size = 54u + data_size;
                            if (file_size > std::numeric_limits<uint32_t>::max()) {
                                finish_screenshot({{"type", "error"}, {"cmd", g_screenshot.command},
                                                   {"message", "screenshot file would exceed the BMP size limit"}});
                            } else {
                                std::vector<unsigned char> row(row_size, 0);
                                std::string open_error;
                                FILE* f = open_screenshot_file(g_screenshot.path, open_error);
                                if (!f) {
                                    finish_screenshot({{"type", "error"}, {"cmd", g_screenshot.command},
                                                       {"message", "failed to write screenshot: " + open_error}});
                                } else {
                                    unsigned char header[54] = {};
                                    header[0] = 'B'; header[1] = 'M';
                                    auto write_u32 = [&header](size_t offset, uint32_t value) {
                                        header[offset] = static_cast<unsigned char>(value & 0xffu);
                                        header[offset + 1u] = static_cast<unsigned char>((value >> 8u) & 0xffu);
                                        header[offset + 2u] = static_cast<unsigned char>((value >> 16u) & 0xffu);
                                        header[offset + 3u] = static_cast<unsigned char>((value >> 24u) & 0xffu);
                                    };
                                    write_u32(2u, static_cast<uint32_t>(file_size));
                                    header[10] = 54;
                                    header[14] = 40;
                                    write_u32(18u, static_cast<uint32_t>(fb_w));
                                    write_u32(22u, static_cast<uint32_t>(fb_h));
                                    header[26] = 1;
                                    header[28] = 24;
                                    write_u32(34u, static_cast<uint32_t>(data_size));
                                    header[38] = 0x13; header[39] = 0x0B;
                                    header[42] = 0x13; header[43] = 0x0B;

                                    bool write_ok = fwrite(header, 1, sizeof(header), f) == sizeof(header);
                                    for (int y = 0; write_ok && y < fb_h; ++y) {
                                        const unsigned char* src =
                                            &pixels[static_cast<size_t>(y) * width * 3u];
                                        for (int x = 0; x < fb_w; ++x) {
                                            const size_t pixel = static_cast<size_t>(x) * 3u;
                                            row[pixel + 0u] = src[pixel + 2u]; // B
                                            row[pixel + 1u] = src[pixel + 1u]; // G
                                            row[pixel + 2u] = src[pixel + 0u]; // R
                                        }
                                        write_ok = fwrite(row.data(), 1, row_size, f) == row_size;
                                    }
                                    write_ok = fclose(f) == 0 && write_ok;
                                    if (write_ok) {
                                        json screenshot = {{"type", "screenshot"}, {"cmd", g_screenshot.command},
                                                           {"path", g_screenshot.path}, {"width", fb_w},
                                                           {"height", fb_h}, {"annotated", g_screenshot.annotated}};
                                        screenshot.update(capture_metadata);
                                        finish_screenshot(std::move(screenshot));
                                    } else {
                                        const std::string path = g_screenshot.path;
                                        remove_incomplete_screenshot(path);
                                        finish_screenshot({{"type", "error"}, {"cmd", g_screenshot.command},
                                                           {"message", "failed to write screenshot: " + path}});
                                    }
                                }
                            }
                        } catch (const std::bad_alloc&) {
                            finish_screenshot({{"type", "error"}, {"cmd", g_screenshot.command},
                                               {"message", "insufficient memory for screenshot capture"}});
                        }
                    }
                }
            }
        }

        SDL_GL_SwapWindow(window);

        // (l) Flush queued events to stdout
        {
            std::lock_guard<std::mutex> lock(g_events_mutex);
            for (auto& ev : g_events) {
                emit_json(ev);
            }
            g_events.clear();
        }

        // (m) Advance frame counter
        g_frame_count++;
    }

    // Cleanup
    reader.join();
    for (auto& [texture_id, texture] : g_textures) {
        (void)texture_id;
        if (texture.tex_id != 0)
            glDeleteTextures(1, &texture.tex_id);
    }
    g_textures.clear();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    emit_json({{"type", "shutdown"}, {"frames", g_frame_count}});
    return 0;
}
