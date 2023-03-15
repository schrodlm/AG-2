#ifndef __PROGTEST__

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <limits>
#include <optional>
#include <algorithm>
#include <bitset>
#include <list>
#include <array>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <random>

template<typename F, typename S>
struct std::hash<std::pair<F, S>> {
    std::size_t operator()(const std::pair<F, S> &p) const noexcept {
        // something like boost::combine would be much better
        return std::hash<F>()(p.first) ^ (std::hash<S>()(p.second) << 1);
    }
};

// For exposition only. In the tests Place will not
// be a string but some other type. This type will always
// be comparable, hashable and it will have default and
// copy constructors.
using Place = std::string;
using Connection = std::pair<Place, Place>;

struct Map {
    std::vector<Place> places;
    std::vector<Connection> connections;
};

#endif

struct TrafficNetworkTester {
    explicit TrafficNetworkTester(const Map &map) {
        places = map.places;
        assignConnections(map);
        createComponents();
    }

    // Count how many areas exist in the network
    // after adding conns.
    // Note that conns may introduce new places.
    unsigned count_areas(const std::vector<Connection> &conns) const {
        Map tmp = components;
        std::set<Place> new_seen_places;
        //add new connections
        for (auto &[from, to]: conns) {
            if (component.count(from) && component.count(to))
                tmp.connections.emplace_back(component.at(from), component.at(to));

                //only one of the places already exists
            else if (component.count(from)) {
                tmp.connections.emplace_back(component.at(from), to);
                if (!new_seen_places.count(to)) tmp.places.push_back(to), new_seen_places.insert(to);
            } else if (component.count(to)) {
                tmp.connections.emplace_back(from, component.at(to));
                if (!new_seen_places.count(from)) tmp.places.push_back(from), new_seen_places.insert(from);
            }
                //none of these places exists
            else {
                if (from != to)
                    tmp.connections.emplace_back(from, to), tmp.places.push_back(from), tmp.places.push_back(to);

            }

        }

        TrafficNetworkTester count(tmp);
        return count.number_of_components;
    }

    void assignConnections(const Map &map) {
        for (auto &[from, to]: map.connections) {
            connections[from].push_back(to);
            reversed_connections[to].push_back(from);
        }
    }

    void createComponents() {
        //1. create empty stack
        std::stack<Place> outs;

        //2. find the places that are part of sources components in original graph (by finding target places in reversed graph)
        std::set<Place> seen;
        for (auto &place: places) {
            // if place has not yet been found, call recursiveDFSForFindingSources
            if (!seen.count(place)) {

                seen.insert(place);
                // ** recursiveDFSForFindingSources(place, seen, outs) ** ;
                iterativeDFSForFindingSources(place, seen, outs);
            }
        }

        // now out stack should be filled (on top are the source places)
        // 3. Start to find each component by emptying the stack
        while (!outs.empty()) {
            Place place = outs.top();
            outs.pop();

            if (!component.count(place)) {
                // ** recursiveDFSForFindingComponents(place,place); **
                iterativeDFSForFindingComponents(place, place);
            }
        }

        // 4. Create component map
        std::set<Place> seen_components;
        for (auto &place: places) {
            //add places
            if (!seen_components.count(component[place])) {
                components.places.push_back(component[place]);
                seen_components.insert(component[place]);
            }
            //add edges
            for (auto &connection: connections[place]) {
                //add only if it is not a loopback (edge into itself)
                if (component[place] != component[connection])
                    components.connections.emplace_back(component[place], component[connection]);
            }

        }
        number_of_components = components.places.size();


    }


    /**
     *  Looks for component of a graph
     *
     */
    void iterativeDFSForFindingComponents(Place start_place, const Place component_name) {
        std::stack<Place> st;
        st.push(start_place);

        while (!st.empty()) {
            Place curr_place = st.top();
            st.pop();
            component[curr_place] = component_name;

            for (auto &next_place: connections[curr_place]) {
                if (!component.count(next_place)) {
                    st.push(next_place);
                }
            }
        }

    }

    /**
     * Recursive approach for finding components
     */
    void recursiveDFSForFindingComponents(Place curr_place, const Place component_name) {
        component[curr_place] = component_name;
        for (auto &next_place: connections[curr_place]) {
            if (!component.count(next_place)) {
                recursiveDFSForFindingComponents(next_place, component_name);
            }
        }

    }

