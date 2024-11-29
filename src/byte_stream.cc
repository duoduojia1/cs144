#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return is_close_;
}

void Writer::push( string data )
{
  // Your code here.
  if ( data.size() > capacity_ - cur_capacity_ ) {
    data = data.substr( 0, capacity_ - cur_capacity_ );
  }
  // std::cout << data.size() << std::endl;
  for ( auto& c : data )
    _dq.push_back( c );
  cur_capacity_ += data.size();
  writer_size_ += data.size();
  (void)data;
  return;
}

void Writer::close()
{
  // Your code here.
  is_close_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - cur_capacity_;
}

uint64_t Writer::bytes_pushed() const
{
 // Your code here.
  return writer_size_;
}

bool Reader::is_finished() const
{
   // Your code here.
  return ( !cur_capacity_ ) && ( is_close_ );
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return reader_size_;
}

string_view Reader::peek() const
{
   // Your code here.
  std::string_view view( &_dq.front(), 1 );
  return view;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  //(void)len;
  len = std::min( _dq.size(), len );
  for ( size_t i = 0; i < len; i++ ) {
    auto& p = _dq.front();
    (void)p;
    _dq.pop_front();
  }
  reader_size_ += len;
  cur_capacity_ -= len;
  return;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return cur_capacity_;
}
