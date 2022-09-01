#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received_ms; }

void TCPConnection::segment_received(const TCPSegment &seg) {   // refer some blogs 
    _time_since_last_segment_received_ms = 0;

    // a segment is accetable only if syn is received
    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::LISTEN && !seg.header().syn && seg.header().ack){ 
        if(seg.header().rst){
            unclean_shutdown(false);
        }
        return;
    } 
    

    // data segments with acceptable ACKs should be ignored in SYN_SENT
    if(TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_SENT && seg.header().ack && seg.payload().size() > 0)
        return;
    
    // if exception incur, send empty segment?
    bool sent_empty = false;

    // _receiver.segment_received(seg);
    bool result = _receiver.segment_received(seg);
    if(!result)
        sent_empty = true;

    // if rst, stop immediatly
    if(seg.header().rst) {
        // RST segments without ACKs should be ignored in SYN_SENT (why???)
        if(TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_SENT && seg.header().ack) {
            return;
        }
        unclean_shutdown(false);
        return;
    }

    // if receive ack, update sender
    // tell the TCPSender about the fields it cares about on incoming segments: ackno and window size
    if(seg.header().ack) {
        bool ret = _sender.ack_received(seg.header().ackno, seg.header().win);
        // unacceptable ACKs should produced a segment that existed (why???)
        if(_sender.next_seqno_absolute() > 0 && seg.header().ack && !ret) {
            sent_empty = true;
        }
    }

    // if the segment has received has ack, don't have to send ack again
    if(seg.length_in_sequence_space() > 0)
        sent_empty = true;

    // if listen the syn
    if(TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED && 
        TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV) {
        connect();
        return;
    }
    
    // judge whether has to wait when tear down
    // CLOSE_WAIT
    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_ACKED) {
        _linger_after_streams_finish = false;
    }

    // CLOSED
    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && !_linger_after_streams_finish) {
        _is_active = false;
    }

    if(sent_empty) {
        if(_receiver.ackno().has_value() && _sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
    }

    push_segments_out();

}

bool TCPConnection::active() const { return _is_active; }

size_t TCPConnection::write(const string &data) {
    size_t result = _sender.stream_in().write(data);
    _sender.fill_window();
    push_segments_out();
    return result;
}

void TCPConnection::push_segments_out() {
    // Add the local ackno and window size to the packets waiting to be sent
    while(!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if(_receiver.ackno().has_value()) {
            seg.header().ackno = _receiver.ackno().value();
            seg.header().ack = true;
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(seg);
    }
    clean_shutdown();
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    if(!_is_active)
        return;
    // TODO: order?
    _sender.tick(ms_since_last_tick);
    _time_since_last_segment_received_ms += ms_since_last_tick;

    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        // send a segment with the rst flag set
        unclean_shutdown(true);
        return;
    }

    // Forward package that may be retransmitted
    // default not send syn before recv a SYN
    if(!(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::LISTEN &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED)) {
        _sender.fill_window();
        push_segments_out();
    }


    // if in `time wait` and timeout, then close
    if(TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && 
        TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV && (_linger_after_streams_finish &&
        time_since_last_segment_received() >= 10 * _cfg.rt_timeout)) {
        _is_active = false;
        _linger_after_streams_finish = false;
    }

 }

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    // send fin immediately
    _sender.fill_window();
    push_segments_out();
}

void TCPConnection::connect() {
    // when first call `fill_window`, will send a package with syn
    _sender.fill_window();
    _is_active = true;
    push_segments_out();
}

void TCPConnection::unclean_shutdown(bool send_rst) {
    // the outbound and inbound ByteStreams should both be in the error state
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    // active() can return false immediately
    _is_active = false;
    _linger_after_streams_finish = false;
    // the TCPConnection either sends or receives a segment with the rst flag set
    if(send_rst) {
        TCPSegment rst_seg;
        rst_seg.header().rst = true;
        _segments_out.push(rst_seg);
        if(_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
    }
}

bool TCPConnection::clean_shutdown() {
    // If the inbound stream ends before the TCPConnection reached EOF on its outbound stream, 
    // _linger_after_streams_finish, needs to be set to false
    if(_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    // The inbound stream has been fully assembled and has ended
    // The outbound stream has been fully acknowledged by the remote peer
    // _linger_after_streams_finish is false
    // if satisfy above, active() should return false
    // Otherwise need to linger: the connection is only done after enough time (10 × cfg.rt timeout) has elapsed since the last segment was received.
    if(_receiver.stream_out().input_ended() && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0) {
        if(!_linger_after_streams_finish) {
            _is_active = false;
        }
        else if(time_since_last_segment_received() > 10 * _cfg.rt_timeout) {
            _is_active = false;
        }
    }
    return !_is_active;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            // cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            unclean_shutdown(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
