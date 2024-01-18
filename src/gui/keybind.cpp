#include <algorithm>
#include <string>
#include <unordered_map>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "gui/IconsForkAwesome.h"
#include "gui/input.hpp"
#include "gui/keybind.hpp"

namespace Toolbox::UI {

    static std::unordered_map<int, std::string> s_key_to_name = {
        {GLFW_KEY_UNKNOWN,       "Unknown"          },
        {GLFW_KEY_SPACE,         "Space"            },
        {GLFW_KEY_APOSTROPHE,    "'"                },
        {GLFW_KEY_COMMA,         ","                },
        {GLFW_KEY_MINUS,         "-"                },
        {GLFW_KEY_PERIOD,        "."                },
        {GLFW_KEY_SLASH,         "/"                },
        {GLFW_KEY_0,             "0"                },
        {GLFW_KEY_1,             "1"                },
        {GLFW_KEY_2,             "2"                },
        {GLFW_KEY_3,             "3"                },
        {GLFW_KEY_4,             "4"                },
        {GLFW_KEY_5,             "5"                },
        {GLFW_KEY_6,             "6"                },
        {GLFW_KEY_7,             "7"                },
        {GLFW_KEY_8,             "8"                },
        {GLFW_KEY_9,             "9"                },
        {GLFW_KEY_SEMICOLON,     ";"                },
        {GLFW_KEY_EQUAL,         "="                },
        {GLFW_KEY_A,             "A"                },
        {GLFW_KEY_B,             "B"                },
        {GLFW_KEY_C,             "C"                },
        {GLFW_KEY_D,             "D"                },
        {GLFW_KEY_E,             "E"                },
        {GLFW_KEY_F,             "F"                },
        {GLFW_KEY_G,             "G"                },
        {GLFW_KEY_H,             "H"                },
        {GLFW_KEY_I,             "I"                },
        {GLFW_KEY_J,             "J"                },
        {GLFW_KEY_K,             "K"                },
        {GLFW_KEY_L,             "L"                },
        {GLFW_KEY_M,             "M"                },
        {GLFW_KEY_N,             "N"                },
        {GLFW_KEY_O,             "O"                },
        {GLFW_KEY_P,             "P"                },
        {GLFW_KEY_Q,             "Q"                },
        {GLFW_KEY_R,             "R"                },
        {GLFW_KEY_S,             "S"                },
        {GLFW_KEY_T,             "T"                },
        {GLFW_KEY_U,             "U"                },
        {GLFW_KEY_V,             "V"                },
        {GLFW_KEY_W,             "W"                },
        {GLFW_KEY_X,             "X"                },
        {GLFW_KEY_Y,             "Y"                },
        {GLFW_KEY_Z,             "Z"                },
        {GLFW_KEY_LEFT_BRACKET,  "["                },
        {GLFW_KEY_BACKSLASH,     "\\"               },
        {GLFW_KEY_RIGHT_BRACKET, "]"                },
        {GLFW_KEY_GRAVE_ACCENT,  "`"                },
        {GLFW_KEY_WORLD_1,       "World1"           }, /* non-US #1 */
        {GLFW_KEY_WORLD_2,       "World2"           }, /* non-US #2 */
        {GLFW_KEY_ESCAPE,        "Esc"              },
        {GLFW_KEY_ENTER,         "Enter"            },
        {GLFW_KEY_TAB,           "Tab"              },
        {GLFW_KEY_BACKSPACE,     "Backspace"        },
        {GLFW_KEY_INSERT,        "Ins"              },
        {GLFW_KEY_DELETE,        "Del"              },
        {GLFW_KEY_RIGHT,         ICON_FK_ARROW_RIGHT},
        {GLFW_KEY_LEFT,          ICON_FK_ARROW_LEFT },
        {GLFW_KEY_DOWN,          ICON_FK_ARROW_DOWN },
        {GLFW_KEY_UP,            ICON_FK_ARROW_UP   },
        {GLFW_KEY_PAGE_UP,       "PgUp"             },
        {GLFW_KEY_PAGE_DOWN,     "PgDn"             },
        {GLFW_KEY_HOME,          "Home"             },
        {GLFW_KEY_END,           "End"              },
        {GLFW_KEY_CAPS_LOCK,     "CapsLk"           },
        {GLFW_KEY_SCROLL_LOCK,   "ScrollLk"         },
        {GLFW_KEY_NUM_LOCK,      "NumLk"            },
        {GLFW_KEY_PRINT_SCREEN,  "PrintScrn"        },
        {GLFW_KEY_PAUSE,         "Pause"            },
        {GLFW_KEY_F1,            "F1"               },
        {GLFW_KEY_F2,            "F2"               },
        {GLFW_KEY_F3,            "F3"               },
        {GLFW_KEY_F4,            "F4"               },
        {GLFW_KEY_F5,            "F5"               },
        {GLFW_KEY_F6,            "F6"               },
        {GLFW_KEY_F7,            "F7"               },
        {GLFW_KEY_F8,            "F8"               },
        {GLFW_KEY_F9,            "F9"               },
        {GLFW_KEY_F10,           "F10"              },
        {GLFW_KEY_F11,           "F11"              },
        {GLFW_KEY_F12,           "F12"              },
        {GLFW_KEY_F13,           "F13"              },
        {GLFW_KEY_F14,           "F14"              },
        {GLFW_KEY_F15,           "F15"              },
        {GLFW_KEY_F16,           "F16"              },
        {GLFW_KEY_F17,           "F17"              },
        {GLFW_KEY_F18,           "F18"              },
        {GLFW_KEY_F19,           "F19"              },
        {GLFW_KEY_F20,           "F20"              },
        {GLFW_KEY_F21,           "F21"              },
        {GLFW_KEY_F22,           "F22"              },
        {GLFW_KEY_F23,           "F23"              },
        {GLFW_KEY_F24,           "F24"              },
        {GLFW_KEY_F25,           "F25"              },
        {GLFW_KEY_KP_0,          "KP0"              },
        {GLFW_KEY_KP_1,          "KP1"              },
        {GLFW_KEY_KP_2,          "KP2"              },
        {GLFW_KEY_KP_3,          "KP3"              },
        {GLFW_KEY_KP_4,          "KP4"              },
        {GLFW_KEY_KP_5,          "KP5"              },
        {GLFW_KEY_KP_6,          "KP6"              },
        {GLFW_KEY_KP_7,          "KP7"              },
        {GLFW_KEY_KP_8,          "KP8"              },
        {GLFW_KEY_KP_DECIMAL,    "KPDec"            },
        {GLFW_KEY_KP_DIVIDE,     "KPDiv"            },
        {GLFW_KEY_KP_MULTIPLY,   "KPMul"            },
        {GLFW_KEY_KP_SUBTRACT,   "KPSub"            },
        {GLFW_KEY_KP_ADD,        "KPAdd"            },
        {GLFW_KEY_KP_ENTER,      "KPEnter"          },
        {GLFW_KEY_KP_EQUAL,      "KPEqu"            },
        {GLFW_KEY_LEFT_SHIFT,    "Shift"            },
        {GLFW_KEY_LEFT_CONTROL,  "Ctrl"             },
        {GLFW_KEY_LEFT_ALT,      "Alt"              },
        {GLFW_KEY_LEFT_SUPER,    "Super"            },
        {GLFW_KEY_RIGHT_SHIFT,   "ShiftR"           },
        {GLFW_KEY_RIGHT_CONTROL, "CtrlR"            },
        {GLFW_KEY_RIGHT_ALT,     "AltR"             },
        {GLFW_KEY_RIGHT_SUPER,   "SuperR"           },
        {GLFW_KEY_MENU,          "Menu"             },
    };

