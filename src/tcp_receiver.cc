#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if(!message.RST) return;
  uint64_t byte_record= reassembler_.get_byte_record();
  if( message.FIN ) { set_offset_rand( message.seqno ); }
  uint64_t seq_no = message.FIN ? ( message.seqno + 1 ).unwrap( get_offset_rand(), byte_record ) : ( message.seqno ).unwrap( get_offset_rand(), byte_record );
  reassembler_.insert( seq_no, message.payload, message.FIN );
  //(void)message;
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage tcp_rec_msg{};
  tcp_rec_msg.window_size = static_cast<uint16_t>(std::min( writer().available_capacity(), 65535UL ));
  tcp_rec_msg.ackno = Wrap32(reassembler_.get_byte_record());
  tcp_rec_msg.RST = true;
  return tcp_rec_msg;
}
