#include "header.hpp"

#include <stdio.h>
#if defined(__unix__) || defined(__unix)
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#endif

std::string open_file_dialog() {
    char filename[1024];
    std::string home_dir = "";
    const char* home_env = getenv("HOME");
    if (home_env) {
        home_dir = std::string(home_env) + "/";
    }

    bool has_zenity = system("which zenity > /dev/null 2>&1") == 0;
    bool has_kdialog = system("which kdialog > /dev/null 2>&1") == 0;

    std::string cmd_zenity = "zenity --file-selection --title=\"Hintergrundbild auswählen\" --filename=\"" + home_dir + "\" 2>/dev/null";
    std::string cmd_kdialog = "kdialog --getopenfilename \"" + home_dir + "\" \"image/png image/jpeg\" 2>/dev/null";
    
    std::string cmd = "";
    const char* desktop = getenv("XDG_CURRENT_DESKTOP");
    bool is_kde = desktop && (std::string(desktop).find("KDE") != std::string::npos);

    if (is_kde && has_kdialog) {
        cmd = cmd_kdialog;
    } else if (has_zenity) {
        cmd = cmd_zenity;
    } else if (has_kdialog) {
        cmd = cmd_kdialog;
    } else {
        return "";
    }

    FILE* f = popen(cmd.c_str(), "r");
    if (f && fgets(filename, sizeof(filename), f) != NULL) {
        std::string result = filename;
        if (!result.empty() && result.back() == '\n') result.pop_back();
        pclose(f);
        return result;
    }
    if (f) pclose(f);

    return "";
}

