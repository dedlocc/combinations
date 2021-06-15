#include "Combinations.h"

#include "pugixml.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <type_traits>
#include <unordered_set>
#include <variant>

namespace {
struct Leg
{
    InstrumentType type;
    std::variant<double, bool> ratio;
    std::variant<char, int> strike;
    std::variant<char, int, Duration> expiration;
};

struct Combination
{
    const std::string name;

    Combination(std::string name)
        : name(std::move(name))
    {
    }

    virtual ~Combination() = default;

    virtual bool match(const std::vector<Component> & components, std::vector<int> & order) const = 0;
};

struct MultipleCombination : Combination
{
    const std::vector<Leg> legs;

    MultipleCombination(std::string name, std::vector<Leg> legs)
        : Combination(std::move(name))
        , legs(std::move(legs))
    {
    }

    bool match(const std::vector<Component> & components, std::vector<int> & order) const override
    {
        if (!preCheck(components)) {
            return false;
        }

        std::iota(order.begin(), order.end(), 0);

        do {
            if (matchOrdered(components, order)) {
                return true;
            }
        } while (std::next_permutation(order.begin(), order.end()));

        return false;
    }

protected:
    virtual bool preCheck(const std::vector<Component> & components) const
    {
        if (components.empty() || components.size() % legs.size() != 0) {
            return false;
        }

        std::unordered_set<InstrumentType> types;

        for (const auto & comp : components) {
            types.insert(comp.type);
        }

        for (const auto & leg : legs) {
            if (types.find(leg.type) == types.end()) {
                return false;
            }
        }

        return true;
    }

    bool matchOrdered(const std::vector<Component> & components, const std::vector<int> & order) const
    {
        for (std::size_t i = 0; i < components.size(); i += legs.size()) {
            std::unordered_map<char, double> strikes;
            double lastStrike = 0;
            int lastStrikeOffset = 0;
            std::unordered_map<char, Expiration> exps;
            Expiration lastExp;
            int lastExpOffset = 0;

            for (std::size_t j = 0; j < legs.size(); ++j) {
                const auto & leg = legs[j];
                const auto & comp = components[order[i + j]];

                if (leg.type != comp.type) {
                    return false;
                }

                // Ratio
                if (std::holds_alternative<double>(leg.ratio)
                            ? comp.ratio != std::get<double>(leg.ratio)
                            : std::get<bool>(leg.ratio) != (comp.ratio > 0)) {
                    return false;
                }

                // Strike
                if (!matchOffset(strikes, lastStrike, lastStrikeOffset, leg.strike, comp.strike)) {
                    return false;
                }

                // Expiration
                if (const auto * dur = std::get_if<Duration>(&leg.expiration)) {
                    if (!lastExp.match(*dur, comp.expiration)) {
                        return false;
                    }
                }
                else if (!matchOffset(exps, lastExp, lastExpOffset, leg.expiration, comp.expiration)) {
                    return false;
                }
            }
        }

        return true;
    }

    template <class T, class... Ts>
    static bool matchOffset(
            std::unordered_map<char, T> & map,
            T & last,
            int & lastOffset,
            const std::variant<Ts...> & leg,
            const T & comp)
    {
        if (const auto * ch = std::get_if<char>(&leg)) {
            auto [it, ins] = map.emplace(*ch, comp);
            if (!ins && comp != it->second) {
                return false;
            }
            lastOffset = 0;
        }
        else {
            const auto offset = std::get<int>(leg);
            if (offset != 0 && offset == lastOffset) {
                if (comp != last) {
                    return false;
                }
            }
            else if ((offset > 0 && comp <= last) || (offset < 0 && comp >= last)) {
                return false;
            }
            lastOffset = offset;
        }

        last = comp;
        return true;
    }
};

struct FixedCombination : MultipleCombination
{
    using MultipleCombination::MultipleCombination;

    bool preCheck(const std::vector<Component> & components) const override
    {
        return components.size() == legs.size();
    }
};

struct MoreCombination : Combination
{
    const Leg leg;
    const std::size_t minCount;

    MoreCombination(std::string name, Leg leg, const std::size_t minCount)
        : Combination(std::move(name))
        , leg(std::move(leg))
        , minCount(minCount)
    {
    }

