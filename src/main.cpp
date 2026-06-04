#include "header.hpp"

#if !defined(__unix__) && !defined(__unix)
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <libgen.h>
#endif

sf::RenderWindow window;

#if defined(__unix__) || defined(__unix)
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, char ** argv) {
    bool skip_launcher = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--run-cat") == 0) skip_launcher = true;
    }

    // Automatically change working directory to the parent of the 'bin' folder
    char exe_path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", exe_path, PATH_MAX);
    if (count != -1) {
        exe_path[count] = '\0';
        char* dir = dirname(exe_path); // gets the directory containing the executable (e.g. .../bin)
        char* parent_dir = dirname(dir);
        chdir(parent_dir);
    }

    // Check if we have evdev access, if not and we are not root, prompt for password via pkexec
    if (geteuid() != 0) {
        bool can_open_any = false;
        DIR *dir = opendir("/dev/input");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (strncmp(ent->d_name, "event", 5) == 0) {
                    std::string path = std::string("/dev/input/") + ent->d_name;
                    int fd = open(path.c_str(), O_RDONLY);
                    if (fd >= 0) {
                        close(fd);
                        can_open_any = true;
                        break;
                    }
                }
            }
            closedir(dir);
        }

        if (!can_open_any) {
            const char* appimage_env = getenv("APPIMAGE");
            const char* target_exe = appimage_env ? appimage_env : exe_path;
            if (system("which pkexec > /dev/null 2>&1") == 0) {
                const char* disp = getenv("DISPLAY");
                const char* wayl = getenv("WAYLAND_DISPLAY");
                const char* xauth = getenv("XAUTHORITY");
                const char* home = getenv("HOME");
                
                std::string env_disp = disp ? std::string("DISPLAY=") + disp : "DISPLAY=:0";
                std::string env_wayl = wayl ? std::string("WAYLAND_DISPLAY=") + wayl : "WAYLAND_DISPLAY=wayland-0";
                std::string env_xauth = xauth ? std::string("XAUTHORITY=") + xauth : "";
                std::string env_home = home ? std::string("HOME=") + home : "";

                std::vector<const char*> args;
                args.push_back("pkexec");
                args.push_back("env");
                args.push_back(env_disp.c_str());
                args.push_back(env_wayl.c_str());
                if (xauth) args.push_back(env_xauth.c_str());
                if (home) args.push_back(env_home.c_str());
                args.push_back(target_exe);
                for (int i = 1; i < argc; i++) args.push_back(argv[i]);
                args.push_back(NULL);

                execvp("pkexec", (char* const*)args.data());
                // If execvp returns, pkexec failed or was cancelled by user, so we continue with X11 fallback
            }
        }
    }
