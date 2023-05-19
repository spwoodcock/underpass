//
// Copyright (c) 2023 Humanitarian OpenStreetMap Team
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

/// \file queryraw.cc
/// \brief This file is used to work with the OSM Raw database
///
/// This manages the OSM Raw schema in a postgres database. This
/// includes querying existing data in the database, as well as
/// updating the database.

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <iostream>
#include <boost/timer/timer.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include "utils/log.hh"
#include "data/pq.hh"
#include "raw/queryraw.hh"
#include "osm/osmobjects.hh"
#include "osm/osmchange.hh"

#include <boost/timer/timer.hpp>

using namespace pq;
using namespace logger;
using namespace osmobjects;
using namespace osmchange;

/// \namespace queryraw
namespace queryraw {

QueryRaw::QueryRaw(void) {}

QueryRaw::QueryRaw(std::shared_ptr<Pq> db) {
    dbconn = db;
}

std::string
QueryRaw::applyChange(const OsmNode &node) const
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("applyChange(raw node): took %w seconds\n");
#endif
    std::string query;
    if (node.action == osmobjects::create || node.action == osmobjects::modify) {
        query = "INSERT INTO raw_node as r (osm_id,  geometry, tags, timestamp, version) VALUES(";
        std::string format = "%d, ST_GeomFromText(\'%s\', 4326), %s, \'%s\', %d \
        ) ON CONFLICT (osm_id) DO UPDATE SET  geometry = ST_GeomFromText(\'%s\', \
        4326), tags = %s, timestamp = \'%s\', version = %d WHERE r.version < %d;";
        boost::format fmt(format);

        // osm_id
        fmt % node.id;

        // geometry
        std::stringstream ss;
        ss << std::setprecision(12) << boost::geometry::wkt(node.point);
        std::string geometry = ss.str();
        fmt % geometry;

        // tags
        std::string tags = "";
        if (node.tags.size() > 0) {
            for (auto it = std::begin(node.tags); it != std::end(node.tags); ++it) {
                std::string tag_format = "\"%s\" => \"%s\",";
                boost::format tag_fmt(tag_format);
                tag_fmt % dbconn->escapedString(it->first);
                tag_fmt % dbconn->escapedString(it->second);
                tags += tag_fmt.str();
            }
            tags.erase(tags.size() - 1);
            tags = "'" + tags + "'";
        } else {
            tags = "null";
        }
        fmt % tags;
        // timestamp
        std::string timestamp = to_simple_string(boost::posix_time::microsec_clock::universal_time());
        fmt % timestamp;
        // version
        fmt % node.version;

        // ON CONFLICT
        fmt % geometry;
        fmt % tags;
        fmt % timestamp;
        fmt % node.version;
        fmt % node.version;

        query += fmt.str();

    } else if (node.action == osmobjects::remove) {
        query = "DELETE from raw_node where osm_id = " + std::to_string(node.id) + ";";
    }

    return query;
}

std::string
QueryRaw::applyChange(const OsmWay &way) const
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("applyChange(raw poly): took %w seconds\n");
#endif
    std::string query = "";

    if (way.refs.front() == way.refs.back() && (way.action == osmobjects::create || way.action == osmobjects::modify)) {
        query = "INSERT INTO raw_poly as r (osm_id, tags, refs, timestamp, version) VALUES(";
        std::string format = "%d, %s, %s, \'%s\', %d) \
        ON CONFLICT (osm_id) DO UPDATE SET geometry = %s, tags = %s, refs = %s, timestamp = \'%s\', version = %d WHERE r.version < %d;";
        boost::format fmt(format);
        // osm_id
        fmt % way.id;

        // tags
        std::string tags = "";
        if (way.tags.size() > 0) {
            for (auto it = std::begin(way.tags); it != std::end(way.tags); ++it) {
                std::string tag_format = "\"%s\" => \"%s\",";
                boost::format tag_fmt(tag_format);
                tag_fmt % dbconn->escapedString(it->first);
                tag_fmt % dbconn->escapedString(it->second);
                tags += tag_fmt.str();
            }
            tags.erase(tags.size() - 1);
            tags = "'" + tags + "'";
        } else {
            tags = "null";
        }

        fmt % tags;
        // refs
        std::string refs = "";
        if (way.refs.size() > 0) {
            for (auto it = std::begin(way.refs); it != std::end(way.refs); ++it) {
                refs += std::to_string(*it) + ",";
            }
            refs.erase(refs.size() - 1);
            refs = "ARRAY[" + refs + "]";
        } else {
            refs = "null";
        }
        fmt % refs;
        // timestamp
        std::string timestamp = to_simple_string(boost::posix_time::microsec_clock::universal_time());
        fmt % timestamp;
        // version
        fmt % way.version;

        // ON CONFLICT
        fmt % tags;
        fmt % refs;
        fmt % timestamp;
        fmt % way.version;
        fmt % way.version;

        query += fmt.str();

        // Way refs table
        for (auto it = way.refs.begin(); it != way.refs.end(); ++it) {
            query += "INSERT INTO rawrefs (node_id, way_id) VALUES (" + std::to_string(*it) + "," + std::to_string(way.id) + ") ON CONFLICT (node_id, way_id) DO NOTHING;";
        }

    } else if (way.action == osmobjects::remove) {
        query = "DELETE from raw_poly where osm_id = " + std::to_string(way.id) + ";";
        query += "DELETE from rawrefs where way_id = " + std::to_string(way.id) + ";";
    }

    return query;
}

