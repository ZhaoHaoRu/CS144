#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):buffer(), cap(capacity), begin(0), end(0), nowlen(0), _eof(false), _error(false) { 
    // DUMMY_CODE(capacity); 
}

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data);
    // cout << "cap: " << cap << endl;
    int len = data.length();
    nowlen += len;
    int result = end;
    //如果输入的长度使得buffer的长度大于capacity时，data多余的长度要截去
    if((end + len - begin) >= cap){
        buffer += data.substr(0, cap - (end - begin));
        end = begin + cap;
    }
    else{
        end = end + len;
        buffer += data;
    }
    result = end - result;
    // cout << "byte_stream write: " << begin << " " << end << " " << result << endl; 
    return  static_cast<size_t>(result);
    // return data.length();
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // cout << "get peek_output" << endl;
    // DUMMY_CODE(len);
    string result;
    size_t length = len;
    int tmp_len = length;
    // cout << "length: " << length << " cap: " << cap <<  " begin: " << begin << " end: " << end << endl;
    // if(length > cap || (begin + length) > end){
    //     // _error = true;
    //     return result;
    // }
    if(tmp_len > cap || (begin + tmp_len) > end){
        // _error = true;
        length = end - begin;
    }
    result = buffer.substr(begin, length);
    return result;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    // DUMMY_CODE(len); 
    int length = len;
    if((begin + length) > end || length > cap){
        _error = true;
        return;
    }
    begin += length;
    // cout << "read: " << begin << endl;
    if(input_ended())
        return;
    //注意end不要在此处更改
    // end = ((nowlen - begin) > cap) ? begin + cap : nowlen; 
}

void ByteStream::end_input() {
    _eof = true;
}

bool ByteStream::input_ended() const { 
    return _eof;
}

size_t ByteStream::buffer_size() const { 
    return (nowlen - begin) > cap ? cap : nowlen - begin;
}

bool ByteStream::buffer_empty() const { 
    return nowlen == begin;
}

bool ByteStream::eof() const {
    return begin == nowlen && _eof;
}

size_t ByteStream::bytes_written() const { 
    // cout << "written: " << end<< endl;
    return end;
}

size_t ByteStream::bytes_read() const { 
    return begin;
}

size_t ByteStream::remaining_capacity() const {
    //这里假设remaining_capacity是还可以读入的值
    int remain = cap - (end - begin);
    if(remain < 0)
        return 0;
    return remain;
 }