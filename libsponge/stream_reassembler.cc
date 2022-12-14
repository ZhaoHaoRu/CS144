#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :data_buffer(), stream_length(0),  _eof(false), unassembled(0), _output(capacity), _capacity(capacity), isExist(capacity * 20, false){
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    bool isSub = false;
    size_t left = index, len = data.length(), i = 0;
    // ofstream file("deubg.txt");
    // cout << "data input: " << data << " " << index << " " << eof << endl;
    if(len == 0){
        if(eof && unassembled == 0){
            _eof = eof;
            _output.end_input();
        }
        return;
    }
    size_t arr_length = isExist.size();
    for(i = index; i < len + index; ++i){
        // fix the bug: invalid pointer
        while(i > arr_length) {
            arr_length *= 2;
            isExist.resize(arr_length, false);
        }

        if(isExist[i]){
            isSub = true;
            if(i - left == 0){
                ++left;
                continue;
            }
            // cout << "write in: " << data.substr(left - index, i - left) << " " << left - index << " " << i << " " << left << endl;
            data_buffer[left] = data.substr(left - index, i - left);
            unassembled += i - left;
            left = i + 1;
        }
        else{
            isExist[i] = true;
        }
    }
    // cout << "after scan: " << left << " " << index + len << endl; 
    if(isSub && left != (len + index)){
        data_buffer[left] = data.substr(left - index, len + index - left);
        unassembled += len + index - left;
    }
    if(!isSub){
        data_buffer[index] = data;
        unassembled += len;
        // cout << "not sub: " << index << " " << len << endl; 
    }
    // cerr << "begin index before while:" << data_buffer.begin()->first << endl;
    // cerr << "write before while:" << data_buffer.begin()->second << '\n';
    // file << "begin index:" << data_buffer.begin()->first << '\n';
    while(data_buffer.begin()->first == stream_length){
        // cout << "write: " << data_buffer.begin()->first << " " << data_buffer.begin()->second << endl;
        size_t write_in = _output.write(data_buffer.begin()->second);
        // cerr << "begin index:" << data_buffer.begin()->first << endl;
        // cerr << "write:" << data_buffer.begin()->second << '\n';
        if(write_in != (data_buffer.begin()->second).length()) {
            size_t length = (data_buffer.begin()->second).length();
            size_t sub = length - write_in;
            size_t base = data_buffer.begin()->first;
            for(size_t k = 1; k <= sub; ++k) {
                isExist[base + length - k] = false;
            }  
        }
        data_buffer.erase(data_buffer.begin()->first);
        stream_length += write_in;
        // if(stream_out().bytes_written() != stream_length) {
        //     size_t true_written = stream_out().bytes_written();
        //     while(stream_length != true_written) {

        //     }
        // }
        unassembled -= write_in;
    }
    if(eof)
        _eof = eof;
    if(_eof && unassembled == 0)
        _output.end_input();
    
    

}

size_t StreamReassembler::unassembled_bytes() const { 
    return unassembled;
}

bool StreamReassembler::empty() const { 
    return unassembled == 0;
}
