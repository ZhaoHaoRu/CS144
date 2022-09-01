#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timeout{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { 
    return _bytes_in_flight;
}

void TCPSender::fill_window() {
    // send syn before send other segment
    if(!_has_syn_before) {
        TCPSegment seg;
        seg.header().syn = true;
        send_segment(seg);
        _has_syn_before = true;
    }  

    // send as many as many bytes as possible in form of TCPSegments
    size_t win_size = _window_size == 0 ? 1 : _window_size;
    while(win_size + _ack_seqno > _next_seqno && !_has_fin_before) {
        TCPSegment seg;
        size_t size = min(win_size - (_next_seqno - _ack_seqno), TCPConfig::MAX_PAYLOAD_SIZE);
        string buffer = _stream.read(size);
        seg.payload() = Buffer(move(buffer));   // use move instead of copy
        if(seg.length_in_sequence_space() < win_size && _stream.eof() && !_has_fin_before) {
            seg.header().fin = true;
            _has_fin_before = true;
        }

        // TODO: if segment is empty
        if(seg.length_in_sequence_space() == 0){
            return;
        }
        send_segment(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // ARQ protocol based on "cumulative acknowledgement", that is, the sender receives an ackno, 
    // which means that the receiver has received all the segments before the ackno
    uint64_t abs_ackno = unwrap(ackno, _isn, _ack_seqno);
    cout << "abs_ackno: " << abs_ackno << " " << _next_seqno << " " << _ack_seqno << endl;
    // if the ackno appears invalid
    if(abs_ackno > _next_seqno) {
        return false;
    }
    // if the ackno has been acknowledged before
    if(abs_ackno < _ack_seqno || abs_ackno == 0) {
        return true;
    }

    // the receiver has received all the segments before the ackno
    _ack_seqno = abs_ackno;
    _window_size = window_size;
    cout << "window_size: " << _window_size << endl; 
    while(!_segments_outstanding.empty()) {
        TCPSegment seg = _segments_outstanding.front();
        // why checkpoint is '_next_seqno', I don't understand
        cout << "the deleted segment seg_no: " << unwrap(seg.header().seqno, _isn, _next_seqno) << endl;
        if(unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
            _segments_outstanding.pop();
            _bytes_in_flight -= seg.length_in_sequence_space();
            cout << "_bytes_in_flight ack: " << _bytes_in_flight << endl;
        } else {
            break;
        }
    }

    fill_window();

    _retransmission_timeout = _initial_retransmission_timeout;
    if(!_segments_outstanding.empty()) {
        _alarm_started = true;
        _retransmission_timer = 0;
    } else {
        // When all outstanding data has been acknowledged, turn off the retransmission timer
        _alarm_started = false; 
    }
    _consecutive_retransmission = 0;
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _retransmission_timer += ms_since_last_tick;
    if (_retransmission_timer >= _retransmission_timeout) {
        if(!_segments_outstanding.empty()){
            _segments_out.push(_segments_outstanding.front());
        }

        // according to the doc, but not correct 
        // if(_window_size != 0) {
        //     ++_consecutive_retransmission;
        //     _retransmission_timeout *= 2;
        // }

        ++_consecutive_retransmission;
        _retransmission_timeout *= 2;

        _retransmission_timer = 0;
        _alarm_started = true;
    }
 }

unsigned int TCPSender::consecutive_retransmissions() const { 
    return _consecutive_retransmission; 
}

// This is useful if the owner (the TCPConnection that you’re going to
// implement next week) wants to send an empty ACK segment
void TCPSender::send_empty_segment() {
    TCPSegment tcp_Segment;
    tcp_Segment.header().ack = true;
    tcp_Segment.header().seqno = next_seqno();
    _segments_out.push(tcp_Segment);
}


 void TCPSender::send_segment(TCPSegment &seg) {
    seg.header().seqno = next_seqno();
    _next_seqno += seg.length_in_sequence_space();
    _bytes_in_flight += seg.length_in_sequence_space();
    cout << "_bytes_in_flight: " << _bytes_in_flight << endl;
    _segments_outstanding.push(seg);
    _segments_out.push(seg);
    if(!_alarm_started) {
        _alarm_started = true;
        _retransmission_timer = 0;
    }
 }