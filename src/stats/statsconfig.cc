//
// Copyright (c) 2020, 2021, 2022, 2023, 2024 Humanitarian OpenStreetMap Team
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

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include "utils/yaml.hh"
#include "statsconfig.hh"
#include "osm/osmchange.hh"
#include <memory>

/// \namespace statsconfig
namespace statsconfig {

    std::map<std::string, std::shared_ptr<std::vector<StatsConfigCategory>>> StatsConfig::cache;
    std::string StatsConfig::path;

    StatsConfigCategory::StatsConfigCategory(std::string name) {
        this->name = name;
    }
    StatsConfigCategory::StatsConfigCategory (
        std::string name,
        std::map<std::string, std::set<std::string>> way,
        std::map<std::string, std::set<std::string>> node,
        std::map<std::string, std::set<std::string>> relation )
    {
        this->name = name;
        this->way = way;
        this->node = node;
        this->relation = relation;
    };


    StatsConfig::StatsConfig() {
        if (path.empty()) {
            path = ETCDIR;
            path += "/stats/statistics.yaml";
            if (!boost::filesystem::exists(path)) {
                throw std::runtime_error("Statistics file not found: " + path);
            }
        }
        read_yaml(path);
    }

    void StatsConfig::setConfigurationFile(std::string statsConfigFilename) {
        if (!boost::filesystem::exists(statsConfigFilename)) {
            throw std::runtime_error("Statistics configuration file not found: " + statsConfigFilename);
        }
        path = statsConfigFilename;
    }

    std::shared_ptr<std::vector<StatsConfigCategory>> StatsConfig::read_yaml(std::string filename) {
        if (!cache.count(filename)) {
            yaml::Yaml yaml;
            yaml.read(filename);

            auto statscategories = std::make_shared<std::vector<StatsConfigCategory>>();
            for (auto it = std::begin(yaml.root.children); it != std::end(yaml.root.children); ++it) {
                std::map<std::string, std::set<std::string>> way_tags;
                std::map<std::string, std::set<std::string>> node_tags;
                std::map<std::string, std::set<std::string>> relation_tags;
                for (auto type_it = std::begin(it->children); type_it != std::end(it->children); ++type_it) {
                    for (auto value_it = std::begin(type_it->children); value_it != std::end(type_it->children); ++value_it) {
                        if (value_it->value != "*") {
                            for (auto tag_it = std::begin(value_it->children); tag_it != std::end(value_it->children); ++tag_it) {
                                if (type_it->value == "way") {
                                    way_tags[value_it->value].insert(tag_it->value);
                                } else if (type_it->value == "node") {
                                    node_tags[value_it->value].insert(tag_it->value);
                                } else if (type_it->value == "relation") {
                                    relation_tags[value_it->value].insert(tag_it->value);
                                }
                            }
                        } else {
                            if (type_it->value == "way") {
                                way_tags["*"].insert("*");
                            } else if (type_it->value == "node") {
                                node_tags["*"].insert("*");
                            } else if (type_it->value == "relation") {
                                relation_tags["*"].insert("*");
                            }
                        }
                    }
                }
                statscategories->push_back(
                    StatsConfigCategory(it->value, way_tags, node_tags, relation_tags)
                );
            }
            cache.insert(
                std::pair<std::string, std::shared_ptr<std::vector<StatsConfigCategory>>>(
                    filename, statscategories
                )
            );
        }
        return cache.at(filename);
    }

    bool StatsConfig::searchCategory(std::string tag, std::string value, std::map<std::string, std::set<std::string>> tags) {
        for (auto tag_it = std::begin(tags); tag_it != std::end(tags); ++tag_it) {
            if (tag_it->first == "*" || (
                    tag == tag_it->first && (
                        *(tag_it->second.begin()) == "*" ||
                        tag_it->second.find(value) != tag_it->second.end()
                    )
                )
            ) {
                return true;
            }
        }
        return false;
    };

    std::string StatsConfig::search(std::string tag, std::string value, osmchange::osmtype_t type) {
        bool category = false;
        std::vector<StatsConfigCategory> statscategories = *cache.begin()->second.get();
        for (auto it = std::begin(statscategories); it != std::end(statscategories); ++it) {
            if (type == osmchange::node) {
                category = searchCategory(tag, value, it->node);
            } else if (type == osmchange::way) {
                category = searchCategory(tag, value, it->way);
            } else if (type == osmchange::relation) {
                category = searchCategory(tag, value, it->relation);
            }
            if (category) {
                if (it->name == "\"[key]\"") {
                    return tag;
                } else if (it->name == "\"[key:value]\"") {
                    return tag + ":" + value;
                }
                return it->name;
            }
        }
        return "";
    }



} // EOF statsconfig namespace

// Local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
