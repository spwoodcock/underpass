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

#ifndef __YAML_HH__
#define __YAML_HH__

/// \file yaml.hh
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

/// \namespace yaml
namespace yaml {

/// \class Node
/// \brief Represents a Node inside a nested structure
class Node {
    public:
        ///
        /// \brief value is a string value for the Node
        ///
        std::string value;
        ///
        /// \brief children the Node can contain several children
        ///
        std::vector<struct Node> children;
        ///
        /// \brief get returns a Node identified by a key
        /// \param key is the value that must to match with the Node
        /// \return the Node
        ///
        Node get(std::string key);
        ///
        /// \brief get_value returns the value for a Node that has only one children
        /// \param key is the value that must to match with the parent Node
        /// \return the string value of the children Node
        ///
        std::string get_value(std::string key);
        ///
        /// \brief get_values returns all values of children Nodes
        /// \param key is the value that must to match with the parent Node
        /// \return a vector of strings with the values of all the Node's children
        ///
        std::vector<std::string> get_values(std::string key);
        ///
        /// \brief contains_key check if a key is present in some Node
        /// \param key is the value that must to match with the Node
        /// \return TRUE if key is present or FALSE if not
        ///
        bool contains_key(std::string key);
        ///
        /// \brief contains_value check if a combination of key:value is present in some Node
        ///        and one of its children
        /// \param key is the value that must to match with the Node
        /// \param value is the value that must to match with one of the Node's children
        /// \return TRUE if the key:value combination is present or FALSE if not
        ///
        bool contains_value(std::string key, std::string value);
        ///
        /// \brief dump prints values for all Node's children (all values if Node is root)
        ///
        void dump();
        Node();
        ~Node();
    private:
        void get(std::string key, Node &node);
        void contains_key(std::string key, bool &result);
        void contains_value(std::string key, std::string value, bool &result);

};

/// \class Yaml
/// \brief Read a YAML file into a nested data structure
class Yaml {
    public:
        Node root;
        ///
        /// \brief read reads a YAML file and parse it to a nested data structure
        /// \param fspec is the filename
        ///
        void read(const std::string &fspec);
        ///
        /// \brief get returns a Node identified by a key, starting from the root Node
        /// \param key is the value that must to match with the Node
        /// \return the Node
        ///
        Node get(std::string key);
        ///
        /// \brief dump prints all values for all Nodes
        ///
        void dump();
        ///
        /// \brief contains_key check if a key is present in some Node, , starting from the root Node
        /// \param key is the value that must to match with the Node
        /// \return TRUE if key is present or FALSE if not
        ///
        bool contains_key(std::string key);
        ///
        /// \brief contains_value check if a combination of key:value is present in some Node
        ///        and one of its children, starting from the root Node
        /// \param key is the value that must to match with the Node
        /// \param value is the value that must to match with one of the Node's children
        /// \return TRUE if the key:value combination is present or FALSE if not
        ///
        bool contains_value(std::string key, std::string value);
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

} // EOF yaml namespace

#endif  // EOF __YAML_HH__

// Local Variables:
// mode: C++
// indent-tabs-mode: nil
// End: