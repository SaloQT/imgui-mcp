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
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/gl.h>

#include <cstdio>
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

// ─── Global state definitions (externs declared in types.h) ─────────────────

std::mutex                          g_mutex;          // protects g_windows / g_window_order
std::mutex                          g_cmd_mutex;      // protects g_commands queue only
std::queue<json>                    g_commands;
std::atomic<bool>                   g_running{true};
std::map<std::string, WindowState>  g_windows;
std::vector<std::string>            g_window_order;
std::vector<json>                   g_events;
std::mutex                          g_events_mutex;   // protects g_events
int                                 g_frame_count = 0;
std::vector<json>                   g_input_queue;    // queued input events to inject
std::mutex                          g_input_mutex;    // protects g_input_queue

// Dear ImGui overlay windows
bool g_show_demo          = false;
bool g_show_metrics       = false;
bool g_show_about         = false;
bool g_show_style_editor  = false;
bool g_show_debug_log     = false;
bool g_show_id_stack_tool = false;

// Background clear color
float g_bg_color[3] = {0.06f, 0.06f, 0.08f};

// Screenshot capture state
std::string g_screenshot_path        = "/tmp/imgui_screenshot.bmp";
bool        g_screenshot_requested   = false;
int         g_screenshot_quality     = 90;
bool        g_screenshot_annotated   = false;

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

// Serialize a JSON object to stdout as one newline-delimited line.
// A dedicated static mutex serializes all writers to stdout.
void emit_json(const json& j) {
    static std::mutex out_mutex;
    std::lock_guard<std::mutex> lock(out_mutex);
    std::string s = j.dump();
    fwrite(s.c_str(), 1, s.size(), stdout);
    fputc('\n', stdout);
    fflush(stdout);
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
static void stdin_reader() {
    std::string line;
    char buf[65536];
    while (g_running) {
        if (!fgets(buf, sizeof(buf), stdin)) {
            // stdin closed -> request shutdown
            g_running = false;
            break;
        }
        line = buf;
        // Trim trailing newline / carriage return
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            line.pop_back();
        if (line.empty()) continue;

        try {
            json cmd = json::parse(line);
            std::lock_guard<std::mutex> lock(g_cmd_mutex);
            g_commands.push(cmd);
        } catch (const json::parse_error& e) {
            emit_json({{"type", "error"},
                       {"message", std::string("JSON parse error: ") + e.what()}});
        }
    }
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

    SDL_Window* window = SDL_CreateWindow("ImGui MCP",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_width, win_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    g_sdl_window = window;

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
               {"version", IMGUI_VERSION},
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
                    pending.push_back(std::move(g_commands.front()));
                    g_commands.pop();
                }
            }
            for (auto& cmd : pending) {
                process_command(cmd);
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
                            else if (it->property == "pos_x") w.size_x = value;
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
                    for (char c : text_str)
                        io_ref.AddInputCharacter((unsigned int)c);
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
                    float ax = 0, ay = 0;
                    switch(win.window_anchor) {
                        case Anchor::TopLeft:      ax = g_safe_area[2]; ay = g_safe_area[0]; break;
                        case Anchor::TopCenter:    ax = viewport.x*0.5f; ay = g_safe_area[0]; break;
                        case Anchor::TopRight:     ax = viewport.x - g_safe_area[3]; ay = g_safe_area[0]; break;
                        case Anchor::CenterLeft:   ax = g_safe_area[2]; ay = viewport.y*0.5f; break;
                        case Anchor::Center:       ax = viewport.x*0.5f; ay = viewport.y*0.5f; break;
                        case Anchor::CenterRight:  ax = viewport.x - g_safe_area[3]; ay = viewport.y*0.5f; break;
                        case Anchor::BottomLeft:   ax = g_safe_area[2]; ay = viewport.y - g_safe_area[1]; break;
                        case Anchor::BottomCenter: ax = viewport.x*0.5f; ay = viewport.y - g_safe_area[1]; break;
                        case Anchor::BottomRight:  ax = viewport.x - g_safe_area[3]; ay = viewport.y - g_safe_area[1]; break;
                        default: break;
                    }
                    ImGui::SetNextWindowPos(ImVec2(vp_pos.x + ax + win.window_anchor_offset[0],
                                                   vp_pos.y + ay + win.window_anchor_offset[1]),
                                           ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                } else {
                    ImGui::SetNextWindowPos(ImVec2(win.x, win.y), ImGuiCond_FirstUseEver);
                }
                ImGui::SetNextWindowSize(ImVec2(win.w, win.h), ImGuiCond_FirstUseEver);

                if (ImGui::Begin(win.title.c_str(), &win.open, win.flags)) {
                    for (auto& wid : win.widget_order) {
                        auto wit = win.widgets.find(wid);
                        if (wit != win.widgets.end()) {
                            render_widget(win_id, wit->second);
                        }
                    }
                }
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

        // (k) Render and present
        ImGui::Render();
        int fb_w, fb_h;
        SDL_GL_GetDrawableSize(window, &fb_w, &fb_h);
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(g_bg_color[0], g_bg_color[1], g_bg_color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Screenshot capture: read pixels and write 24-bit uncompressed BMP
        if (g_screenshot_requested) {
            g_screenshot_requested = false;
            if (fb_w > 0 && fb_h > 0) {
                std::vector<unsigned char> pixels(fb_w * fb_h * 3);
                glReadPixels(0, 0, fb_w, fb_h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
                int row_size = ((fb_w * 3 + 3) / 4) * 4;
                int data_size = row_size * fb_h;
                int file_size = 54 + data_size;
                FILE* f = fopen(g_screenshot_path.c_str(), "wb");
                if (f) {
                    unsigned char header[54] = {};
                    header[0] = 'B'; header[1] = 'M';
                    header[2] = file_size & 0xFF;
                    header[3] = (file_size >> 8) & 0xFF;
                    header[4] = (file_size >> 16) & 0xFF;
                    header[5] = (file_size >> 24) & 0xFF;
                    header[10] = 54;
                    header[14] = 40;
                    header[18] = fb_w & 0xFF;
                    header[19] = (fb_w >> 8) & 0xFF;
                    header[20] = (fb_w >> 16) & 0xFF;
                    header[21] = (fb_w >> 24) & 0xFF;
                    header[22] = fb_h & 0xFF;
                    header[23] = (fb_h >> 8) & 0xFF;
                    header[24] = (fb_h >> 16) & 0xFF;
                    header[25] = (fb_h >> 24) & 0xFF;
                    header[26] = 1;
                    header[28] = 24;
                    header[34] = data_size & 0xFF;
                    header[35] = (data_size >> 8) & 0xFF;
                    header[36] = (data_size >> 16) & 0xFF;
                    header[37] = (data_size >> 24) & 0xFF;
                    header[38] = 0x13; header[39] = 0x0B;
                    header[42] = 0x13; header[43] = 0x0B;
                    fwrite(header, 1, 54, f);
                    std::vector<unsigned char> row(row_size, 0);
                    for (int y = 0; y < fb_h; y++) {
                        const unsigned char* src = &pixels[y * fb_w * 3];
                        for (int x = 0; x < fb_w; x++) {
                            row[x * 3 + 0] = src[x * 3 + 2]; // B
                            row[x * 3 + 1] = src[x * 3 + 1]; // G
                            row[x * 3 + 2] = src[x * 3 + 0]; // R
                        }
                        fwrite(row.data(), 1, row_size, f);
                    }
                    fclose(f);
                    emit_json({{"type", "screenshot"}, {"path", g_screenshot_path},
                               {"width", fb_w}, {"height", fb_h},
                               {"annotated", g_screenshot_annotated}});
                } else {
                    emit_json({{"type", "error"}, {"message", "failed to write screenshot: " + g_screenshot_path}});
                }
                g_screenshot_annotated = false;
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
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    emit_json({{"type", "shutdown"}, {"frames", g_frame_count}});
    return 0;
}
