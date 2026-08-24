#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>
#include <memory>
#include <algorithm>
#include <unistd.h>
#include <limits.h>

class in_game_terminal {
public:
    bool is_open = false;
    sf::Font font;
    bool has_font = false;

    std::vector<std::string> output_lines;
    std::string current_input;
    std::vector<std::string> history;
    int history_index = -1;

    std::string current_dir;
    float cursor_blink_timer = 0.0f;
    bool show_cursor = true;
    int scroll_offset = 0;

    in_game_terminal() {
        std::vector<std::string> font_candidates = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/chromeos/noto/NotoSansMono-Regular.ttf",
            "/usr/share/fonts/chromeos/roboto/Roboto-Regular.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
        };

        for (const auto& path : font_candidates) {
            if (font.loadFromFile(path)) {
                has_font = true;
                break;
            }
        }

        update_current_dir();

        output_lines.push_back("==================================================================");
        output_lines.push_back("   KIRBY WORKSTATION TERMINAL v1.0 [BASH SHELL]");
        output_lines.push_back("   Direct subshell execution in current project workspace");
        output_lines.push_back("   Commands: ls, pwd, git, make, cat, echo, python3, etc.");
        output_lines.push_back("   Type 'clear' to clear, 'exit' or press ESC to exit terminal");
        output_lines.push_back("==================================================================");
        output_lines.push_back("");
    }

    void update_current_dir() {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            current_dir = cwd;
        } else {
            current_dir = ".";
        }
    }

    std::string get_prompt() const {
        return "kirby@ps1-host:" + current_dir + "$ ";
    }

    void open() {
        is_open = true;
        update_current_dir();
        current_input.clear();
        scroll_offset = 0;
    }

    void close() {
        is_open = false;
    }

    void execute_command(const std::string& cmd) {
        if (cmd.empty()) {
            output_lines.push_back(get_prompt());
            return;
        }

        history.push_back(cmd);
        history_index = -1;

        output_lines.push_back(get_prompt() + cmd);

        if (cmd == "exit" || cmd == "quit") {
            close();
            return;
        }
        if (cmd == "clear" || cmd == "cls") {
            output_lines.clear();
            return;
        }
        if (cmd.rfind("cd ", 0) == 0 || cmd == "cd") {
            std::string path = (cmd == "cd") ? getenv("HOME") : cmd.substr(3);
            while (!path.empty() && path[0] == ' ') path.erase(0, 1);
            if (chdir(path.c_str()) == 0) {
                update_current_dir();
            } else {
                output_lines.push_back("cd: no such file or directory: " + path);
            }
            return;
        }

        // Execute command in current working directory and capture output
        std::string full_cmd = cmd + " 2>&1";
        FILE* pipe = popen(full_cmd.c_str(), "r");
        if (!pipe) {
            output_lines.push_back("Error: Failed to execute command");
            return;
        }

        char buffer[512];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string line(buffer);
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
                line.pop_back();
            }
            output_lines.push_back(line);
        }
        pclose(pipe);

        // Keep last 1000 lines
        if (output_lines.size() > 1000) {
            output_lines.erase(output_lines.begin(), output_lines.begin() + (output_lines.size() - 1000));
        }

        scroll_offset = 0;
    }

    void handle_event(const sf::Event& event) {
        if (!is_open) return;

        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                close();
                return;
            }
            if (event.key.code == sf::Keyboard::Return) {
                std::string cmd = current_input;
                current_input.clear();
                execute_command(cmd);
                return;
            }
            if (event.key.code == sf::Keyboard::Up) {
                if (!history.empty()) {
                    if (history_index == -1) history_index = (int)history.size() - 1;
                    else if (history_index > 0) history_index--;
                    current_input = history[history_index];
                }
                return;
            }
            if (event.key.code == sf::Keyboard::Down) {
                if (!history.empty() && history_index != -1) {
                    if (history_index < (int)history.size() - 1) {
                        history_index++;
                        current_input = history[history_index];
                    } else {
                        history_index = -1;
                        current_input.clear();
                    }
                }
                return;
            }
            if (event.key.code == sf::Keyboard::PageUp) {
                scroll_offset += 5;
                return;
            }
            if (event.key.code == sf::Keyboard::PageDown) {
                scroll_offset = std::max(0, scroll_offset - 5);
                return;
            }
        }

        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '\b') {
                if (!current_input.empty()) {
                    current_input.pop_back();
                }
            } else if (event.text.unicode == 13 || event.text.unicode == 10) {
                // Enter handled in KeyPressed
            } else if (event.text.unicode == 27) {
                // Escape handled in KeyPressed
            } else if (event.text.unicode >= 32 && event.text.unicode < 127) {
                current_input += static_cast<char>(event.text.unicode);
            }
        }
    }

    void draw(sf::RenderTarget& target) {
        if (!is_open) return;

        cursor_blink_timer += 0.04f;
        if (cursor_blink_timer >= 1.0f) {
            cursor_blink_timer = 0.0f;
            show_cursor = !show_cursor;
        }

        sf::Vector2u win_size = target.getSize();
        float margin_x = 30.0f;
        float margin_y = 30.0f;
        float term_w = (float)win_size.x - margin_x * 2.0f;
        float term_h = (float)win_size.y - margin_y * 2.0f;

        // Semi-transparent dark CRT terminal background
        sf::RectangleShape bg(sf::Vector2f(term_w, term_h));
        bg.setPosition(margin_x, margin_y);
        bg.setFillColor(sf::Color(10, 20, 12, 240));
        bg.setOutlineThickness(3.0f);
        bg.setOutlineColor(sf::Color(50, 255, 100, 230));
        target.draw(bg);

        // Header bar
        sf::RectangleShape header(sf::Vector2f(term_w, 28.0f));
        header.setPosition(margin_x, margin_y);
        header.setFillColor(sf::Color(20, 50, 25, 255));
        target.draw(header);

        if (has_font) {
            sf::Text title_text("KIRBY OS v1.0 -- RETRO BASH SHELL TERMINAL  [Press ESC to Close]", font, 13);
            title_text.setPosition(margin_x + 10.0f, margin_y + 5.0f);
            title_text.setFillColor(sf::Color(100, 255, 150));
            target.draw(title_text);

            const unsigned int char_size = 13;
            const float line_spacing = 18.0f;
            int max_visible_lines = (int)((term_h - 65.0f) / line_spacing);

            int total_lines = (int)output_lines.size() + 1;
            int start_idx = std::max(0, total_lines - max_visible_lines - scroll_offset);
            int end_idx = std::min((int)output_lines.size(), start_idx + max_visible_lines);

            float cur_y = margin_y + 36.0f;

            for (int i = start_idx; i < end_idx; ++i) {
                sf::Text line_text(output_lines[i], font, char_size);
                line_text.setPosition(margin_x + 12.0f, cur_y);
                line_text.setFillColor(sf::Color(60, 245, 100));
                target.draw(line_text);
                cur_y += line_spacing;
            }

            // Current input prompt line
            if (start_idx + max_visible_lines >= (int)output_lines.size()) {
                std::string input_display = get_prompt() + current_input + (show_cursor ? "_" : " ");
                sf::Text prompt_text(input_display, font, char_size);
                prompt_text.setPosition(margin_x + 12.0f, cur_y);
                prompt_text.setFillColor(sf::Color(130, 255, 150));
                target.draw(prompt_text);
            }
        }
    }
};