#else
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    bool skip_launcher = false;
#endif

    if (skip_launcher) {
        window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bongo Cat for osu!", sf::Style::Titlebar | sf::Style::Close);
    } else {
        window.create(sf::VideoMode(WINDOW_WIDTH, 420), "Bongo Cat for osu! (Launcher)", sf::Style::Titlebar | sf::Style::Close);
    }
    window.setFramerateLimit(MAX_FRAMERATE);

    // loading configs
    while (!data::init()) {
        continue;
    }

    // initialize input
    if (!input::init()) {
        return EXIT_FAILURE;
    }

    if (!launcher::init()) {
        return EXIT_FAILURE;
    }

    if (skip_launcher) {
        launcher::is_launcher = false;
    }

    bool is_reload = false;
    bool is_show_input_debug = false;
    bool was_launcher = true;
    bool last_is_green = false;

    bool is_dragging = false;
    bool is_dragging_bg = false;
    bool is_dragging_cat = false;
    sf::Vector2i drag_offset;
    sf::Vector2i bg_drag_last_pos;
    sf::Vector2i cat_drag_last_pos;

    while (window.isOpen()) {
        if (was_launcher && !launcher::is_launcher) {
            was_launcher = false;
            // Wenn Green Screen oder ein eigener Hintergrund aktiv ist, automatisch den Rahmen entfernen!
            last_is_green = (data::cfg["decoration"]["rgb"][0].asInt() == 0 && data::cfg["decoration"]["rgb"][1].asInt() == 255);
            if (last_is_green || data::has_custom_bg) {
                window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bongo Cat for osu!", sf::Style::None);
            } else {
                window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bongo Cat for osu!", sf::Style::Titlebar | sf::Style::Close);
            }
            window.setFramerateLimit(MAX_FRAMERATE);
            window.setView(sf::View(sf::FloatRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT)));
        }

        if (!launcher::is_launcher) {
#if defined(__unix__) || defined(__unix)
            struct stat st;
            std::string cfg_path = data::get_config_path();
            if (stat(cfg_path.c_str(), &st) == 0) {
                static time_t last_mtime = st.st_mtime;
                if (st.st_mtime > last_mtime) {
                    last_mtime = st.st_mtime;
                    while (!data::init()) { continue; }

                    bool is_green = (data::cfg["decoration"]["rgb"][0].asInt() == 0 && data::cfg["decoration"]["rgb"][1].asInt() == 255);
                    if (is_green != last_is_green) {
                        last_is_green = is_green;
                        if (is_green || data::has_custom_bg) {
                            window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bongo Cat for osu!", sf::Style::None);
                        } else {
                            window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bongo Cat for osu!", sf::Style::Titlebar | sf::Style::Close);
                        }
                        window.setFramerateLimit(MAX_FRAMERATE);
                        window.setView(sf::View(sf::FloatRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT)));
                    }
                }
            }
