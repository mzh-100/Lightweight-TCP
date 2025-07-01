#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  if ( closed_ ) {
    return;
  }
  if ( data.empty() ) {
    return;
  }
  if ( data.size() <= available_capacity() ) {
    streambuf.push_back( data );
    pushed_bytes += data.size();
    size_ += data.size();
  } else {
    // Handle the case where data exceeds available capacity.
    // You might want to push only as much as fits or throw an error.
    uint64_t available = available_capacity();
    if ( available > 0 ) {
      streambuf.push_back( data.substr( 0, available ) );
      pushed_bytes += available;
      size_ += available;
    }
  }
  // if(!available_capacity()) {
  //   close(); // If no capacity left, mark as closed.
  // }
}

void Writer::close()
{
  closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  // cout << capacity_ << " " << size_ << endl;
  return capacity_ - size_;
}

uint64_t Writer::bytes_pushed() const
{
  return pushed_bytes;
}

string_view Reader::peek() const
{
  return string_view( streambuf.front() ); // Your code here.
}

void Reader::pop( uint64_t len )
{
  if ( streambuf.empty() ) {
    return;
  }
  string buf = streambuf.front();
  streambuf.pop_front();
  if ( len < buf.size() ) {
    streambuf.push_front( buf.substr( len ) );
    size_ -= len;
    poped_bytes += len;
    return;
  }
  size_ -= buf.size();
  poped_bytes += buf.size();
  len -= buf.size();
  while ( len > 0 && !streambuf.empty() ) {
    buf = streambuf.front();
    streambuf.pop_front();
    if ( len < buf.size() ) {
      streambuf.push_front( buf.substr( len ) );
      size_ -= len;
      poped_bytes += len;
      return;
    }
    size_ -= buf.size();
    poped_bytes += buf.size();
    len -= buf.size();
  }
  return;
}

bool Reader::is_finished() const
{
  return ( !size_ ) && ( closed_ ); // Your code here.
}

uint64_t Reader::bytes_buffered() const
{
  return size_; // Your code here.
}

uint64_t Reader::bytes_popped() const
{
  return poped_bytes; // Your code here.
}