    static std::unordered_map<std::string, int> s_name_to_key = {
        {"Unknown",           GLFW_KEY_UNKNOWN      },
        {"Space",             GLFW_KEY_SPACE        },
        {"'",                 GLFW_KEY_APOSTROPHE   },
        {",",                 GLFW_KEY_COMMA        },
        {"-",                 GLFW_KEY_MINUS        },
        {".",                 GLFW_KEY_PERIOD       },
        {"/",                 GLFW_KEY_SLASH        },
        {"0",                 GLFW_KEY_0            },
        {"1",                 GLFW_KEY_1            },
        {"2",                 GLFW_KEY_2            },
        {"3",                 GLFW_KEY_3            },
        {"4",                 GLFW_KEY_4            },
        {"5",                 GLFW_KEY_5            },
        {"6",                 GLFW_KEY_6            },
        {"7",                 GLFW_KEY_7            },
        {"8",                 GLFW_KEY_8            },
        {"9",                 GLFW_KEY_9            },
        {";",                 GLFW_KEY_SEMICOLON    },
        {"=",                 GLFW_KEY_EQUAL        },
        {"A",                 GLFW_KEY_A            },
        {"B",                 GLFW_KEY_B            },
        {"C",                 GLFW_KEY_C            },
        {"D",                 GLFW_KEY_D            },
        {"E",                 GLFW_KEY_E            },
        {"F",                 GLFW_KEY_F            },
        {"G",                 GLFW_KEY_G            },
        {"H",                 GLFW_KEY_H            },
        {"I",                 GLFW_KEY_I            },
        {"J",                 GLFW_KEY_J            },
        {"K",                 GLFW_KEY_K            },
        {"L",                 GLFW_KEY_L            },
        {"M",                 GLFW_KEY_M            },
        {"N",                 GLFW_KEY_N            },
        {"O",                 GLFW_KEY_O            },
        {"P",                 GLFW_KEY_P            },
        {"Q",                 GLFW_KEY_Q            },
        {"R",                 GLFW_KEY_R            },
        {"S",                 GLFW_KEY_S            },
        {"T",                 GLFW_KEY_T            },
        {"U",                 GLFW_KEY_U            },
        {"V",                 GLFW_KEY_V            },
        {"W",                 GLFW_KEY_W            },
        {"X",                 GLFW_KEY_X            },
        {"Y",                 GLFW_KEY_Y            },
        {"Z",                 GLFW_KEY_Z            },
        {"[",                 GLFW_KEY_LEFT_BRACKET },
        {"\\",                GLFW_KEY_BACKSLASH    },
        {"]",                 GLFW_KEY_RIGHT_BRACKET},
        {"`",                 GLFW_KEY_GRAVE_ACCENT },
        {"World1",            GLFW_KEY_WORLD_1      }, /* non-US #1 */
        {"World2",            GLFW_KEY_WORLD_2      }, /* non-US #2 */
        {"Esc",               GLFW_KEY_ESCAPE       },
        {"Enter",             GLFW_KEY_ENTER        },
        {"Tab",               GLFW_KEY_TAB          },
        {"Backspace",         GLFW_KEY_BACKSPACE    },
        {"Ins",               GLFW_KEY_INSERT       },
        {"Del",               GLFW_KEY_DELETE       },
        {ICON_FK_ARROW_RIGHT, GLFW_KEY_RIGHT        },
        {ICON_FK_ARROW_LEFT,  GLFW_KEY_LEFT         },
        {ICON_FK_ARROW_DOWN,  GLFW_KEY_DOWN         },
        {ICON_FK_ARROW_UP,    GLFW_KEY_UP           },
        {"PgUp",              GLFW_KEY_PAGE_UP      },
        {"PgDn",              GLFW_KEY_PAGE_DOWN    },
        {"Home",              GLFW_KEY_HOME         },
        {"End",               GLFW_KEY_END          },
        {"CapsLk",            GLFW_KEY_CAPS_LOCK    },
        {"ScrollLk",          GLFW_KEY_SCROLL_LOCK  },
        {"NumLk",             GLFW_KEY_NUM_LOCK     },
        {"PrintScrn",         GLFW_KEY_PRINT_SCREEN },
        {"Pause",             GLFW_KEY_PAUSE        },
        {"F1",                GLFW_KEY_F1           },
        {"F2",                GLFW_KEY_F2           },
        {"F3",                GLFW_KEY_F3           },
        {"F4",                GLFW_KEY_F4           },
        {"F5",                GLFW_KEY_F5           },
        {"F6",                GLFW_KEY_F6           },
        {"F7",                GLFW_KEY_F7           },
        {"F8",                GLFW_KEY_F8           },
        {"F9",                GLFW_KEY_F9           },
        {"F10",               GLFW_KEY_F10          },
        {"F11",               GLFW_KEY_F11          },
        {"F12",               GLFW_KEY_F12          },
        {"F13",               GLFW_KEY_F13          },
        {"F14",               GLFW_KEY_F14          },
        {"F15",               GLFW_KEY_F15          },
        {"F16",               GLFW_KEY_F16          },
        {"F17",               GLFW_KEY_F17          },
        {"F18",               GLFW_KEY_F18          },
        {"F19",               GLFW_KEY_F19          },
        {"F20",               GLFW_KEY_F20          },
        {"F21",               GLFW_KEY_F21          },
        {"F22",               GLFW_KEY_F22          },
        {"F23",               GLFW_KEY_F23          },
        {"F24",               GLFW_KEY_F24          },
        {"F25",               GLFW_KEY_F25          },
        {"KP0",               GLFW_KEY_KP_0         },
        {"KP1",               GLFW_KEY_KP_1         },
        {"KP2",               GLFW_KEY_KP_2         },
        {"KP3",               GLFW_KEY_KP_3         },
        {"KP4",               GLFW_KEY_KP_4         },
        {"KP5",               GLFW_KEY_KP_5         },
        {"KP6",               GLFW_KEY_KP_6         },
        {"KP7",               GLFW_KEY_KP_7         },
        {"KP8",               GLFW_KEY_KP_8         },
        {"KPDec",             GLFW_KEY_KP_DECIMAL   },
        {"KPDiv",             GLFW_KEY_KP_DIVIDE    },
        {"KPMul",             GLFW_KEY_KP_MULTIPLY  },
        {"KPSub",             GLFW_KEY_KP_SUBTRACT  },
        {"KPAdd",             GLFW_KEY_KP_ADD       },
        {"KPEnter",           GLFW_KEY_KP_ENTER     },
        {"KPEqu",             GLFW_KEY_KP_EQUAL     },
        {"Shift",             GLFW_KEY_LEFT_SHIFT   },
        {"Ctrl",              GLFW_KEY_LEFT_CONTROL },
        {"Alt",               GLFW_KEY_LEFT_ALT     },
        {"Super",             GLFW_KEY_LEFT_SUPER   },
        {"ShiftR",            GLFW_KEY_RIGHT_SHIFT  },
        {"CtrlR",             GLFW_KEY_RIGHT_CONTROL},
        {"AltR",              GLFW_KEY_RIGHT_ALT    },
        {"SuperR",            GLFW_KEY_RIGHT_SUPER  },
        {"Menu",              GLFW_KEY_MENU         },
    };

    bool KeyBindHeld(const std::vector<int> &keybind) {
        return std::all_of(keybind.begin(), keybind.end(),
                           [](int keybind) { return Input::GetKey(keybind); });
    }

    bool KeyBindScanInput(std::vector<int> &current_keybind) {
        // Check if any of the keys are still being held
        bool any_keys_held = std::any_of(current_keybind.begin(), current_keybind.end(),
                                          [](int key) { return Input::GetKey(key); });
        if (current_keybind.size() > 0 && !any_keys_held) {
            return true;
        }
        // Check if any new keys have been pressed
        for (int key = 0; key < 512; ++key) {
            bool is_already_held = std::any_of(current_keybind.begin(), current_keybind.end(),
                                               [&](int our_key) { return our_key == key; });
            if (!is_already_held && Input::GetKey(key)) {
                current_keybind.push_back(key);
            }
        }
        // Force a max length of 3 keys
        return current_keybind.size() >= 3;
    }

    std::string KeyNameFromEnum(int key) { return s_key_to_name[key]; }
    int KeyNameToEnum(const std::string &key_name) { return s_name_to_key[key_name]; }

}  // namespace Toolbox::UI