    /**
     * I had to make backtracking work exactly the same as in recursive approach, this was challenging but this approach works
     * -> finds sources in G^T (reversed graph G) -> which in fact finds targets in original G graph
     */
    void iterativeDFSForFindingSources(Place start_place, std::set<Place> &seen, std::stack<Place> &outs) {
        std::stack<Place> st;
        st.push(start_place);

        while (!st.empty()) {
            Place curr_place = st.top();
            bool processed = true;
            for (auto &next_place: reversed_connections[curr_place]) {
                if (!seen.count(next_place)) {
                    seen.insert(next_place);
                    st.push(next_place);
                    //if we can continue to any path from this place, it is not yet processed
                    processed = false;
                    break;
                }

            }
            if (processed) {
                st.pop();
                outs.push(curr_place);
            }

        }

    }

    /**
     *  Recursive approach to finding sources
     */
    void recursiveDFSForFindingSources(Place curr_place, std::set<Place> &seen, std::stack<Place> &outs) {

        for (auto &next_place: reversed_connections[curr_place]) {
            if (!seen.count(next_place)) {
                seen.insert(next_place);
                recursiveDFSForFindingSources(next_place, seen, outs);
            }
        }
        outs.push(curr_place);
    }

    //printing utilities
    /**
     * prints out provided Map in a graphwiz format so you can run ./a.out | dot -Tpng -o output.png in your CLI to create
     * png representation of that graph
     */
    void printDot();

    void printDotInner(Place place, std::set<Place> &seen);


    std::vector<Place> places;
    std::map<Place, std::vector<Place>> connections;
    std::map<Place, std::vector<Place>> reversed_connections;

    Map components;
    // Place is a part of Place component
    std::map<Place, Place> component;
    unsigned number_of_components = 0;
};

#ifndef __PROGTEST__

using Test = std::pair<Map, std::vector<std::pair<unsigned, std::vector<Connection>>>>;