namespace launcher {
sf::Font font;
int selected_mode = 1;
bool any_key_enabled = false;
bool smoke_enabled = false;
bool is_launcher = true;

bool init() {
    if (!font.loadFromFile("font/RobotoMono-Bold.ttf")) {
        data::error_msg("Failed to load font from font/RobotoMono-Bold.ttf", "Error");
        return false;
    }
    selected_mode = data::cfg["mode"].asInt();
    if (data::cfg["osu"].isMember("anyKey")) {
        any_key_enabled = data::cfg["osu"]["anyKey"].asBool();
    }
    if (data::cfg["osu"].isMember("toggleSmoke")) {
        smoke_enabled = data::cfg["osu"]["toggleSmoke"].asBool();
    }
    return true;
}

void draw_text(std::string str, int x, int y, int size, sf::Color color) {
    sf::Text text;
    text.setFont(font);
    text.setString(str);
    text.setCharacterSize(size);
    text.setFillColor(color);
    text.setPosition(x, y);
    window.draw(text);
}

void draw() {
    window.clear(sf::Color(25, 25, 35)); // Dark modern background

    draw_text("Bongo Cat Launcher", 20, 15, 28, sf::Color::White);
    draw_text("Select Mode:", 20, 60, 16, sf::Color(180, 180, 200));

    std::string modes[] = {"osu! (Mouse)", "Taiko (Drums)", "Catch", "Mania", "Custom"};
    
    auto mouse_pos = sf::Mouse::getPosition(window);
    static bool last_mouse = false;
    bool current_mouse = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    bool clicked = current_mouse && !last_mouse;
    last_mouse = current_mouse;

    // Mode Buttons
    for (int i = 0; i < 5; i++) {
        sf::RectangleShape btn(sf::Vector2f(180, 35));
        btn.setPosition(20, 90 + i * 45);
        btn.setOutlineThickness(1);
        
        bool hovered = btn.getGlobalBounds().contains(mouse_pos.x, mouse_pos.y);
        if (hovered && clicked) {
            if (selected_mode != i + 1) {
                selected_mode = i + 1;
                data::cfg["mode"] = selected_mode;
                data::save_config(); // Fails gracefully in AppImage
                data::img_holder.clear();
                switch (selected_mode) {
                    case 1: osu::init(); break;
                    case 2: taiko::init(); break;
                    case 3: ctb::init(); break;
                    case 4: mania::init(); break;
                    case 5: custom::init(); break;
                }
            }
        }

        if (selected_mode == i + 1) {
            btn.setFillColor(sf::Color(65, 105, 225)); // Active (Royal Blue)
            btn.setOutlineColor(sf::Color(100, 150, 255));
        } else if (hovered) {
            btn.setFillColor(sf::Color(45, 45, 65)); // Hover
            btn.setOutlineColor(sf::Color(80, 80, 100));
        } else {
            btn.setFillColor(sf::Color(30, 30, 45)); // Idle
            btn.setOutlineColor(sf::Color(50, 50, 70));
        }

        window.draw(btn);
        draw_text(modes[i], 35, 98 + i * 45, 14, sf::Color::White);
    }

    // Green Screen Checkbox
    sf::RectangleShape gsCheckbox(sf::Vector2f(16, 16));
    gsCheckbox.setPosition(220, 62);
    gsCheckbox.setOutlineThickness(1);
    gsCheckbox.setOutlineColor(sf::Color(100, 100, 120));
    
    sf::FloatRect gsCheckBounds(220, 62, 170, 16);
    bool gs_hovered = gsCheckBounds.contains(mouse_pos.x, mouse_pos.y);
    if (gs_hovered && clicked) {
        bool is_green = (data::cfg["decoration"]["rgb"][0].asInt() == 0 && data::cfg["decoration"]["rgb"][1].asInt() == 255);
        if (is_green) {
            data::cfg["decoration"]["rgb"][0] = 255;
            data::cfg["decoration"]["rgb"][1] = 255;
            data::cfg["decoration"]["rgb"][2] = 255;
        } else {
            data::cfg["decoration"]["rgb"][0] = 0;
            data::cfg["decoration"]["rgb"][1] = 255;
            data::cfg["decoration"]["rgb"][2] = 0;
        }
        data::save_config();
    }
    
    bool is_green_now = (data::cfg["decoration"]["rgb"][0].asInt() == 0 && data::cfg["decoration"]["rgb"][1].asInt() == 255);
    gsCheckbox.setFillColor(is_green_now ? sf::Color(46, 204, 113) : sf::Color(40, 40, 50));
    window.draw(gsCheckbox);
    draw_text("Green Screen", 245, 62, 14, sf::Color::White);

    // Custom Background File Picker
    bool has_custom_bg = data::cfg["decoration"].isMember("customBackground") && data::cfg["decoration"]["customBackground"].asString() != "";

    sf::RectangleShape bgBtn(sf::Vector2f(120, 20));
    bgBtn.setPosition(366, 60);
    bgBtn.setOutlineThickness(1);
    bgBtn.setOutlineColor(sf::Color(100, 150, 255));
    bool bgBtn_hovered = bgBtn.getGlobalBounds().contains(mouse_pos.x, mouse_pos.y);
    bgBtn.setFillColor(bgBtn_hovered ? sf::Color(100, 150, 255, 100) : sf::Color(40, 40, 50));
    window.draw(bgBtn);
    draw_text("Bild aendern...", 376, 63, 12, sf::Color::White);

    if (bgBtn_hovered && clicked) {
        if (system("which zenity > /dev/null 2>&1 || which kdialog > /dev/null 2>&1") != 0) {
            data::error_msg("Um diese Funktion zu nutzen, muss entweder 'zenity' oder 'kdialog' installiert sein.\nBitte installiere 'zenity' in deinem Terminal (z.B. sudo pacman -S zenity oder sudo apt install zenity).", "Fehlendes Paket");
        } else {
            std::string path = open_file_dialog();
            if (!path.empty()) {
                data::cfg["decoration"]["customBackground"] = path;
                data::save_config();
                data::img_holder.clear();
                switch (selected_mode) {
                    case 1: osu::init(); break;
                    case 2: taiko::init(); break;
                    case 3: ctb::init(); break;
                    case 4: mania::init(); break;
                    case 5: custom::init(); break;
                }
            }
        }
    }

    if (has_custom_bg) {
        sf::RectangleShape clearBtn(sf::Vector2f(20, 20));
        clearBtn.setPosition(492, 60);
        clearBtn.setOutlineThickness(1);
        clearBtn.setOutlineColor(sf::Color(255, 100, 100));
        bool clearBtn_hovered = clearBtn.getGlobalBounds().contains(mouse_pos.x, mouse_pos.y);
        clearBtn.setFillColor(clearBtn_hovered ? sf::Color(255, 100, 100, 100) : sf::Color(40, 40, 50));
        window.draw(clearBtn);
        draw_text("X", 498, 62, 14, sf::Color(255, 100, 100));

        if (clearBtn_hovered && clicked) {
            data::cfg["decoration"]["customBackground"] = "";
            data::save_config();
            data::img_holder.clear();
            switch (selected_mode) {
                case 1: osu::init(); break;
                case 2: taiko::init(); break;
                case 3: ctb::init(); break;
                case 4: mania::init(); break;
                case 5: custom::init(); break;
            }
        }
    }

    // Draw Preview Image
    if (selected_mode >= 1 && selected_mode <= 5) {
        // Frame around preview
        sf::RectangleShape frame(sf::Vector2f(306, 177));
        frame.setPosition(220, 90);
        Json::Value rgb = data::cfg["decoration"]["rgb"];
        frame.setFillColor(sf::Color(rgb[0].asInt(), rgb[1].asInt(), rgb[2].asInt()));
        frame.setOutlineThickness(2);
        frame.setOutlineColor(sf::Color(100, 150, 255, 120));
        window.draw(frame);

        // Set viewport for preview
        sf::View previewView(sf::FloatRect(0, 0, 612, 354));
        previewView.setViewport(sf::FloatRect(220.0f / 612.0f, 90.0f / 354.0f, 306.0f / 612.0f, 177.0f / 354.0f));

        sf::View defaultView = window.getView();
        window.setView(previewView);

        if (data::has_custom_bg) {
            sf::Sprite custom_bg_sprite(data::custom_bg_tex);
            window.draw(custom_bg_sprite);
        }

        // Draw the selected mode natively!
        switch (selected_mode) {
            case 1: osu::draw(); break;
            case 2: taiko::draw(); break;
            case 3: ctb::draw(); break;
            case 4: mania::draw(); break;
            case 5: custom::draw(); break;
        }

        window.setView(defaultView);
    }

    // Smoke Checkbox
    sf::RectangleShape smokeBox(sf::Vector2f(20, 20));
    smokeBox.setPosition(220, 255);
    smokeBox.setOutlineThickness(1);
    smokeBox.setOutlineColor(sf::Color(100, 100, 120));
    
    sf::FloatRect smokeBounds(220, 255, 250, 20);
    bool sb_hovered = smokeBounds.contains(mouse_pos.x, mouse_pos.y);
    if (sb_hovered && clicked) {
        smoke_enabled = !smoke_enabled;
        data::cfg["osu"]["toggleSmoke"] = smoke_enabled;
        data::save_config();
        
        switch (selected_mode) {
            case 1: osu::init(); break;
            case 2: taiko::init(); break;
            case 3: ctb::init(); break;
            case 4: mania::init(); break;
            case 5: custom::init(); break;
        }
    }
    
    smokeBox.setFillColor(smoke_enabled ? sf::Color(46, 204, 113) : sf::Color(40, 40, 50));
    window.draw(smokeBox);
    draw_text("Smoke Overlay dauerhaft (statt Taste halten)", 250, 257, 12, sf::Color::White);

    // Any Key Checkbox
    sf::RectangleShape checkbox(sf::Vector2f(20, 20));
    checkbox.setPosition(220, 280);
    checkbox.setOutlineThickness(1);
    checkbox.setOutlineColor(sf::Color(100, 100, 120));
    
    // Make text also clickable for checkbox
    sf::FloatRect checkBounds(220, 280, 250, 20);
    bool cb_hovered = checkBounds.contains(mouse_pos.x, mouse_pos.y);
    if (cb_hovered && clicked) {
        any_key_enabled = !any_key_enabled;
    }
    
    checkbox.setFillColor(any_key_enabled ? sf::Color(46, 204, 113) : sf::Color(40, 40, 50));
    window.draw(checkbox);
    draw_text("\"Any Key\" Feature (Tippen mit der Katze)", 250, 280, 14, sf::Color::White);

    // Start Button
    sf::RectangleShape startBtn(sf::Vector2f(306, 35));
    startBtn.setPosition(220, 315);
    startBtn.setOutlineThickness(1);
    startBtn.setOutlineColor(sf::Color(46, 204, 113));
    
    bool start_hovered = startBtn.getGlobalBounds().contains(mouse_pos.x, mouse_pos.y);
    startBtn.setFillColor(start_hovered ? sf::Color(46, 204, 113) : sf::Color(39, 174, 96));
    
    if (start_hovered && clicked) {
        data::cfg["mode"] = selected_mode;
        data::cfg["osu"]["anyKey"] = any_key_enabled;
        data::cfg["taiko"]["anyKey"] = any_key_enabled;
        data::save_config(); // Fails gracefully in AppImage
        
#if defined(__unix__) || defined(__unix)
        static pid_t cat_pid = 0;
        if (cat_pid > 0) {
            kill(cat_pid, SIGTERM);
        }

        cat_pid = fork();
        if (cat_pid == 0) {
            const char* appimage_env = getenv("APPIMAGE");
            if (appimage_env) {
                execlp(appimage_env, appimage_env, "--run-cat", NULL);
            } else {
                char exe_path[PATH_MAX];
                ssize_t count = readlink("/proc/self/exe", exe_path, PATH_MAX);
                if (count != -1) {
                    exe_path[count] = '\0';
                    execlp(exe_path, exe_path, "--run-cat", NULL);
                }
            }
            exit(1);
        }
#endif
    }
    
    window.draw(startBtn);
    draw_text("Bongo Cat Starten!", 295, 323, 16, sf::Color::White);
}
}; // namespace launcher
