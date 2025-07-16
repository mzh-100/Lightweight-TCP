#pragma once
#include <unordered_map>
#include <map>
#include <vector>
#include "byte_stream.hh"
#define MAXINT 2^63

typedef struct packed_string{
  uint64_t first_index;
  std::string data;
  bool is_last_substring;

  packed_string( uint64_t first_index_, std::string data_, bool is_last_substring_ )
      : first_index( first_index_ ), data( std::move( data_ ) ), is_last_substring( is_last_substring_ ) {}
}packed_string;

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ) {
  }

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
  bool isOverlap( );
  // How many bytes are stored in the Reassembler itself?
  // This function is for testing only; don't add extra state to support it.
  uint64_t count_bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;
  uint64_t next_index {}; 
  std::map<uint64_t, packed_string> pending_strings {};
  
  // 辅助函数
  void write_to_stream(Writer& writer, std::string& data, bool is_last_substring);
  bool handleOverlap(std::string& data, uint64_t first_index);
};