Test TESTS[] = {
        {
                {{
                         "U Vojenské nemocnice", "Kuchyňka", "V Korytech", "Kelerka", "Vozovna Strašnice",
                         "Geologická", "U Studánky", "U Jahodnice", "Hadovka", "Barrandovská",
                         "K Netlukám", "Obchodní centrum Sárská", "Praha-Smíchov", "Sušická", "Moráň",
                         "Praha-Bubny", "Rajská zahrada", "Strossmayerovo náměstí", "Průmstav",
                 }, {
                         {"U Vojenské nemocnice", "Vozovna Strašnice"}, {"K Netlukám", "Obchodní centrum Sárská"},
                         {"Praha-Smíchov", "U Jahodnice"}, {"Praha-Smíchov", "K Netlukám"},
                         {"Vozovna Strašnice", "Kelerka"}, {"Obchodní centrum Sárská", "Geologická"},
                         {"K Netlukám", "Praha-Smíchov"}, {"V Korytech", "Geologická"},
                         {"V Korytech", "Vozovna Strašnice"}, {"Vozovna Strašnice", "V Korytech"},
                         {"U Vojenské nemocnice", "Kuchyňka"}, {"Kelerka", "Geologická"},
                         {"Praha-Bubny", "Strossmayerovo náměstí"}, {"Kuchyňka", "V Korytech"},
                         {"Praha-Smíchov", "Praha-Bubny"}, {"Obchodní centrum Sárská", "Moráň"},
                         {"Kelerka", "V Korytech"}, {"Kelerka", "V Korytech"},
                         {"Hadovka", "Rajská zahrada"}, {"V Korytech", "Geologická"},
                         {"Sušická", "Praha-Smíchov"}, {"Barrandovská", "K Netlukám"},
                         {"V Korytech", "Kelerka"}, {"K Netlukám", "V Korytech"},
                         {"U Studánky", "Kuchyňka"}, {"Hadovka", "Barrandovská"},
                         {"Praha-Bubny", "U Studánky"}, {"Moráň", "K Netlukám"},
                         {"Strossmayerovo náměstí", "Kelerka"}, {"Barrandovská", "U Jahodnice"},
                         {"V Korytech", "Kuchyňka"}, {"Průmstav", "Praha-Smíchov"},
                         {"Geologická", "V Korytech"}, {"Rajská zahrada", "Kuchyňka"},
                         {"U Jahodnice", "Kuchyňka"}, {"Praha-Smíchov", "Sušická"},
                         {"K Netlukám", "Obchodní centrum Sárská"}, {"Geologická", "Kelerka"},
                         {"Obchodní centrum Sárská", "K Netlukám"}, {"Obchodní centrum Sárská", "K Netlukám"},
                         {"Hadovka", "U Studánky"}, {"K Netlukám", "Sušická"},
                         {"Moráň", "U Vojenské nemocnice"}, {"Obchodní centrum Sárská", "Praha-Smíchov"},
                         {"V Korytech", "U Studánky"}, {"Kuchyňka", "Geologická"},
                         {"K Netlukám", "Moráň"}, {"Sušická", "U Vojenské nemocnice"},
                         {"Kuchyňka", "U Vojenské nemocnice"},
                 }}, {
                        {9, {
                        }},
                        {5, {
                                {"Kuchyňka", "Kuchyňka"}, {"Strossmayerovo náměstí", "Průmstav"},
                                {"Průmstav", "V Korytech"}, {"K Netlukám", "Praha-Smíchov"},
                                {"Praha-Bubny", "Barrandovská"},
                        }},
                        {9, {
                                {"Rajská zahrada", "Strossmayerovo náměstí"}, {"Sušická", "Obchodní centrum Sárská"},
                                {"Průmstav", "Strossmayerovo náměstí"}, {"Moráň", "Strossmayerovo náměstí"},
                        }},
                        {5, {
                                {"Kelerka", "K Netlukám"}, {"U Studánky", "Sušická"},
                                {"U Studánky", "V Korytech"}, {"U Studánky", "Strossmayerovo náměstí"},
                                {"Kuchyňka", "V Korytech"}, {"Průmstav", "Rajská zahrada"},
                        }},
                        {5, {
                                {"Vozovna Strašnice", "Obchodní centrum Sárská"},
                                {"Strossmayerovo náměstí", "Praha-Bubny"},
                                {"U Vojenské nemocnice", "V Korytech"}, {"U Jahodnice", "U Studánky"},
                                {"Rajská zahrada", "V Korytech"}, {"Obchodní centrum Sárská", "Sušická"},
                        }},
                        {2, {
                                {"Barrandovská", "Praha-Smíchov"}, {"Geologická", "Hadovka"},
                                {"U Studánky", "Moráň"}, {"U Vojenské nemocnice", "Praha-Smíchov"},
                        }},
                }
        },

};

template<typename C>
void test(C &&tests, bool exact) {
    int fail = 0, ok = 0;

    for (auto &&[map, test_cases]: tests) {
        TrafficNetworkTester T{map};

        for (auto &&[ans, conns]: test_cases)
            if (exact)
                (ans == T.count_areas(conns) ? ok : fail)++;
            else
                ((ans == 1) == (T.count_areas(conns) == 1) ? ok : fail)++;
    }

    if (fail)
        std::cout << fail << " of " << fail + ok << " tests failed!" << std::endl;
    else
        std::cout << "All " << ok << " tests succeded!" << std::endl;
}

int main() {
    test(TESTS, true);
}

#endif

void TrafficNetworkTester::printDot() {

    std::cout << "digraph {" << std::endl;
    std::cout << "node [shape=circle, width=0.4]" << std::endl;
    std::set<Place> seen;
    for (auto &place: places) {
        if (!seen.count(place)) {
            printDotInner(place, seen);
        }

    }
    std::cout << "}" << std::endl;
}

void TrafficNetworkTester::printDotInner(Place place, std::set<Place> &seen) {


    seen.insert(place);

    //std::cout << place << " [label = <<FONT POINT-SIZE=\"24.0\" FACE=\"ambrosia\">" << place << "</FONT>" << ">]"<< std::endl;

    for (auto &conn: connections[place]) {
        std::cout << "\"" << place << "\"" << "->" << "\"" << conn << "\"" << std::endl;

        if (!seen.count(conn)) {
            seen.insert(conn);
            printDotInner(conn, seen);
        }

    }
}

