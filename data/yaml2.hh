//
// Copyright (c) 2020, 2021, 2022 Humanitarian OpenStreetMap Team
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

#ifndef __YAML2_HH__
#define __YAML2_HH__

/// \file yaml2.hh
/// \brief Simple YAML file reader.
///
/// Read in a YAML config file and create a nested data structure so it can be accessed.

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>

/// \namespace yaml2
namespace yaml2 {

struct Node
{
   std::string value;
   std::vector<struct Node> children;
};

/// \class Yaml2
/// \brief Read a YAML file into a nested data structure
class Yaml2 {
    public:
        Node root;
        void read(const std::string &fspec);
    private:
        void add_node(Node &node, int index, Node &parent, int depth = 0);
        void clean(std::string &line);
        std::pair<std::string, int> process_line(std::string line );
        std::string scan_ident(std::string line);
        std::string filespec;
        char ident_char = '\0';
        int ident_count = 0;
        int ident_len = 0;
        int linenumber = 0;
        std::map<int, std::vector<std::string>> level;
        char idents[2] = {' ', '\t'};
};

} // EOF yaml2 namespace

#endif  // EOF __YAML2_HH__

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
