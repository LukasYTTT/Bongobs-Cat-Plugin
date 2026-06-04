#include "header.hpp"
#include <fstream>
#include <iostream>
#include <string>

namespace face {

struct Face {
    sf::Texture tex;
    bool active = false;
    sf::Keyboard::Key key;
};

std::map<sf::Keyboard::Key, Face> faces;

sf::Keyboard::Key parse_key(const std::string& key_str) {
    if (key_str == "f1") return sf::Keyboard::F1;
    if (key_str == "f2") return sf::Keyboard::F2;
    if (key_str == "f3") return sf::Keyboard::F3;
    if (key_str == "f4") return sf::Keyboard::F4;
    if (key_str == "f5") return sf::Keyboard::F5;
    if (key_str == "f6") return sf::Keyboard::F6;
    if (key_str == "f7") return sf::Keyboard::F7;
    if (key_str == "f8") return sf::Keyboard::F8;
    if (key_str == "f9") return sf::Keyboard::F9;
    if (key_str == "f10") return sf::Keyboard::F10;
    if (key_str == "f11") return sf::Keyboard::F11;
    if (key_str == "f12") return sf::Keyboard::F12;
    return sf::Keyboard::Unknown;
}

void init() {
    faces.clear();
    std::string face_dir = "img/face/";
    std::ifstream cfg_file(face_dir + "config.json", std::ifstream::binary);
    if (!cfg_file.good()) {
        std::cout << "No face config found at " << face_dir << "config.json" << std::endl;
        return;
    }

    std::string cfg_string((std::istreambuf_iterator<char>(cfg_file)), std::istreambuf_iterator<char>()), error;
    Json::CharReaderBuilder cfg_builder;
    Json::CharReader* cfg_reader = cfg_builder.newCharReader();
    Json::Value cfg_read;
    if (!cfg_reader->parse(cfg_string.c_str(), cfg_string.c_str() + cfg_string.size(), &cfg_read, &error)) {
        std::cerr << "Syntax error in face config.json" << std::endl;
        delete cfg_reader;
        return;
    }
    delete cfg_reader;

    if (cfg_read.isMember("HotKey") && cfg_read.isMember("FaceImageName")) {
        Json::Value hotkeys = cfg_read["HotKey"];
        Json::Value images = cfg_read["FaceImageName"];
        int count = std::min(hotkeys.size(), images.size());
        
        for (int i = 0; i < count; i++) {
            std::string key_str = hotkeys[i].asString();
            std::string img_str = images[i].asString();
            
            sf::Keyboard::Key key = parse_key(key_str);
            if (key != sf::Keyboard::Unknown) {
                Face f;
                f.key = key;
                f.active = false;
                if (f.tex.loadFromFile(face_dir + img_str)) {
                    faces[key] = f;
                } else {
                    std::cerr << "Failed to load face image: " << face_dir + img_str << std::endl;
                }
            }
        }
    }
}

void handle_key(sf::Keyboard::Key key) {
    if (faces.find(key) != faces.end()) {
        faces[key].active = !faces[key].active;
    }
}

void draw(sf::RenderWindow& window) {
    for (auto& pair : faces) {
        if (pair.second.active) {
            sf::Sprite sprite;
            sprite.setTexture(pair.second.tex);
            window.draw(sprite);
        }
    }
}

} // namespace face