    bool match(const std::vector<Component> & components, std::vector<int> & order) const override
    {
        if (components.size() < minCount) {
            return false;
        }

        for (const auto & comp : components) {
            if (leg.type != comp.type &&
                !(leg.type == InstrumentType::O &&
                  (comp.type == InstrumentType::C || comp.type == InstrumentType::P))) {
                return false;
            }

            if (std::holds_alternative<double>(leg.ratio)
                        ? comp.ratio != std::get<double>(leg.ratio)
                        : std::get<bool>(leg.ratio) != (comp.ratio > 0)) {
                return false;
            }
        }

        std::iota(order.begin(), order.end(), 0);
        return true;
    }
};
} // namespace

struct Combinations::Impl
{
    std::vector<std::unique_ptr<Combination>> combinations;

    template <class T, class... Args>
    void add(Args &&... args)
    {
        combinations.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }
};

Combinations::Combinations()
    : impl(new Impl)
{
}

Combinations::~Combinations()
{
}

bool Combinations::load(const std::filesystem::path & resource)
{
    pugi::xml_document doc;

    if (!doc.load_file(resource.c_str())) {
        return false;
    }

    const auto combNodes = doc.child("combinations");

    if (!combNodes) {
        return false;
    }

    for (const auto & combNode : combNodes) {
        const auto legsNode = combNode.first_child();
        std::vector<Leg> legs;

        for (const auto & legNode : legsNode) {
            auto & leg = legs.emplace_back();

            // Type
            leg.type = static_cast<InstrumentType>(legNode.attribute("type").value()[0]);

            // Ratio
            const auto ratioAttr = legNode.attribute("ratio");

            if (std::strcmp(ratioAttr.value(), "+") == 0) {
                leg.ratio = true;
            }
            else if (std::strcmp(ratioAttr.value(), "-") == 0) {
                leg.ratio = false;
            }
            else {
                leg.ratio = ratioAttr.as_double(0);
            }

            // Strike
            if (const auto strikeAttr = legNode.attribute("strike")) {
                leg.strike = legNode.attribute("strike").value()[0];
            }
            else {
                const auto strikeOffsetStr = legNode.attribute("strike_offset").value();
                const int strikeOffset = std::strlen(strikeOffsetStr);
                leg.strike = strikeOffsetStr[0] == '+' ? strikeOffset : -strikeOffset;
            }

            // Expiration
            if (const auto expirationAttr = legNode.attribute("expiration").value()[0]) {
                leg.expiration = legNode.attribute("expiration").value()[0];
            }
            else {
                const auto expirationOffsetAttr = legNode.attribute("expiration_offset");
                Duration dur;
                char * durPtr;
                const auto expirationOffsetStr = expirationOffsetAttr.value();
                dur.amount = std::strtoull(expirationOffsetStr, &durPtr, 10);
                if (dur.amount == 0) {
                    const int strikeOffset = std::strlen(expirationOffsetStr);
                    leg.expiration = expirationOffsetStr[0] == '+' ? strikeOffset : -strikeOffset;
                }
                else {
                    dur.period = static_cast<Period>(*durPtr);
                    leg.expiration = dur;
                }
            }
        }

        std::string name = combNode.attribute("name").value();
        const char * cardinality = legsNode.attribute("cardinality").value();

        if (std::strcmp(cardinality, "fixed") == 0) {
            impl->add<FixedCombination>(std::move(name), std::move(legs));
        }
        else if (std::strcmp(cardinality, "multiple") == 0) {
            impl->add<MultipleCombination>(std::move(name), std::move(legs));
        }
        else if (std::strcmp(cardinality, "more") == 0) {
            impl->add<MoreCombination>(std::move(name), std::move(legs.front()), legsNode.attribute("mincount").as_ullong());
        }
    }

    return true;
}

std::string Combinations::classify(const std::vector<Component> & components, std::vector<int> & order) const
{
    std::vector<int> tempOrder(components.size());

    for (const auto & comb : impl->combinations) {
        if (comb->match(components, tempOrder)) {
            order.resize(tempOrder.size());
            for (std::size_t i = 0; i < order.size(); ++i) {
                order[tempOrder[i]] = i + 1;
            }
            return comb->name;
        }
    }

    return "Unclassified";
}