std::vector<long> arrayStrToVector(std::string &refs_str) {
    refs_str.erase(0, 1);
    refs_str.erase(refs_str.size() - 1);
    std::vector<long> refs;
    std::stringstream ss(refs_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        refs.push_back(std::stod(token));
    }
    return refs;
}

// Update ways geometries: not used yet
std::string
QueryRaw::applyChange(const std::shared_ptr<std::map<long, std::pair<double, double>>> nodes) const
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("applyChange(modified nodes): took %w seconds\n");
#endif
    // 1. Get all ways that have references to nodes
    std::string nodeIds;
    for (auto node = nodes->begin(); node != nodes->end(); ++node) {
        nodeIds += std::to_string(node->first) + ",";
    }
    nodeIds.erase(nodeIds.size() - 1);
    std::string waysQuery = "SELECT osm_id, refs FROM raw_poly where refs && ARRAY[" + nodeIds + "]::bigint[];";
    auto ways = dbconn->query(waysQuery);

    // 2. Update ways geometries

    // For each way in results ...
    std::string query = "";
    for (auto way_it = ways.begin(); way_it != ways.end(); ++way_it) {
        std::string queryPoints = "";
        std::string queryWay = "";
        // Way OSM id
        auto osm_id = (*way_it)[0].as<long>();
        // Way refs
        std::string refs_str = (*way_it)[1].as<std::string>();
        // For each way ref ...
        if (refs_str.size() > 1) {
            auto refs =  arrayStrToVector(refs_str);
            // For each ref in way ...
            int refIndex = 0;
            queryWay += "UPDATE raw_poly SET geometry = ";
            for (auto ref_it = refs.begin(); ref_it != refs.end(); ++ref_it) {
                // If node was modified, update it
                if (nodes->find(*ref_it) != nodes->end()) {
                    queryWay += "ST_SetPoint(";
                    queryPoints += std::to_string(refIndex) + ", ST_MakePoint(";
                    queryPoints += std::to_string(nodes->at(*ref_it).first) + "," + std::to_string(nodes->at(*ref_it).second);
                    queryPoints += ")),";
                }
                refIndex++;
            }
        }
        queryPoints.erase(queryPoints.size() - 1);
        queryWay += "geometry, " + queryPoints;
        queryWay += " WHERE and osm_id = " + std::to_string(osm_id) + ";";
        query += queryWay;

    }
    // 3. Save ways
    return query;
}

void QueryRaw::getNodeCache(std::shared_ptr<OsmChangeFile> osmchanges) {
    // Get all nodes ids referenced in ways
    std::string nodeIds;
    for (auto it = std::begin(osmchanges->changes); it != std::end(osmchanges->changes); it++) {
        OsmChange *change = it->get();
        for (auto wit = std::begin(change->ways); wit != std::end(change->ways); ++wit) {
            OsmWay *way = wit->get();
            for (auto rit = std::begin(way->refs); rit != std::end(way->refs); ++rit) {
                if (!osmchanges->nodecache.count(*rit)) {
                    nodeIds += std::to_string(*rit) + ",";
                }
            }
        }
    }
    if (nodeIds.size() > 1) {

        nodeIds.erase(nodeIds.size() - 1);

        // Get Nodes from DB
        std::string nodesQuery = "SELECT osm_id, st_x(geometry) as lat, st_y(geometry) as lon FROM raw_node where  osm_id in (" + nodeIds + ") and st_x(geometry) is not null and st_y(geometry) is not null;";
        auto result = dbconn->query(nodesQuery);

        // Fill nodecache
        for (auto node_it = result.begin(); node_it != result.end(); ++node_it) {
            auto node_id = (*node_it)[0].as<long>();
            auto node_lat = (*node_it)[1].as<double>();
            auto node_lon = (*node_it)[2].as<double>();
            OsmNode node(node_lat, node_lon);
            osmchanges->nodecache[node_id] = node.point;
        }
    }
}

