#include "header.hpp"

namespace launcher {
sf::Font font;
int selected_mode = 1;
bool any_key_enabled = false;
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
    window.clear(sf::Color(30, 30, 40));

    draw_text("Bongo Cat Launcher", 50, 30, 36, sf::Color::White);
    draw_text("Select Mode:", 50, 90, 20, sf::Color(200, 200, 200));

    std::string modes[] = {"osu! (Mouse/Tablet)", "Taiko (Drums)", "Catch", "Mania", "Custom"};
    
    auto mouse_pos = sf::Mouse::getPosition(window);
    static bool last_mouse = false;
    bool current_mouse = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    bool clicked = current_mouse && !last_mouse;
    last_mouse = current_mouse;

    for (int i = 0; i < 5; i++) {
        sf::RectangleShape btn(sf::Vector2f(250, 40));
        btn.setPosition(50, 130 + i * 50);
        
        bool hovered = btn.getGlobalBounds().contains(mouse_pos.x, mouse_pos.y);
        if (hovered && clicked) {
            selected_mode = i + 1;
        }

        if (selected_mode == i + 1) {
            btn.setFillColor(sf::Color(100, 150, 255)); // Active
        } else if (hovered) {
            btn.setFillColor(sf::Color(60, 60, 80)); // Hover
        } else {
            btn.setFillColor(sf::Color(40, 40, 60)); // Idle
        }

        window.draw(btn);
        draw_text(modes[i], 60, 135 + i * 50, 18, sf::Color::White);
    }

    // Draw Preview Image
    std::string preview_paths[] = {
        "img/osu/mousebg.png",
        "img/taiko/bg.png",
        "img/ctb/bg.png",
        "img/mania/bg4k.png",
        "img/osu/mousebg.png" // fallback for custom
    };
    
    if (selected_mode >= 1 && selected_mode <= 5) {
        sf::Sprite preview;
        preview.setTexture(data::load_texture(preview_paths[selected_mode - 1]));
        
        // Scale down to fit in the right side
        // original width is 612x354 usually
        preview.setPosition(350, 180);
        preview.setScale(0.5f, 0.5f);
        window.draw(preview);
    }

    // Any Key Checkbox
    sf::RectangleShape checkbox(sf::Vector2f(24, 24));
    checkbox.setPosition(350, 135);
    bool cb_hovered = checkbox.getGlobalBounds().contains(mouse_pos.x, mouse_pos.y);
    if (cb_hovered && clicked) {
        any_key_enabled = !any_key_enabled;
    }
    checkbox.setFillColor(any_key_enabled ? sf::Color(100, 255, 100) : sf::Color(60, 60, 60));
    window.draw(checkbox);
    draw_text("\"Any Key\" Feature (Type anywhere)", 385, 135, 18, sf::Color::White);

    // Start Button
    sf::RectangleShape startBtn(sf::Vector2f(150, 50));
    startBtn.setPosition(350, 370);
    bool start_hovered = startBtn.getGlobalBounds().contains(mouse_pos.x, mouse_pos.y);
    startBtn.setFillColor(start_hovered ? sf::Color(50, 200, 50) : sf::Color(30, 150, 30));
    
    if (start_hovered && clicked) {
        data::cfg["mode"] = selected_mode;
        data::cfg["osu"]["anyKey"] = any_key_enabled;
        data::cfg["taiko"]["anyKey"] = any_key_enabled;
        data::save_config();
        
        is_launcher = false; // Exit launcher
        while (!data::init()) { continue; } // Load assets for the new mode
    }
    
    window.draw(startBtn);
    draw_text("START", 385, 380, 24, sf::Color::White);
}
}; // namespace launcher
