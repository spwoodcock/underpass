//
// Copyright (c) 2020, 2021, 2022, 2023 Humanitarian OpenStreetMap Team
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

/// \file querystats.cc
/// \brief This file is used to work with the OSM Stats database
///
/// This manages the OSM Stats schema in a postgres database. This
/// includes querying existing data in the database, as well as
/// updating the database.

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/date_time.hpp>
#include <boost/locale.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/time_facet.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/relative_difference.hpp>

using namespace boost::math;
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "osm/osmobjects.hh"
#include "utils/log.hh"
#include "osm/changeset.hh"
#include "stats/querystats.hh"
#include "data/pq.hh"
using namespace pq;
using namespace logger;

/// \namespace querystats
namespace querystats {

QueryStats::QueryStats(void) {}

QueryStats::QueryStats(std::shared_ptr<Pq> db) {
    dbconn = db;
}

std::string
QueryStats::applyChange(const osmchange::ChangeStats &change) const
{
#ifdef TIMING_DEBUG_X
    boost::timer::auto_cpu_timer timer("applyChange(statistics): took %w seconds\n");
#endif
    // std::cout << "Applying OsmChange data" << std::endl;

    std::string ahstore;
    std::string mhstore;

    if (change.added.size() > 0) {
        ahstore = "HSTORE(ARRAY[";
        for (const auto &added: std::as_const(change.added)) {
            if (added.second > 0) {
                ahstore += " ARRAY[" + dbconn->escapedString(added.first) + "," +
                           dbconn->escapedString(std::to_string(added.second)) + "],";
            }
        }
        ahstore.erase(ahstore.size() - 1);
        ahstore += "])";
    }

    if (change.modified.size() > 0) {
        mhstore = "HSTORE(ARRAY[";
        for (const auto &modified: std::as_const(change.modified)) {
            if (modified.second > 0) {
                mhstore += " ARRAY[" + dbconn->escapedString(modified.first) + "," +
                           dbconn->escapedString(std::to_string(modified.second)) + "],";
            }
        }
        mhstore.erase(mhstore.size() - 1);
        mhstore += "])";
    }

    // Some of the data field in the changset come from a different file,
    // which may not be downloaded yet.
    ptime now = boost::posix_time::microsec_clock::universal_time();
    std::string aquery = "INSERT INTO changesets (id, user_id, closed_at, updated_at, ";

    if (change.added.size() > 0) {
        aquery += "added, ";
    }
    if (change.modified.size() > 0) {
        aquery += "modified, ";
    }
    aquery.erase(aquery.size() - 2);
    aquery += ")";

    aquery += " VALUES(" + std::to_string(change.change_id) + ", ";
    aquery += std::to_string(change.user_id) + ", ";
    aquery += "\'" + to_simple_string(change.closed_at) + "\', ";
    aquery += "\'" + to_simple_string(now) + "\', ";

    if (change.added.size() > 0) {
        aquery += ahstore + ", ";
    }
    if (change.modified.size() > 0) {
        aquery += mhstore + ", ";
    }

    aquery.erase(aquery.size() - 2);
    aquery += ") ON CONFLICT (id) DO UPDATE SET";

    aquery += " closed_at = \'" + to_simple_string(change.closed_at) + "\', ";
    aquery += " updated_at = \'" + to_simple_string(now) + "\', ";
    if (change.added.size() > 0) {
        aquery += "added = " + ahstore + ", ";
    } else {
        aquery += "added = null, ";
    }
    if (change.modified.size() > 0) {
        aquery += "modified = " + mhstore + ", ";
    } else {
        aquery += "modified = null, ";
    }
    aquery.erase(aquery.size() - 2);

    return aquery + ";";

}

std::string
QueryStats::applyChange(const changeset::ChangeSet &change) const
{
#ifdef TIMING_DEBUG_X
    boost::timer::auto_cpu_timer timer("applyChange(changeset): took %w seconds\n");
#endif

    std::string query = "INSERT INTO changesets (id, editor, user_id, created_at, closed_at, updated_at";

    if (change.hashtags.size() > 0) {
        query += ", hashtags ";
    }

    if (!change.source.empty()) {
        query += ", source ";
    }

    query += ", bbox) VALUES(";
    query += std::to_string(change.id) + "," + dbconn->escapedString(change.editor) + ",\'";

    query += std::to_string(change.uid) + "\',\'";
    query += to_simple_string(change.created_at) + "\', \'";
    if (change.closed_at != not_a_date_time) {
        query += to_simple_string(change.closed_at) + "\', \'";
    } else {
        query += to_simple_string(change.created_at) + "\', \'";
    }
    ptime now = boost::posix_time::microsec_clock::universal_time();
    query += to_simple_string(now) + "\'";


    // Hashtags
    if (change.hashtags.size() > 0) {
        query += ", ARRAY[";
        for (const auto &hashtag: std::as_const(change.hashtags)) {
            auto ht{hashtag};
            boost::algorithm::replace_all(ht, "\"", "&quot;");
            query += dbconn->escapedString(ht) + ", ";
        }
        query.erase(query.size() - 2);
        query += "]";
    }

    // The source field is not always present
    if (!change.source.empty()) {
        query += ",\'" + change.source += "\'";
    }

    // Store the current values as they can get changed to expand very short
    // lines or POIs so they have a bounding box big enough for Postgis to use.
    double min_lat = change.min_lat;
    double max_lat = change.max_lat;
    double min_lon = change.min_lon;
    double max_lon = change.max_lon;

    const double fudge{0.0001};

    // A changeset with a single node in it doesn't draw a line
    if (change.max_lon < 0 && change.min_lat < 0) {
        min_lat = change.min_lat + (fudge / 2);
        max_lat = change.max_lat + (fudge / 2);
        min_lon = change.min_lon - (fudge / 2);
        max_lon = change.max_lon - (fudge / 2);
    }

    // Not a line
    if (max_lon == min_lon || max_lat == min_lat) {
        min_lat = change.min_lat + (fudge / 2);
        max_lat = change.max_lat + (fudge / 2);
        min_lon = change.min_lon - (fudge / 2);
        max_lon = change.max_lon - (fudge / 2);
    }

    // Single point
    if (max_lon < 0 && min_lat < 0) {
        min_lat = change.min_lat + (fudge / 2);
        max_lat = change.max_lat + (fudge / 2);
        min_lon = change.min_lon - (fudge / 2);
        max_lon = change.max_lon - (fudge / 2);
    }

    // Changeset bounding box
    std::string bbox;
    bbox += ", ST_MULTI(ST_GeomFromEWKT(\'SRID=4326;POLYGON((";
    // Upper left
    bbox += std::to_string(max_lon) + "  ";
    bbox += std::to_string(max_lat) + ",";
    // Upper right
    bbox += std::to_string(min_lon) + "  ";
    bbox += std::to_string(max_lat) + ",";
    // Lower right
    bbox += std::to_string(min_lon) + "  ";
    bbox += std::to_string(min_lat) + ",";
    // Lower left
    bbox += std::to_string(max_lon) + "  ";
    bbox += std::to_string(min_lat) + ",";
    // Close the polygon
    bbox += std::to_string(max_lon) + "  ";
    bbox += std::to_string(max_lat) + ")";
    query += bbox;

    query += ")\')";
    query += ")) ON CONFLICT (id) DO UPDATE SET editor=" + dbconn->escapedString(change.editor);
    query += ", created_at=\'" + to_simple_string(change.created_at);
    query += "\', updated_at=\'" + to_simple_string(now) + "\'";

    if (change.hashtags.size() > 0) {
        query += ", hashtags=";
        query += "ARRAY [";
        for (const auto &hashtag: std::as_const(change.hashtags)) {
            auto ht{hashtag};
            boost::algorithm::replace_all(ht, "\"", "&quot;");
            query += dbconn->escapedString(ht) + ", ";
        }
        query.erase(query.size() - 2);
        query += "]";
    } else {
        query += ", hashtags=null";
    }

    query += ", bbox=" + bbox.substr(2) + ")'));";
    
    return query;

}

} // namespace querystats

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
