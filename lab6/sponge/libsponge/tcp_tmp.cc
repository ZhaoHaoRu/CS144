// #include "tcp_sender.hh"

// #include "tcp_config.hh"
// #include "wrapping_integers.hh"

// #include <algorithm>
// #include <random>
// #include <string>

// // Dummy implementation of a TCP sender

// // For Lab 3, please replace with a real implementation that passes the
// // automated checks run by `make check_lab3`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

// using namespace std;

// //! \param[in] capacity the capacity of the outgoing byte stream
// //! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
// //! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
// TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
//     : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
//     , _initial_retransmission_timeout{retx_timeout}
//     , _stream(capacity)
//     , _retransmission_timeout(retx_timeout) {}

// uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

// void TCPSender::fill_window(bool send_syn) {
//     // sent a SYN before sent other segment
//     if (!_syn_flag) {
//         if (send_syn) {
//             TCPSegment seg;
//             seg.header().syn = true;
//             send_segment(seg);
//             _syn_flag = true;
//         }
//         return;
//     }

//     // take window_size as 1 when it equal 0
//     size_t win = _window_size > 0 ? _window_size : 1;
//     size_t remain;  // window's free space
//     // when window isn't full and never sent FIN
//     while ((remain = win - (_next_seqno - _recv_ackno)) != 0 && !_fin_flag) {
//         size_t size = min(TCPConfig::MAX_PAYLOAD_SIZE, remain);
//         TCPSegment seg;
//         string str = _stream.read(size);
//         seg.payload() = Buffer(std::move(str));
//         // add FIN
//         if (seg.length_in_sequence_space() < win && _stream.eof()) {
//             seg.header().fin = true;
//             _fin_flag = true;
//         }
//         // stream is empty
//         if (seg.length_in_sequence_space() == 0) {
//             return;
//         }
//         send_segment(seg);
//     }
// }

// //! \param ackno The remote receiver's ackno (acknowledgment number)
// //! \param window_size The remote receiver's advertised window size
// //! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
// bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
//     size_t abs_ackno = unwrap(ackno, _isn, _recv_ackno);
//     // out of window, invalid ackno
//     if (abs_ackno > _next_seqno) {
//         return false;
//     }

//     // if ackno is legal, modify _window_size before return
//     _window_size = window_size;

//     // ack has been received
//     if (abs_ackno <= _recv_ackno) {
//         return true;
//     }
//     _recv_ackno = abs_ackno;

//     // pop all elment before ackno
//     while (!_segments_outstanding.empty()) {
//         TCPSegment seg = _segments_outstanding.front();
//         if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
//             _bytes_in_flight -= seg.length_in_sequence_space();
//             _segments_outstanding.pop();
//         } else {
//             break;
//         }
//     }

//     fill_window();

//     _retransmission_timeout = _initial_retransmission_timeout;
//     _consecutive_retransmission = 0;

//     // if have other outstanding segment, restart timer
//     if (!_segments_outstanding.empty()) {
//         _timer_running = true;
//         _timer = 0;
//     }
//     return true;
// }

// //! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
// void TCPSender::tick(const size_t ms_since_last_tick) {
//     _timer += ms_since_last_tick;
//     if (_timer >= _retransmission_timeout && !_segments_outstanding.empty()) {
//         _segments_out.push(_segments_outstanding.front());
//         _consecutive_retransmission++;
//         _retransmission_timeout *= 2;
//         _timer_running = true;
//         _timer = 0;
//     }
//     if (_segments_outstanding.empty()) {
//         _timer_running = false;
//     }
// }

// // void TCPSender::tick(const size_t ms_since_last_tick) {
// //     _timecount += ms_since_last_tick;

// //     auto iter = _outgoing_map.begin();
// //     // 如果存在发送中的数据包，并且定时器超时
// //     if (iter != _outgoing_map.end() && _timecount >= _timeout) {
// //         // 如果窗口大小不为0还超时，则说明网络拥堵
// //         if (_last_window_size > 0)
// //             _timeout *= 2;
// //         _timecount = 0;
// //         _segments_out.push(iter->second);
// //         // 连续重传计时器增加
// //         ++_consecutive_retransmissions_count;
// //     }
// // }

// unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission; }

// void TCPSender::send_empty_segment() {
//     // empty segment doesn't need store to outstanding queue
//     TCPSegment seg;
//     seg.header().seqno = wrap(_next_seqno, _isn);
//     _segments_out.push(seg);
// }

// void TCPSender::send_empty_segment(WrappingInt32 seqno) {
//     // empty segment doesn't need store to outstanding queue
//     TCPSegment seg;
//     seg.header().seqno = seqno;
//     _segments_out.push(seg);
// }

// void TCPSender::send_segment(TCPSegment &seg) {
//     seg.header().seqno = wrap(_next_seqno, _isn);
//     _next_seqno += seg.length_in_sequence_space();
//     _bytes_in_flight += seg.length_in_sequence_space();
//     _segments_outstanding.push(seg);
//     _segments_out.push(seg);
//     if (!_timer_running) {  // start timer
//         _timer_running = true;
//         _timer = 0;
//     }
// }


// #include "tcp_connection.hh"

// #include <iostream>

// // Dummy implementation of a TCP connection

// // For Lab 4, please replace with a real implementation that passes the
// // automated checks run by `make check`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

