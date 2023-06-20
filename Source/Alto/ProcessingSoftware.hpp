#pragma once

#include <string>

namespace frog::alto {

struct ProcessingSoftware {

    std::string softwareCreator;
    std::string softwareName;
    std::string softwareVersion;
    std::string applicationDescription;

    ProcessingSoftware() = default;
    ProcessingSoftware(std::string_view softwareCreator, std::string_view softwareName, std::string_view softwareVersion, std::string_view applicationDescription)
        : softwareCreator{ softwareCreator }, softwareName{ softwareName }, softwareVersion{ softwareVersion }, applicationDescription{ applicationDescription } {}

};

}
