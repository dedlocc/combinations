#include "Component.h"

#include <iomanip>

Component Component::from_stream(std::istream & strm)
{
    Component component;

    char type = '\0';
    strm >> type;
    if (strm.fail()) {
        return {};
    }
    bool read_strike = false;
    component.type = static_cast<InstrumentType>(type);
    switch (component.type) {
    case InstrumentType::C: [[fallthrough]];
    case InstrumentType::O: [[fallthrough]];
    case InstrumentType::P:
        read_strike = true;
        break;
    case InstrumentType::F: [[fallthrough]];
    case InstrumentType::U:
        break;
    case InstrumentType::Unknown: [[fallthrough]];
    default:
        return {};
    }

    strm >> component.ratio;
    if (strm.fail()) {
        return {};
    }

    if (read_strike) {
        strm >> component.strike;
        if (strm.fail()) {
            return {};
        }
    }

    std::tm tm;
    strm >> std::get_time(&tm, "%Y-%m-%d");
    if (strm.fail()) {
        return {};
    }

    component.expiration = tm;

    return component;
}

Component Component::from_string(const std::string & str)
{
    std::istringstream strm{str};
    return from_stream(strm);
}
