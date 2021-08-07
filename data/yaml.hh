//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
//
// This file is part of Underpass.
//
//     Underpass is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Underpass is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Underpass.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef __YAML_HH__
#define __YAML_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <map>
#include <iostream>

/// \file yaml.hh
/// \brief Simple YAML file reader.

/// \namespace yaml
namespace yaml {

class Yaml {
public:
    void read(const std::string &filespec);
    void dump(void);
private:
    std::string filespec;
    std::map<std::string, std::vector<std::string>> config;    
};
    
} // EOF yaml namespace

#endif  // EOF __YAML_HH__