void
QueryRaw::getNodeCacheFromWays(std::shared_ptr<std::vector<OsmWay>> ways, std::map<double, point_t> &nodecache) const
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("getNodeCacheFromWays(ways, nodecache): took %w seconds\n");
#endif

    // Get all nodes ids referenced in ways
    std::string nodeIds;
    for (auto wit = ways->begin(); wit != ways->end(); ++wit) {
        for (auto rit = std::begin(wit->refs); rit != std::end(wit->refs); ++rit) {
            nodeIds += std::to_string(*rit) + ",";
        }
    }
    if (nodeIds.size() > 1) {

        nodeIds.erase(nodeIds.size() - 1);

        // Get Nodes from DB
        std::string nodesQuery = "SELECT osm_id, st_x(geometry) as lat, st_y(geometry) as lon FROM raw_node where osm_id in (" + nodeIds + ") and st_x(geometry) is not null and st_y(geometry) is not null;";
        auto result = dbconn->query(nodesQuery);
        // Fill nodecache
        for (auto node_it = result.begin(); node_it != result.end(); ++node_it) {
            auto node_id = (*node_it)[0].as<long>();
            auto node_lat = (*node_it)[1].as<double>();
            auto node_lon = (*node_it)[2].as<double>();
            auto point = point_t(node_lat, node_lon);
            nodecache.insert(std::make_pair(node_id, point));
        }
    }
}

std::shared_ptr<std::vector<OsmWay>>
QueryRaw::getWaysByNodesRefs(const std::shared_ptr<std::map<long, std::pair<double, double>>> nodes) const
{
    // Get all ways that have references to nodes
    std::string nodeIds;
    for (auto node = nodes->begin(); node != nodes->end(); ++node) {
        nodeIds += std::to_string(node->first) + ",";
    }
    nodeIds.erase(nodeIds.size() - 1);
    std::string waysQuery = "SELECT osm_id, refs, version FROM raw_poly where refs && ARRAY[" + nodeIds + "]::bigint[] and tags -> 'building' = 'yes';";
    auto ways_result = dbconn->query(waysQuery);

    // Fill vector of OsmWay objects
    auto ways = std::make_shared<std::vector<OsmWay>>();
    for (auto way_it = ways_result.begin(); way_it != ways_result.end(); ++way_it) {
        OsmWay way;
        way.id = (*way_it)[0].as<long>();
        way.version = (*way_it)[2].as<long>();
        std::string refs_str = (*way_it)[1].as<std::string>();
        if (refs_str.size() > 1) {
            way.refs = arrayStrToVector(refs_str);
        }
        ways->push_back(way);
    }

    return ways;
}

int QueryRaw::getWaysCount() {
    std::string query = "select count(osm_id) from raw_poly where tags -> 'building' = 'yes'";
    auto result = dbconn->query(query);
    return result[0][0].as<int>();
}

std::shared_ptr<std::vector<OsmWay>> 
QueryRaw::getWaysFromDB(int lastid) {
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("getWaysFromDB(page): took %w seconds\n");
#endif
    std::string waysQuery = "SELECT osm_id, refs, version FROM raw_poly where tags -> 'building' = 'yes' and osm_id > " + std::to_string(lastid) + " order by osm_id asc limit 100;";
    auto ways_result = dbconn->query(waysQuery);

    // Fill vector of OsmWay objects
    auto ways = std::make_shared<std::vector<OsmWay>>();
    for (auto way_it = ways_result.begin(); way_it != ways_result.end(); ++way_it) {
        OsmWay way;
        way.id = (*way_it)[0].as<long>();
        way.version = (*way_it)[2].as<long>();
        std::string refs_str = (*way_it)[1].as<std::string>();
        if (refs_str.size() > 1) {
            way.refs = arrayStrToVector(refs_str);
        }
        ways->push_back(way);
    }

    return ways;
}

std::shared_ptr<OsmWay> 
QueryRaw::getWayById(long id) {
    std::string waysQuery = "SELECT osm_id, refs, version FROM raw_poly where osm_id=" + std::to_string(id) + ";";
    auto result = dbconn->query(waysQuery);

    // Fill vector of OsmWay objects
    OsmWay way;
    way.id = result[0][0].as<long>();
    way.version = result[0][2].as<long>();
    std::string refs_str = result[0][1].as<std::string>();
    if (refs_str.size() > 1) {
        way.refs = arrayStrToVector(refs_str);
    }
    return std::make_shared<OsmWay>(way);
}

} // namespace queryraw

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
