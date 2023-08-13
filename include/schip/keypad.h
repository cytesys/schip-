#pragma once

#ifndef KEYPAD_H
#define KEYPAD_H

#include <cstdint>
#include <atomic>
#include <thread>
#include <string_view>
#include <array>

class KeyPad {
public:
    static KeyPad& get_instance() {
        static KeyPad instance;
        return instance;
    }

    void press_key(unsigned char key) { set_key(key, true); }

    void release_key(unsigned char key) { set_key(key, false); }

    bool is_pressed(uint8_t key) {
        return m_key.load() == key;
    }

    uint8_t wait_keypress() {
        int k;

        for (k = NO_KEY; k < 0; k = m_key.load())
            std::this_thread::yield();

        m_key.store(NO_KEY);

        return k;
    }

    int get_key() { return m_key.load(); }

    static constexpr int NO_KEY = -1;

private:
    static constexpr std::string_view m_keymap{"x123qweasdzc4rfv"};

    std::atomic<int> m_key{NO_KEY};

    KeyPad() {}

    void set_key(unsigned char key, bool pressed) {
        int index = m_keymap.find_first_of(key);
        int keyval = (pressed) ? index : NO_KEY;

        if (index != std::string_view::npos && m_key.load() != keyval) {
            m_key.store(keyval);
        }
    }
};

#endif
