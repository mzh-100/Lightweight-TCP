#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::write_to_stream(Writer& writer, std::string& data, bool is_last_substring){
  if(writer.available_capacity() < data.size()) {
    data = data.substr(0, writer.available_capacity());
    is_last_substring = false;
  }
  output_.writer().push(data);
  next_index += data.size();
  if(is_last_substring) {
    output_.writer().close();
    return; //may be wrong
  }
  
 for (auto it = pending_strings.begin(); it != pending_strings.end(); ) {
    const auto& key = it->first;
    auto& value = it->second;
    
    if (key >= next_index) {
        break;
    }
    
    if (key + value.data.size() <= next_index) {
        it = pending_strings.erase(it);
    } else {        
        auto node = pending_strings.extract(it++); 
        std::string_view sv(node.mapped().data.data(), node.mapped().data.size());
        node.mapped().data = std::string(sv.substr(next_index - node.key()));
        node.key() = next_index;
        pending_strings.insert(std::move(node));
  
    }
}
  
  //If there is a data whose first byte match the next byte, keep inserting 
  while(pending_strings.contains(next_index) && writer.available_capacity()){
    string popped_data = pending_strings.at(next_index).data;
    if(writer.available_capacity() < popped_data.size()) {
      data = popped_data.substr(0, writer.available_capacity());
      std::string_view sv(popped_data.data(), popped_data.size());
      pending_strings.at(next_index).data = sv.substr(writer.available_capacity());
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
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  Writer& writer = output_.writer();
  auto avail = writer.available_capacity();
  if(!avail)
    return;
  if(next_index > first_index){
    if(first_index + data.size() <= next_index)
      return;
    else{
      data = data.substr(next_index - first_index);
      first_index = next_index;
    }
  }
  if(next_index == first_index) {
    write_to_stream(writer, data, is_last_substring);
  } else {
    if(first_index + data.size() > avail + next_index){
      data = data.substr(0, avail - first_index + next_index);
      is_last_substring = false;
    }
    bool b = handleOverlap(data, first_index);
    if(b)
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
bool Reassembler::handleOverlap(std::string& data, uint64_t first_index){
  uint64_t last_index = first_index + data.size();
  
  uint64_t i = next_index;
  // if(pending_strings.contains(i) && pending_strings.at(i).data.size() >= data.size()) {
  //   return;
  // }
  while (i < last_index){
    if(!pending_strings.contains(i)){
      i++;
      continue;
    }
    auto p = pending_strings.at(i);
    uint64_t c = p.data.size() + i;
    if (c < first_index){
      i = c;
      continue;
    }
    
    if( c < last_index ){      
      if(i < first_index){
        pending_strings.at(i).data = pending_strings.at(i).data.substr(0, first_index - i);
        i = c;
        continue;
      }
      pending_strings.erase(i);
      i = c;
      continue;
    }
    else{
      if (i <= first_index){
        return false;
      }
      data = data.substr(0, i - first_index);
      return true;
    }
  }
  return true;
}

