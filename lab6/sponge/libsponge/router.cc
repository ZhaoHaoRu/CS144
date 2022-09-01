#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";
    // Your code here.
    _router_list.push_back(RouteEntry{route_prefix, prefix_length, next_hop, interface_num});
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // The router decrements the datagram’s TTL (time to live)
    if(dgram.header().ttl == 0 || (--dgram.header().ttl) == 0)
        return;

    // the longest match length
    uint8_t longest_match_length = 0;
    bool is_found = false;
    RouteEntry most_match_router;
    uint32_t ip = dgram.header().dst;
    // search the most match router
    auto iterator = _router_list.begin();
    while(iterator != _router_list.end()) {
        RouteEntry router = *iterator;
        // cerr << "router: " << router.route_prefix << endl;
        if(_is_match(ip, router.route_prefix, router.prefix_length)) {
            if(router.prefix_length >= longest_match_length) {
                most_match_router = router;
                is_found = true;
                // cerr << "find the most match router: " << most_match_router.route_prefix << endl;
            }
        }
        ++iterator;
    }
    
    // If no routes matched, the router drops the datagram
    if(is_found == false)
        return;

    // If the router is directly attached to the network in question, the next hop will be an empty optional. 
    // In that case, the next hop is the datagram’s destination address
    if(most_match_router.next_hop == std::nullopt) {
        interface(most_match_router.interface_num).send_datagram(dgram, Address::from_ipv4_numeric(dgram.header().dst));
    } else {
        interface(most_match_router.interface_num).send_datagram(dgram, most_match_router.next_hop.value());
    }

}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}

bool Router::_is_match(uint32_t ip, uint32_t refer_ip, uint8_t prefix_length) {
    int remain = 32 - static_cast<int>(prefix_length);
    uint32_t mask = (0xffffffff << remain);
    // default router
    if(prefix_length == 0)
        return true;
    // cerr << "remain : " << static_cast<int>(prefix_length) << " " << remain << endl;
    // cerr << hex << mask << " " << hex << (mask & ip) << " " << hex << (mask & refer_ip) << endl;
    if((mask & ip) == (mask & refer_ip))
        return true;
    else
        return false;
}