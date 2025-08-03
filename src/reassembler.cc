#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  size_t _first_unassembled = output_.writer().bytes_pushed();
  size_t _first_unaccept = _first_unassembled + _capacity;
  // Handle the special case of an empty final substring
  if (data.empty() && is_last_substring) {
    _is_eof = true;
    _eof_index = first_index;
    // If the stream is already assembled up to EOF, close the stream
    if (_first_unassembled == _eof_index) {
      output_.writer().close();
    }
    return;
  }
  // If the data is completely out of range, return directly
  if(first_index >= _first_unaccept || first_index + data.size() <= _first_unassembled)
  {
    // If this is the last substring, record the EOF position
    if(is_last_substring) {
      _is_eof = true;
      _eof_index = first_index + data.size();
    }
    return;
  }
  
  size_t begin_index = max(first_index, _first_unassembled);
  size_t end_index = min(first_index + data.size(), _first_unaccept);
  for(size_t i = begin_index; i < end_index; i++)
  {
    if (!_flag[i - _first_unassembled]) {
      buffer[i - _first_unassembled] = data[i - first_index];
      _flag[i - _first_unassembled] = true;
      _unassembled_bytes++;
    }
  }

  // Compute how many consecutive bytes from the front are ready
  size_t count = 0;
  for (size_t i = 0; i < _flag.size() && _flag[i]; ++i) {
    ++count;
  }

  if (count > 0) {
    std::string intermediate_data(buffer.begin(), buffer.begin() + count);
    output_.writer().push(intermediate_data);
    _unassembled_bytes -= count;

    buffer.erase(buffer.begin(), buffer.begin() + count);
    _flag.erase(_flag.begin(), _flag.begin() + count);

    buffer.insert(buffer.end(), count, '\0');
    _flag.insert(_flag.end(), count, false);
  }

  if(is_last_substring)
  {
    _is_eof = true;
    _eof_index = first_index + data.size();  // Use the end position of the original data
  }
  // Only close the stream when all data has been processed
  if(_is_eof && output_.writer().bytes_pushed() == _eof_index)
  {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return _unassembled_bytes;
}

