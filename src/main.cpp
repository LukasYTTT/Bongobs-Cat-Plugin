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

int main(int argc, char ** argv) {
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
                
                std::string env_disp = disp ? std::string("DISPLAY=") + disp : "DISPLAY=:0";
                std::string env_wayl = wayl ? std::string("WAYLAND_DISPLAY=") + wayl : "WAYLAND_DISPLAY=wayland-0";
                std::string env_xauth = xauth ? std::string("XAUTHORITY=") + xauth : "";

                if (xauth) {
                    execlp("pkexec", "pkexec", "env", env_disp.c_str(), env_wayl.c_str(), env_xauth.c_str(), target_exe, NULL);
                } else {
                    execlp("pkexec", "pkexec", "env", env_disp.c_str(), env_wayl.c_str(), target_exe, NULL);
                }
                // If execlp returns, pkexec failed or was cancelled by user, so we continue with X11 fallback
            }
        }
    }

#else
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#endif

    window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bongo Cat for osu!", sf::Style::Titlebar | sf::Style::Close);
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

    bool is_reload = false;
    bool is_show_input_debug = false;
    bool was_launcher = true;

    while (window.isOpen()) {
        if (was_launcher && !launcher::is_launcher) {
            was_launcher = false;
            // Wenn Green Screen aktiv ist (wird typischerweise für OBS genutzt), automatisch den Rahmen entfernen!
            bool is_green = (data::cfg["decoration"]["rgb"][0].asInt() == 0 && data::cfg["decoration"]["rgb"][1].asInt() == 255);
            if (is_green) {
                window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bongo Cat for osu!", sf::Style::None);
                window.setFramerateLimit(MAX_FRAMERATE);
            }
        }

        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
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
        
        if (launcher::is_launcher) {
            launcher::draw();
        } else {
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
        }

        if (is_show_input_debug) {
            input::drawDebugPanel();
        }

        window.display();
    }

    input::cleanup();
    return 0;
}

