#pragma once

#include "byte_stream.hh"
#include <map>
#include <queue>
#include <vector>
#include <utility>
#include <set>
#include <cstring>
#include <iostream>
const uint64_t inf = 2e18;
class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ), pending_size(0),
  byte_record(0), capacity_(output_.writer().get_capacity()),end_byte(0),is_end(false){}
  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;
  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }
 private:
    struct seg_node{
      uint64_t begin = 0;
      uint64_t siz = 0;
      std::string str;
      bool operator<(const seg_node &other) const {
        return this->begin < other.begin;
      };
    }; 
public:
  static long jug_block(seg_node& a, const seg_node& b);
  uint64_t get_byte_record() const { return byte_record; }
  bool writer_is_close() const { return output_.writer().is_closed(); }
private:
  ByteStream output_; // the Reassembler writes to this ByteStream
  uint64_t pending_size; 
  uint64_t byte_record;
  uint64_t capacity_;
  uint64_t end_byte;
  //uint64_t Max_capacity_stream; //?
  bool is_end;
  std::set<seg_node> st;
};
