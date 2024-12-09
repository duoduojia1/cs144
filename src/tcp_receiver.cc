#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  // if(!message.RST) return;
  if( message.RST ) {
    reader().set_error();
    return;
  } 
  uint64_t byte_record= reassembler_.get_byte_record();
  if( message.SYN ) {
    set_offset_rand( message.seqno );
    has_syn = true;
  }
  uint64_t seq_no =  message.SYN ? Wrap32( message.seqno.get_raw_value() ).unwrap( get_offset_rand(), byte_record ) 
  : Wrap32( message.seqno.get_raw_value() - 1 ).unwrap( get_offset_rand(), byte_record );
  reassembler_.insert( seq_no, message.payload, message.FIN );
  //(void)message;
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage tcp_rec_msg{};
  tcp_rec_msg.window_size = static_cast<uint16_t>(std::min( writer().available_capacity(), 65535UL ));
  if ( get_syn() ) { tcp_rec_msg.ackno = Wrap32( get_offset_rand() + reassembler_.get_byte_record() + 1 + (reassembler_.writer_is_close() == true) ); };
  tcp_rec_msg.RST = reader().has_error() ? true : false;
  return tcp_rec_msg;
}
