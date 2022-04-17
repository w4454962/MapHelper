#include "colored_cout.h"
#include "pugixml.hpp"
#include <stdafx.h>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#   define NOMINMAX // Fixes the conflicts with STL
#   include <Windows.h>
#   include <wincon.h>

uint16_t colored_cout_impl::getColorCode(const clr color) {
    switch (color) {
    case clr::grey:         return FOREGROUND_BLUE  | FOREGROUND_GREEN | FOREGROUND_RED;
    case clr::blue:         return FOREGROUND_BLUE  | FOREGROUND_INTENSITY;
    case clr::green:        return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case clr::cyan:         return FOREGROUND_BLUE  | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case clr::red:          return FOREGROUND_RED   | FOREGROUND_INTENSITY;
    case clr::magenta:      return FOREGROUND_BLUE  | FOREGROUND_RED   | FOREGROUND_INTENSITY;
    case clr::yellow:       return FOREGROUND_GREEN | FOREGROUND_RED   | FOREGROUND_INTENSITY;
    case clr::white:        return FOREGROUND_BLUE  | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
    case clr::on_blue:      return BACKGROUND_BLUE; //| BACKGROUND_INTENSITY
    case clr::on_red:       return BACKGROUND_RED;  //| BACKGROUND_INTENSITY
    case clr::on_magenta:   return BACKGROUND_BLUE  | BACKGROUND_RED;  //| BACKGROUND_INTENSITY
    case clr::on_grey:      return BACKGROUND_BLUE  | BACKGROUND_GREEN | BACKGROUND_RED;
    case clr::on_green:     return BACKGROUND_GREEN | BACKGROUND_INTENSITY;
    case clr::on_cyan:      return BACKGROUND_BLUE  | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
    case clr::on_yellow:    return BACKGROUND_GREEN | BACKGROUND_RED   | BACKGROUND_INTENSITY;
    case clr::on_white:     return BACKGROUND_BLUE  | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
    case clr::reset:
    default: break;
    }
    return static_cast<uint16_t>(clr::reset);
}

uint16_t colored_cout_impl::getConsoleTextAttr() {
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &buffer_info);
    return buffer_info.wAttributes;
}

void colored_cout_impl::setConsoleTextAttr(const uint16_t attr) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
}

#endif // _WIN32

#include <iostream>

const char* node_types[] =
{
    "null","document","element","pcdata","cdata","comment","pi","declaration"
};

struct parent_color_info {
    clr color, backdrop;
};

std::unordered_map<size_t, parent_color_info> parent_color_map;

struct simple_walker : pugi::xml_tree_walker {
    virtual bool for_each(pugi::xml_node& node) {
        
       // for (int i = 0; i < depth(); ++i) std::cout << "  "; // indentation
        auto type = node.type();
        switch (type) {
        case pugi::node_element:
        {
            clr color = clr::white;

            switch (hash_(node.name())) {
            case "grey"s_hash: color = clr::grey; break;
            case "blue"s_hash: color = clr::blue; break;
            case "green"s_hash: color = clr::green; break;
            case "cyan"s_hash: color = clr::cyan; break;
            case "red"s_hash: color = clr::red; break;
            case "magenta"s_hash: color = clr::magenta; break;
            case "yellow"s_hash: color = clr::yellow; break;
            case "white"s_hash: color = clr::white; break;
            case "reset"s_hash: color = clr::reset; break;
            default:
                break;
            }

            clr backdrop = clr::reset;
            switch (hash_(node.attribute("bd").as_string())) {
            case "blue"s_hash: backdrop = clr::on_blue; break;
            case "red"s_hash: backdrop = clr::on_red; break;
            case "magenta"s_hash: backdrop = clr::on_magenta; break;
            case "grey"s_hash: backdrop = clr::on_grey; break;
            case "green"s_hash: backdrop = clr::on_green; break;
            case "cyan"s_hash: backdrop = clr::on_cyan; break;
            case "yellow"s_hash: backdrop = clr::on_yellow; break;
            case "white"s_hash: backdrop = clr::on_white; break;
            case "reset"s_hash: color = clr::reset; break;
            default:
                break;
            }
            if (backdrop == clr::reset) {
                auto parent_hash = node.parent().hash_value();
                auto it = parent_color_map.find(parent_hash);
                if (it != parent_color_map.end()) {
                    auto& info = it->second;
                    backdrop = info.backdrop;
                }
            }
            parent_color_map[node.hash_value()] = { color, backdrop };
            std::cout << backdrop << color;
            
        }
            break;
        case pugi::node_pcdata:
        {
            std::cout << node.value();
            auto parent_hash = node.parent().parent().hash_value();
            auto it = parent_color_map.find(parent_hash);
            if (it != parent_color_map.end()) {
                auto& info = it->second;
                std::cout << info.backdrop;
                std::cout << info.color;
            } else {
                std::cout << clr::reset;
            }
        }
            break;
        default:
            break;
        }

        //std::cout << node_types[node.type()] << ": name='" << node.name() << "', value='"
        //    << node.value() << "', backdrop='" << node.attribute("backdrop").as_string() << "'\n";

        return true; // continue traversal
    }
};

void console_color_output(const std::string& xml) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(xml.data(), xml.length());
   
    if (result) {
        simple_walker walker;
        parent_color_map.clear();
        doc.traverse(walker);
        std::cout << std::endl;

    } else {
        std::cout << "error:" << result.description() << std::endl;;
        std::cout << xml << std::endl;
    }
    std::cout << clr::reset;

}