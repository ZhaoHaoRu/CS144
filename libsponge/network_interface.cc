#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    // if the destination Ethernet address is already known
    auto arp = _ARP_table.find(next_hop_ip);
    if(arp != _ARP_table.end()) {
        // create a ethernet frame
        EthernetHeader header;
        header.dst = (arp->second)._mac;
        header.src = _ethernet_address;
        header.type = EthernetHeader::TYPE_IPv4;
        // BufferList payload = dgram.serialize();
        EthernetFrame frame;
        frame.header() = header;
        frame.payload() = dgram.serialize();
        _frames_out.push(frame);
        return;
    } else {
        // the destination Ethernet address is unknown and haven't been found
        // If the network interface already sent an ARP request about the same IP address in the last five seconds, don’t send a second request—just wait for a reply to the first one
        if(_waiting_ip_and_time.find(next_hop_ip) == _waiting_ip_and_time.end()) {
            // broadcast an ARP request for the next hop’s Ethernet address
            EthernetHeader header{ETHERNET_BROADCAST, _ethernet_address, EthernetHeader::TYPE_ARP};
            ARPMessage message;
            message.opcode = ARPMessage::OPCODE_REQUEST;
            message.sender_ethernet_address = _ethernet_address;
            message.sender_ip_address = _ip_address.ipv4_numeric();
            message.target_ethernet_address = {};
            message.target_ip_address = next_hop_ip;
            // BufferList payload = BufferList(message.serialize());
            EthernetFrame frame;
            frame.header() = header;
            frame.payload() = BufferList(message.serialize());
            // add to the outbound queue
            _frames_out.push(frame);
            // write down the time passed since send ARP
            _waiting_ip_and_time[next_hop_ip] = _timer;
        }
        // add the IPv4 datagram to the waiting list
        _frames_waiting.push_back(WaitingFrame(next_hop, dgram));
        return; 
    }
    
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // ignore any frames not destined for the network interface
    EthernetHeader header = frame.header();
    if(header.dst != ETHERNET_BROADCAST && header.dst != _ethernet_address) {
        return {};
    }
    // If the inbound frame is IPv4
    if(header.type == EthernetHeader::TYPE_IPv4) {
        // parse the payload as an InternetDatagram
        InternetDatagram datagram;
        if(datagram.parse(frame.payload()) == ParseResult::NoError) {
            return {datagram};
        }
    } else if(header.type == EthernetHeader::TYPE_ARP) {    // If the inbound frame is ARP
        // parse the payload as an ARPMessage
        // mapping between the sender's IP 
        ARPMessage message;
        if(message.parse(frame.payload()) == ParseResult::NoError) {
            bool is_ask_localhost = message.opcode == ARPMessage::OPCODE_REQUEST && message.target_ip_address == _ip_address.ipv4_numeric();
            EthernetAddress addr = message.sender_ethernet_address;
            uint32_t ip = message.sender_ip_address;
            _ARP_table[ip] = ARP{addr, _timer};
            if(is_ask_localhost) {
                // if ask for the host's mac
                if(message.target_ip_address == _ip_address.ipv4_numeric()) {
                    message.opcode = ARPMessage::OPCODE_REPLY;
                    message.target_ethernet_address = message.sender_ethernet_address;
                    message.target_ip_address = message.sender_ip_address;
                    message.sender_ip_address = _ip_address.ipv4_numeric();
                    message.sender_ethernet_address = _ethernet_address;
                    // BufferList payload = BufferList(message.serialize());
                    EthernetHeader ethernetHeader{message.target_ethernet_address, message.sender_ethernet_address, EthernetHeader::TYPE_ARP};
                    EthernetFrame ethernetFrame;
                    ethernetFrame.header() = ethernetHeader;
                    ethernetFrame.payload() = BufferList(message.serialize());
                    // cerr << "ask for the host's mac: " << message.sender_ip_address << " _mac: " << to_string(message.sender_ethernet_address) << endl;
                    // add to the outbound queue
                    _frames_out.push(ethernetFrame);
                } 
            } 
    
            // send those datagram which can find mac in the waiting list
            auto iterator = _frames_waiting.begin();
            while(iterator != _frames_waiting.end())
            {
                if((iterator->_ip).ipv4_numeric() == ip) {
                    send_datagram(iterator->_datagram, iterator->_ip);
                    iterator = _frames_waiting.erase(iterator);
                } else {
                    ++iterator;
                }
            } 
            _waiting_ip_and_time.erase(ip);
                
        
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // the number of milliseconds since the last call to this method
    _timer += ms_since_last_tick;
    // Expire any IP-to-Ethernet mappings that have expired
    auto itr = _ARP_table.begin();
    while(itr != _ARP_table.end()) {
        ARP arp = itr->second;
        if(arp._ttl + _ARP_expire_time <= _timer) {
            itr = _ARP_table.erase(itr);
        } else {
            itr++;
        }
    }
    // retranmission the ARP that haven't received the ARP response yet
    auto iter = _waiting_ip_and_time.begin();
    while(iter != _waiting_ip_and_time.end()) {
        if(_timer - iter->second >= _expire_time) {
            EthernetHeader header{ETHERNET_BROADCAST, _ethernet_address, EthernetHeader::TYPE_ARP};
            ARPMessage message;
            message.opcode = ARPMessage::OPCODE_REQUEST;
            message.sender_ethernet_address = _ethernet_address;
            message.sender_ip_address = _ip_address.ipv4_numeric();
            message.target_ethernet_address = {};
            message.target_ip_address = iter->first;
            // BufferList payload = BufferList(message.serialize());
            EthernetFrame frame;
            frame.header() = header;
            frame.payload() = BufferList(message.serialize());
            // add to the outbound queue
            _frames_out.push(frame);
            // write down the time passed since send ARP
            _waiting_ip_and_time[message.target_ip_address] = _timer;
        }
        ++iter;
    }
    
}