#endif
        }

        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;

            case sf::Event::MouseWheelScrolled:
                if (launcher::is_launcher) {
                    launcher::handle_scroll(event.mouseWheelScroll.delta);
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                    double cScale = data::cfg["decoration"].isMember("catScale") ? data::cfg["decoration"]["catScale"].asDouble() : 1.0;
                    cScale += event.mouseWheelScroll.delta * 0.05;
                    if (cScale < 0.1) cScale = 0.1;
                    if (cScale > 10.0) cScale = 10.0;
                    data::cfg["decoration"]["catScale"] = cScale;
                    data::save_config();
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
                    if (data::has_custom_bg) {
                        double current_scale = data::cfg["decoration"].isMember("customBackgroundScale") ? data::cfg["decoration"]["customBackgroundScale"].asDouble() : 1.0;
                        current_scale += event.mouseWheelScroll.delta * 0.05;
                        if (current_scale < 0.1) current_scale = 0.1;
                        if (current_scale > 10.0) current_scale = 10.0;
                        data::cfg["decoration"]["customBackgroundScale"] = current_scale;
                        data::save_config();
                    }
                }
                break;

            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left && !launcher::is_launcher) {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                        is_dragging_cat = true;
                        cat_drag_last_pos = sf::Mouse::getPosition(window);
                    } else if ((sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) && data::has_custom_bg) {
                        is_dragging_bg = true;
                        bg_drag_last_pos = sf::Mouse::getPosition(window);
                    } else {
                        is_dragging = true;
                        drag_offset = window.getPosition() - sf::Mouse::getPosition();
                    }
                }
                break;
            case sf::Event::MouseButtonReleased:
                if (event.mouseButton.button == sf::Mouse::Left) {
                    is_dragging = false;
                    is_dragging_bg = false;
                    is_dragging_cat = false;
                }
                break;
            case sf::Event::MouseMoved:
                if (!launcher::is_launcher) {
                    if (is_dragging) {
                        window.setPosition(sf::Mouse::getPosition() + drag_offset);
                    } else if (is_dragging_cat) {
                        int dx = sf::Mouse::getPosition(window).x - cat_drag_last_pos.x;
                        int dy = sf::Mouse::getPosition(window).y - cat_drag_last_pos.y;
                        double cScale = data::cfg["decoration"].isMember("catScale") ? data::cfg["decoration"]["catScale"].asDouble() : 1.0;
                        double cOffX = data::cfg["decoration"].isMember("catOffsetX") ? data::cfg["decoration"]["catOffsetX"].asDouble() : 0.0;
                        double cOffY = data::cfg["decoration"].isMember("catOffsetY") ? data::cfg["decoration"]["catOffsetY"].asDouble() : 0.0;
                        data::cfg["decoration"]["catOffsetX"] = cOffX + dx / cScale;
                        data::cfg["decoration"]["catOffsetY"] = cOffY + dy / cScale;
                        data::save_config();
                        cat_drag_last_pos = sf::Mouse::getPosition(window);
                    } else if (is_dragging_bg) {
                        int dx = sf::Mouse::getPosition(window).x - bg_drag_last_pos.x;
                        int dy = sf::Mouse::getPosition(window).y - bg_drag_last_pos.y;
                        int ox = data::cfg["decoration"]["customBackgroundOffsetX"].asInt();
                        int oy = data::cfg["decoration"]["customBackgroundOffsetY"].asInt();
                        data::cfg["decoration"]["customBackgroundOffsetX"] = ox + dx;
                        data::cfg["decoration"]["customBackgroundOffsetY"] = oy + dy;
                        data::save_config();
                        bg_drag_last_pos = sf::Mouse::getPosition(window);
                    }
                }
                break;

            case sf::Event::KeyPressed:
                // press ESC to close (important for borderless mode)
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                    break;
                }

                // get reload config prompt
                if (event.key.code == sf::Keyboard::R && event.key.control) {
                    if (!is_reload) {
                        while (!data::init()) {
                            continue;
                        }
                    }
                    is_reload = true;
                    break;
                }

                // switch mode dynamically
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num5 && event.key.control) {
                    int new_mode = event.key.code - sf::Keyboard::Num1 + 1;
                    data::cfg["mode"] = new_mode;
                    data::save_config();
                    while (!data::init()) { continue; }
                    break;
                }

                // toggle joystick debug panel
                if (event.key.code == sf::Keyboard::D && event.key.control) {
                    is_show_input_debug = !is_show_input_debug;
                    break;
                }

            default:
                is_reload = false;
            }
        }

        int mode = data::cfg["mode"].asInt();

        Json::Value rgb = data::cfg["decoration"]["rgb"];
        int red_value = rgb[0].asInt();
        int green_value = rgb[1].asInt();
        int blue_value = rgb[2].asInt();
        int alpha_value = rgb.size() == 3 ? 255 : rgb[3].asInt();

        window.clear(sf::Color(red_value, green_value, blue_value, alpha_value));
        
        if (data::has_custom_bg && !launcher::is_launcher) {
            sf::Sprite custom_bg_sprite(data::custom_bg_tex);
            int ox = data::cfg["decoration"]["customBackgroundOffsetX"].asInt();
            int oy = data::cfg["decoration"]["customBackgroundOffsetY"].asInt();
            double sc = data::cfg["decoration"].isMember("customBackgroundScale") ? data::cfg["decoration"]["customBackgroundScale"].asDouble() : 1.0;
            custom_bg_sprite.setPosition(ox, oy);
            custom_bg_sprite.setScale(sc, sc);
            window.draw(custom_bg_sprite);
        }
        
        if (launcher::is_launcher) {
            launcher::draw();
        } else {
            sf::View catView(sf::FloatRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
            double cScale = data::cfg["decoration"].isMember("catScale") ? data::cfg["decoration"]["catScale"].asDouble() : 1.0;
            double cOffX = data::cfg["decoration"].isMember("catOffsetX") ? data::cfg["decoration"]["catOffsetX"].asDouble() : 0.0;
            double cOffY = data::cfg["decoration"].isMember("catOffsetY") ? data::cfg["decoration"]["catOffsetY"].asDouble() : 0.0;
            catView.zoom(1.0 / cScale);
            catView.move(-cOffX, -cOffY);
            window.setView(catView);

            switch (mode) {
            case 1:
                osu::draw();
                break;
            case 2:
                taiko::draw();
                break;
            case 3:
                ctb::draw();
                break;
            case 4:
                mania::draw();
                break;
            case 5:
                custom::draw();
            }
            
            window.setView(sf::View(sf::FloatRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT)));
        }

        if (is_show_input_debug) {
            input::drawDebugPanel();
        }

        window.display();
    }

    input::cleanup();
    return 0;
}

