#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if(message.RST){
    reassembler_.reader().set_error();
  }
  if(message.SYN){
    this->ISN = message.seqno;
    start = true;
  }
  auto abs_seqno = message.seqno.unwrap(this->ISN, reassembler_.writer().bytes_pushed());
  // if(message.payload.size())
  if(!message.SYN)
    abs_seqno--;
  reassembler_.insert(abs_seqno, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage message;
  auto i = reassembler_.writer().bytes_pushed() + 1;
  if(reassembler_.writer().is_closed())
    i++;
  if(start)
    message.ackno = Wrap32::wrap(i, ISN);
  auto cap = reassembler_.writer().available_capacity();
  message.window_size = cap > UINT16_MAX ? UINT16_MAX : cap;
  if(reassembler_.reader().has_error())
    message.RST = true;
  return message;
}
