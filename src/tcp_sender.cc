#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return total_outstanding_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return {};
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  
  uint64_t cal_window_size_ = ( window_size_ == 0 ? 1 : window_size_ );
  while( cal_window_size_ - total_outstanding_ > 0 ){
    TCPSenderMessage&& msg = make_empty_message();
    if(not is_syn_) {
      is_syn_ = true;
      transmit( msg ); //这个一定是握手信号。
      abs_seqno_ += msg.sequence_length();
      total_outstanding_ += msg.sequence_length(); //syn 也是得有ack的。
      break;
    }
    //必须要握手成功了，才可以进行发送消息
    if( next_accpet_senum_ > 0UL ){
      if( not input_.reader().bytes_buffered() ) break; // 如果没有内容的话，即便有窗口也要break
      uint64_t len = std::min(static_cast<uint64_t>(TCPConfig::MAX_PAYLOAD_SIZE), cal_window_size_ - total_outstanding_ );
      len = std::min( len, input_.reader().bytes_buffered() );//读buffer中还有多少字节
      for( uint64_t byte = 0; byte < len ; ++byte ) {
        msg.payload += std::string( input_.reader().peek() );
        input_.reader().pop( 1UL ); //把内容都读出来..
      }
      abs_seqno_ += msg.sequence_length();
      total_outstanding_ += msg.sequence_length();
      transmit( msg );
    }
  } 
  // (void)transmit;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  return TCPSenderMessage{ Wrap32( 0 ).wrap( abs_seqno_ , isn_ ), !is_syn_, "", is_fin_, false };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if( msg.ackno.has_value() ){
    auto ack = msg.ackno.value().unwrap( isn_, checkpoint_ );
    if( ack < next_accpet_senum_  || not ack ) return; //ack一定比之前的akc要更新，不能说拿旧的ack来更新窗口。或者就是一个错误的消息
    //比如是0，因为期望的ack至少是1，不然不更新窗口。
    this->window_size_ = msg.window_size;
    this->next_accpet_senum_ = ack;
    //total_outstanding_ -= ( ack - 1UL );
    total_outstanding_ = abs_seqno_ - ack; //取差值就是还未收到ack的序列。
    checkpoint_ = msg.ackno.value().unwrap( isn_, checkpoint_); 
  } 

  (void)msg;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;
}
