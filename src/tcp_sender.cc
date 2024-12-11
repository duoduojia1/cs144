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
      transmit( msg ); //这个一定是握手信号？
      abs_seqno_ += msg.sequence_length();
      total_outstanding_ += msg.sequence_length(); //syn 也是得有ack的
      break;
    }
    uint64_t len = std::min(static_cast<uint64_t>(TCPConfig::MAX_PAYLOAD_SIZE), cal_window_size_);
    input_.reader().pop( len );
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
  this->window_size_ = msg.window_size;
  if( msg.ackno.has_value() ){
    auto& ack = msg.ackno.value().get_raw_value();
    this->next_accpet_senum_ = Wrap32( ack );
    total_outstanding_ -= (msg.ackno.value().unwrap( isn_, checkpoint_ ) - 1UL ); 
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
