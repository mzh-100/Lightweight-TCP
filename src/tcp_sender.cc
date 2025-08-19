#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include <iostream>

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return outstanding_bytes;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_nums;
}


void TCPSender::push(const TransmitFunction& transmit)
{
  // Check if writer has error and set error state
  if (writer().has_error()) {
    _has_error = true;
  }

  if (_has_error) {
    TCPSenderMessage rst_msg = make_empty_message();
    transmit(rst_msg);
    return;
  }
  
  // Calculate effective window size (use 1 if window is 0 and no outstanding data)
  uint64_t effective_window = (received_msg.window_size == 0 && outstanding_bytes == 0) ? 1 : received_msg.window_size;
  
  while (outstanding_bytes < effective_window) {
    TCPSenderMessage msg;
    if (!isSent_ISN) {
      msg.SYN = true;
      msg.seqno = isn_;
      isSent_ISN = true;
    } else {
      msg.seqno = Wrap32::wrap(abs_seqno, isn_);
    }
    
    // Calculate remaining window space
    size_t remaining_window = effective_window - outstanding_bytes;
    // SYN flag consumes one sequence number
    if (msg.SYN) {
      remaining_window = remaining_window > 0 ? remaining_window - 1 : 0;
    }
    
    // Calculate payload size to send
    size_t payload_size = min(remaining_window, TCPConfig::MAX_PAYLOAD_SIZE);
    payload_size = min(payload_size, writer().reader().bytes_buffered());
    
    // Read data from writer
    read(writer().reader(), payload_size, msg.payload);
    
    // Add FIN flag only when all data is sent and FIN fits in window
    if (writer().is_closed() && !isSent_FIN && 
        writer().reader().bytes_buffered() == 0 && 
        outstanding_bytes + msg.sequence_length() < effective_window) {
      isSent_FIN = true;
      msg.FIN = true;
    }
    
    if (!msg.sequence_length()) {
      break;
    }

    outstanding_collections.push_back(msg);
    outstanding_bytes += msg.sequence_length();
    abs_seqno += msg.sequence_length();
    
    // Send the message
    transmit(msg);
    
    // Start timer if there's outstanding data
    if (outstanding_bytes > 0 && !is_start_timer) {
      is_start_timer = true;
      cur_RTO_ms = initial_RTO_ms_;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap(abs_seqno, isn_);
  
  // Set RST flag if there's any error
  if (_has_error || writer().has_error()) {
    msg.RST = true;
  }
  
  return msg;
}

void TCPSender::receive(const TCPReceiverMessage& msg)
{
  // Handle RST flag
  if (msg.RST) {
    _has_error = true;
    const_cast<Writer&>(writer()).set_error();
    return;
  }

  if (_has_error) {
    return;
  }
  
  received_msg = msg;
  primitive_window_size = msg.window_size;
  
  if (msg.ackno.has_value()) {
    uint64_t ackno_unwrapped = msg.ackno.value().unwrap(isn_, abs_seqno);
    if (ackno_unwrapped > abs_seqno) {
      return;
    }
    
    // Remove acknowledged segments
    while (outstanding_bytes > 0 && 
           outstanding_collections.front().seqno.unwrap(isn_, abs_seqno) + 
           outstanding_collections.front().sequence_length() <= ackno_unwrapped) {
      outstanding_bytes -= outstanding_collections.front().sequence_length();
      outstanding_collections.pop_front();
      consecutive_retransmissions_nums = 0;
      cur_RTO_ms = initial_RTO_ms_;
      is_start_timer = (outstanding_bytes > 0);
    }
  }
}

void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit)
{
  if (_has_error) {
    return;
  }
  
  // Only process timeout if timer is active and there's outstanding data
  if (is_start_timer && outstanding_bytes > 0) {
    if (cur_RTO_ms <= ms_since_last_tick) {
      // Timeout: retransmit the first unacknowledged segment
      transmit(outstanding_collections.front());
      consecutive_retransmissions_nums++;
      
      // Exponential backoff (only if window size > 0)
      if (primitive_window_size > 0) {
        cur_RTO_ms = (1UL << consecutive_retransmissions_nums) * initial_RTO_ms_;
      } else {
        cur_RTO_ms = initial_RTO_ms_;
      }
    } else {
      cur_RTO_ms -= ms_since_last_tick;
    }
  }
}