// using namespace std;

// size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

// size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

// size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

// size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

// void TCPConnection::segment_received(const TCPSegment &seg) {
//     if (!_active)
//         return;
//     _time_since_last_segment_received = 0;

//     // data segments with acceptable ACKs should be ignored in SYN_SENT
//     if (in_syn_sent() && seg.header().ack && seg.payload().size() > 0) {
//         return;
//     }
//     bool send_empty = false;
//     if (_sender.next_seqno_absolute() > 0 && seg.header().ack) {
//         // unacceptable ACKs should produced a segment that existed
//         if (!_sender.ack_received(seg.header().ackno, seg.header().win)) {
//             send_empty = true;
//         }
//     }

//     bool recv_flag = _receiver.segment_received(seg);
//     if (!recv_flag) {
//         send_empty = true;
//     }

//     if (seg.header().syn && _sender.next_seqno_absolute() == 0) {
//         connect();
//         return;
//     }

//     if (seg.header().rst) {
//         // RST segments without ACKs should be ignored in SYN_SENT
//         if (in_syn_sent() && !seg.header().ack) {
//             return;
//         }
//         unclean_shutdown(false);
//         return;
//     }

//     if (seg.length_in_sequence_space() > 0) {
//         send_empty = true;
//     }

//     if (send_empty) {
//         if (_receiver.ackno().has_value() && _sender.segments_out().empty()) {
//             _sender.send_empty_segment();
//         }
//     }
//     push_segments_out();
// }

// bool TCPConnection::active() const { return _active; }

// size_t TCPConnection::write(const string &data) {
//     size_t ret = _sender.stream_in().write(data);
//     push_segments_out();
//     return ret;
// }

// //! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
// void TCPConnection::tick(const size_t ms_since_last_tick) {
//     if (!_active)
//         return;
//     _time_since_last_segment_received += ms_since_last_tick;
//     _sender.tick(ms_since_last_tick);
//     if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
//         unclean_shutdown(true);
//     }
//     push_segments_out();
// }

// void TCPConnection::end_input_stream() {
//     _sender.stream_in().end_input();
//     push_segments_out();
// }

// void TCPConnection::connect() {
//     // when connect, must active send a SYN
//     push_segments_out(true);
// }

// TCPConnection::~TCPConnection() {
//     try {
//         if (active()) {
//             // Your code here: need to send a RST segment to the peer
//             cerr << "Warning: Unclean shutdown of TCPConnection\n";
//             unclean_shutdown(true);
//         }
//     } catch (const exception &e) {
//         std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
//     }
// }

// bool TCPConnection::push_segments_out(bool send_syn) {
//     // default not send syn before recv a SYN
//     _sender.fill_window(send_syn || in_syn_recv());
//     TCPSegment seg;
//     while (!_sender.segments_out().empty()) {
//         seg = _sender.segments_out().front();
//         _sender.segments_out().pop();
//         if (_receiver.ackno().has_value()) {
//             seg.header().ack = true;
//             seg.header().ackno = _receiver.ackno().value();
//             seg.header().win = _receiver.window_size();
//         }
//         if (_need_send_rst) {
//             _need_send_rst = false;
//             seg.header().rst = true;
//         }
//         _segments_out.push(seg);
//     }
//     clean_shutdown();
//     return true;
// }

// void TCPConnection::unclean_shutdown(bool send_rst) {
//     _receiver.stream_out().set_error();
//     _sender.stream_in().set_error();
//     _active = false;
//     if (send_rst) {
//         _need_send_rst = true;
//         if (_sender.segments_out().empty()) {
//             _sender.send_empty_segment();
//         }
//         push_segments_out();
//     }
// }

// bool TCPConnection::clean_shutdown() {
//     if (_receiver.stream_out().input_ended() && !(_sender.stream_in().eof())) {
//         _linger_after_streams_finish = false;
//     }
//     if (_sender.stream_in().eof() && _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
//         if (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
//             _active = false;
//         }
//     }
//     return !_active;
// }

// bool TCPConnection::in_listen() { return !_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0; }

// bool TCPConnection::in_syn_recv() { return _receiver.ackno().has_value() && !_receiver.stream_out().input_ended(); }

// bool TCPConnection::in_syn_sent() {
//     return _sender.next_seqno_absolute() > 0 && _sender.bytes_in_flight() == _sender.next_seqno_absolute();
// }

// #ifndef SPONGE_LIBSPONGE_ETHERNET_FRAME_HH
// #define SPONGE_LIBSPONGE_ETHERNET_FRAME_HH

// #include "buffer.hh"
// #include "ethernet_header.hh"

// //! \brief Ethernet frame
// class EthernetFrame {
//   private:
//     EthernetHeader _header{};
//     BufferList _payload{};

//   public:
//     //! \brief Parse the frame from a string
//     ParseResult parse(const Buffer buffer);

//     //! \brief Serialize the frame to a string
//     BufferList serialize() const;

//     //! \name Accessors
//     //!@{
//     const EthernetHeader &header() const { return _header; }
//     EthernetHeader &header() { return _header; }

//     const BufferList &payload() const { return _payload; }
//     BufferList &payload() { return _payload; }
//     //!@}
// };

// #endif  // SPONGE_LIBSPONGE_ETHERNET_FRAME_HH