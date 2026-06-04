#include "header.hpp"

namespace taiko {
Json::Value rim_key_value[2], centre_key_value[2];
sf::Sprite bg, up[2], rim[2], centre[2];

int key_state[2] = {0, 0};
bool rim_key_state[2] = {false, false};
bool centre_key_state[2] = {false, false};
double timer_rim_key[2] = {-1, -1};
double timer_centre_key[2] = {-1, -1};

bool init() {
    // getting configs
    bool chk[256];
    std::fill(chk, chk + 256, false);
    Json::Value taiko = data::cfg["taiko"];

    rim_key_value[0] = taiko["leftRim"];
    for (Json::Value &v : rim_key_value[0]) {
        chk[v.asInt()] = true;
    }
    centre_key_value[0] = taiko["leftCentre"];
    for (Json::Value &v : centre_key_value[0]) {
        if (chk[v.asInt()]) {
            data::error_msg("Overlapping osu!taiko keybinds", "Error reading configs");
            return false;
        }
    }

    std::fill(chk, chk + 256, false);
    rim_key_value[1] = taiko["rightRim"];
    for (Json::Value &v : rim_key_value[1]) {
        chk[v.asInt()] = true;
    }
    centre_key_value[1] = taiko["rightCentre"];
    for (Json::Value &v : centre_key_value[1]) {
        if (chk[v.asInt()]) {
            data::error_msg("Overlapping osu!taiko keybinds", "Error reading configs");
            return false;
        }
    }

    // importing sprites
    bg.setTexture(data::load_texture("img/taiko/bg.png"));
    up[0].setTexture(data::load_texture("img/taiko/leftup.png"));
    rim[0].setTexture(data::load_texture("img/taiko/leftrim.png"));
    centre[0].setTexture(data::load_texture("img/taiko/leftcentre.png"));
    up[1].setTexture(data::load_texture("img/taiko/rightup.png"));
    rim[1].setTexture(data::load_texture("img/taiko/rightrim.png"));
    centre[1].setTexture(data::load_texture("img/taiko/rightcentre.png"));

    return true;
}

void draw() {
    window.draw(bg);

    bool any_active = false;
    int current_drum = -1;

    if (data::cfg["taiko"].isMember("anyKey") && data::cfg["taiko"]["anyKey"].asBool()) {
        static bool last_any = false;
        bool any = input::is_any_key_pressed();
        static int taiko_step = 0;
        
        if (any && !last_any) {
            taiko_step = (taiko_step + 1) % 4;
        }
        last_any = any;
        
        if (any) {
            any_active = true;
            current_drum = taiko_step;
        }
    }

    // 0 for left side, 1 for right side
    for (int i = 0; i < 2; i++) {
        bool rim_key = false;
        if (any_active) {
            if (i == 0 && current_drum == 2) rim_key = true;
            if (i == 1 && current_drum == 3) rim_key = true;
        } else {
            for (Json::Value &v : rim_key_value[i]) {
                if (input::is_pressed(v.asInt())) {
                    rim_key = true;
                    break;
                }
            }
        }
        if (rim_key) {
            if (!rim_key_state[i]) {
                key_state[i] = 1;
                rim_key_state[i] = true;
            }
        } else {
            rim_key_state[i] = false;
        }

        bool centre_key = false;
        if (any_active) {
            if (i == 0 && current_drum == 0) centre_key = true;
            if (i == 1 && current_drum == 1) centre_key = true;
        } else {
            for (Json::Value &v : centre_key_value[i]) {
                if (input::is_pressed(v.asInt())) {
                    centre_key = true;
                    break;
                }
            }
        }
        if (centre_key) {
            if (!centre_key_state[i]) {
                key_state[i] = 2;
                centre_key_state[i] = true;
            }
        } else {
            centre_key_state[i] = false;
        }

        if (!rim_key_state[i] && !centre_key_state[i]) {
            key_state[i] = 0;
            window.draw(up[i]);
        }
        if (key_state[i] == 1) {
            if ((clock() - timer_centre_key[i]) / CLOCKS_PER_SEC > BONGO_KEYPRESS_THRESHOLD) {
                window.draw(rim[i]);
                timer_rim_key[i] = clock();
            } else {
                window.draw(up[i]);
            }
        } else if (key_state[i] == 2) {
            if ((clock() - timer_rim_key[i]) / CLOCKS_PER_SEC > BONGO_KEYPRESS_THRESHOLD) {
                window.draw(centre[i]);
                timer_centre_key[i] = clock();
            } else {
                window.draw(up[i]);
            }
        }
    }
}
}; // namespace taiko
