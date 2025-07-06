#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  Writer& writer = output_.writer();
  if(!writer.available_capacity())
    return;
  if(next_index == first_index) {
    if(writer.available_capacity() < data.size()) {
      data = data.substr(0, writer.available_capacity());
    }
    output_.writer().push(data);
    next_index += data.size();
    if(is_last_substring) {
      output_.writer().close();
    }
    //If there is a data whose first byte match the next byte, keep inserting 
    while(pending_strings.contains(next_index) && writer.available_capacity()){
      string popped_data = pending_strings.at(next_index).data;
      if(writer.available_capacity() < data.size()) {
        data = popped_data.substr(0, writer.available_capacity());
        pending_strings.at(next_index).data = popped_data.substr(writer.available_capacity());
      }
      else{
        packed_string& p = pending_strings.at(next_index);
        data = p.data;
        is_last_substring = p.is_last_substring;
        pending_strings.erase(next_index);
      }      
      output_.writer().push(data);
      next_index += data.size();
       if(is_last_substring) {
        output_.writer().close();
      }
    }
  } else if(first_index > next_index) {
    if(pending_strings.contains(first_index))
      return;
    if(first_index + data.size() - next_index > writer.available_capacity()){
      int i = first_index + data.size() - next_index - writer.available_capacity();
      data = data.substr(0, i);
    }
    pending_strings.emplace(first_index, packed_string(first_index, data, is_last_substring));
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t stored_size = 0;
  //debug( "unimplemented count_bytes_pending() called" );
  for(const auto& p : pending_strings){
    stored_size += p.second.data.size();
  }
  return stored_size;
}

bool isOverlap(){
  
}
