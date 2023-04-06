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

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <array>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <pqxx/pqxx>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "replicator/planetreplicator.hh"
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/regex.hpp>
#include <boost/timer/timer.hpp>

using namespace boost;
namespace opts = boost::program_options;

#include "utils/geoutil.hh"
#include "osm/changeset.hh"
#include "osm/osmchange.hh"
#include "replicator/threads.hh"
#include "utils/log.hh"
#include "underpassconfig.hh"

using namespace querystats;
using namespace underpassconfig;

#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1

// Exit code when connection to DB fails
#define EXIT_DB_FAILURE -1

using namespace logger;

// Forward declarations
namespace changeset {
class ChangeSet;
};

/// \namespace planetreplicator
namespace planetreplicator {

/// A helper function to simplify the main part.
template <class T> std::ostream &
operator<<(std::ostream &os, const std::vector<T> &v)
{
    copy(v.begin(), v.end(), std::ostream_iterator<T>(os, " "));
    return os;
}

/// \class PlanetReplicator
/// \brief This class does all the actual work
///
/// This class identifies, downloads, and processes a replication file.
/// Replication files are available from the OSM planet server.

/// Create a new instance, and read in the geoboundaries file.
PlanetReplicator::PlanetReplicator(void) {};

std::shared_ptr<RemoteURL> PlanetReplicator::findRemotePath(const underpassconfig::UnderpassConfig &config, ptime time) {
    yaml::Yaml yaml;

    std::string statsConfigFilename = "planetreplicator.yaml";
    std::string rep_file = SRCDIR;
    rep_file += "/src/replicator/" + statsConfigFilename;
    if (!boost::filesystem::exists(rep_file)) {
        rep_file = PKGLIBDIR;
        rep_file += "/" + statsConfigFilename;
    }
    yaml.read(rep_file);
    std::map<int, ptime> hashes;

    yaml::Node hashes_config;

    if (config.frequency == replication::minutely) {
        hashes_config = yaml.get("minute");
    } else {
        hashes_config = yaml.get("changeset");
    }
    for (auto it = hashes_config.children.begin(); it != hashes_config.children.end(); ++it) {
        hashes.insert(
            std::make_pair(
                    stoi(it->value),
                    time_from_string(it->children[0].value)
            )
        );
    }

    boost::posix_time::time_duration delta;
    std::pair<int, ptime> closest_prev;
    std::pair<int, ptime> closest_next;
    auto it = hashes.begin();
    for (; it != hashes.end(); ++it) {
        delta = config.start_time - it->second;
        if (delta.total_seconds() < 0) {
            break;
        }
    }

    closest_next = *it;
    closest_prev = *(--it);
    delta = closest_next.second - closest_prev.second;

    if (delta.total_seconds() < 0) {
        closest_next = closest_prev;
        closest_prev = *(std::prev(hashes.end(), 2));
        delta = closest_next.second - closest_prev.second;
    }

    double ratio = (delta.total_seconds() / 60.0) / (closest_next.first - closest_prev.first);

    boost::posix_time::time_duration delta_target = config.start_time - closest_prev.second;
    int target_int = closest_prev.first + (delta_target.total_seconds() / 60.0 / ratio);

    double n = target_int/1000000.0;
    double major = floor(n);
    double decimal = n - major;

    double n2 = decimal * 1000;
    double minor = floor(n2);
    double index = (n2 - minor) * 1000;

    boost::format majorfmt("%03d");
    boost::format minorfmt("%03d");
    boost::format indexfmt("%03d");

    majorfmt % (major);
    minorfmt % (minor);
    indexfmt % (index);

    std::string path = majorfmt.str() + "/" + minorfmt.str() + "/" + indexfmt.str();

    std::string suffix;
    std::string fullurl;
    std::string cached = config.datadir + StateFile::freq_to_string(config.frequency);

    if (config.frequency == replication::minutely) {
        suffix = ".osc.gz";
    } else {
        suffix = ".osm.gz";
    }

    connectServer("https://" + config.planet_server);
    auto remote = std::make_shared<RemoteURL>();
    cached += "/" + path + suffix;
    fullurl = "https://" + config.planet_server + "/" + cached;
    remote->parse(fullurl);
    remote->updatePath(major, minor, index);

    return remote;
};

} // namespace planetreplicator

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
