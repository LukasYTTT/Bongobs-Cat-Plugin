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
int main(int argc, char ** argv) {
    // Automatically change working directory to the parent of the 'bin' folder
    char exe_path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", exe_path, PATH_MAX);
    if (count != -1) {
        exe_path[count] = '\0';
        char* dir = dirname(exe_path); // gets the directory containing the executable (e.g. .../bin)
        // If we want the parent directory (root of the bongocat project), we go one level up
        // Note: we just assume the executable is in bin/ or root. Let's try changing to dir and then if "bin" is in the path, go up.
        // Even simpler: the structure is always <dir>/bin/bongo. So `dirname(dir)` gives us the project root.
        char* parent_dir = dirname(dir);
        chdir(parent_dir);
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

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;

            case sf::Event::KeyPressed:
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

