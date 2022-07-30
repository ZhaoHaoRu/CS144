#include "tcp_receiver.hh"
#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}


using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header = seg.header();
    bool syn = header.syn;
    bool fin = header.fin;
    int debug = 0;
    // cout << "header.seqno.raw_value(): " << header.seqno.raw_value() << endl;

    // set the initial sequence number if necessary
    if(syn && !_has_SYN_before) {
        _init_seq_number = header.seqno.raw_value();
        _has_SYN_before = true;
        abs_seqno = 1;
        _base = 1;
    } else if(!_has_SYN_before) {   // haven't receive syn, throw the segment
        return false;
    } else if(syn) {
        return false;
    } else { // after syn
        abs_seqno = unwrap(header.seqno, WrappingInt32(_init_seq_number), abs_seqno);
    }   

    if(fin) {
        if(_has_FIN_before)
            return false;
        _has_FIN_before = true;
    }
    
    // determine if any part of the segment falls inside the window
    size_t seq_length = seg.length_in_sequence_space();
    if(seq_length == 0 && abs_seqno == _base)
        return true;
    if(seq_length == 0)
        seq_length = 1;
    // calculate the left and right of the sequence
    size_t left_seq = abs_seqno;
    // 此处的假设是fin和syn不会写入_reassembler
    size_t right_seq = abs_seqno + seq_length - 1;
    if(syn)
        --right_seq;
    if(fin)
        --right_seq;

    // If the window has size zero, then its size should be treated as one byte
    size_t win_size = window_size() == 0 ? 1 : window_size();
    // calculate the left and right of the window
    size_t left_window = _base;
    size_t right_window = win_size + left_window - 1;
    // std::cout << "left_seq: " << left_seq << " right_seq: " << right_seq << " left_window: " << left_window << " right_window: " << right_window << std::endl;
    if((left_seq > right_window || right_seq < left_window) && !fin && !syn)
        return false;
    
    // push any data, or end-of-stream marker, to the StreamReassembler
    Buffer payload = seg.payload();
    std::string data = payload.copy();
    // judge EOF
    if(fin) {
        _reassembler.push_substring(data, abs_seqno - 1, true);
    } else {
        _reassembler.push_substring(data, abs_seqno - 1, false);
    }

    _base = _reassembler.stream_out().bytes_written() + 1;
    if(fin) 
        ++_base;
    return true;
}

std::optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!_has_SYN_before)
        return {};
    else
        return {wrap(_base, WrappingInt32(_init_seq_number))};
}

size_t TCPReceiver::window_size() const { 
    return _reassembler.stream_out().remaining_capacity();
}
