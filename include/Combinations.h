#pragma once

#include "Component.h"

#include <filesystem>
#include <vector>

class Combinations
{
public:
    Combinations();
    ~Combinations();

    bool load(const std::filesystem::path & resource);

    std::string classify(const std::vector<Component> & components, std::vector<int> & order) const;

private:
    struct Impl;
    const std::unique_ptr<Impl> impl;
};
