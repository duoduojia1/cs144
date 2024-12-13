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
  return retransmissions;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if( end_ ) return; //防止结束后，又push
  // Your code here.
  if( input_.writer().is_closed() ) {
      is_fin_ = true;//应该是writer不在写了.. //代表已经fin过了，只要最后fin的ack到了，那么结束了。
    }
  uint64_t cal_window_size_ = ( window_size_ == 0 ? 1 : window_size_ );
  while( cal_window_size_ - total_outstanding_ > 0 ) {
    TCPSenderMessage&& msg = make_empty_message();
    if( not is_syn_ ) {
      is_syn_ = true;
      transmit( msg ); //这个一定是握手信号。
      Timer_.start(); // 握手信号一发出去就开始计时。
      wait_ack_que_.push( {msg, 0UL} );//因为这个一定是ack握手。
      abs_seqno_ += msg.sequence_length();
      total_outstanding_ += msg.sequence_length(); //syn 也是得有ack的。
      if( is_fin_ ) end_ = abs_seqno_; //这里是最终结束点
      if( msg.FIN ){//跟下面一样了，握手和发实际消息，两套逻辑。
        is_fin_ = false; //这里有点问题，得确认fin收到了，才能置为false。但是队列是拷贝了一份变量的，好像无所谓
        break;
      }
    }
    //必须要握手成功了，才可以进行发送消息
    if( next_accpet_senum_ > 0UL ){
      if( not input_.reader().bytes_buffered() && !is_fin_ ) break; // 如果没有内容并且没有fin要发的话，即便有窗口也要break
      uint64_t len = std::min(static_cast<uint64_t>(TCPConfig::MAX_PAYLOAD_SIZE), cal_window_size_ - total_outstanding_ );
      len = std::min( len, input_.reader().bytes_buffered() );//读buffer中还有多少字节
      for( uint64_t byte = 0; byte < len ; ++byte ) {
        msg.payload += std::string( input_.reader().peek() );
        input_.reader().pop( 1UL ); //把内容都读出来..
      }
      abs_seqno_ += msg.sequence_length(); //+ ( is_fin_ ? 1UL : 0UL );fin的值已经算进去，下面也一样
      total_outstanding_ += msg.sequence_length(); //+ ( is_fin_ ? 1UL : 0UL);
      if( is_fin_ ) end_ = abs_seqno_; //跟上面同理。
      transmit( msg );
      wait_ack_que_.push( { msg, abs_seqno_ - msg.sequence_length() } );
      if( msg.FIN ){
        is_fin_ = false; //这里有点问题，得确认fin收到了，才能置为false。但是队列是拷贝了一份变量的，好像无所谓
        break;
      } //如果已经发完断开信号，就能break了。
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
    if( ack < next_accpet_senum_ || (!ack && is_syn_) || ack > abs_seqno_ ) return; //ack一定比之前的akc要更新，不能说拿旧的ack来更新窗口。或者就是一个错误的消息
    //比如是0，因为期望的ack至少是1，不然不更新窗口。不能说ack比我发出去的还要大。如果已经连接了，就不能只接受为0的ack
    if( ack > next_accpet_senum_) { //保证要是新数据才能reset   
        Timer_.clear();
        retransmissions = 0;
    }
    this->window_size_ = msg.window_size;
    this->next_accpet_senum_ = ack;
    // 判断一下需要出队的是哪一些已经发送的消息..,干脆加的时候，记一下绝对序列号。
    while( !wait_ack_que_.empty() && wait_ack_que_.front().abs_head_ + wait_ack_que_.front().msg_.sequence_length() <= ack){
      //这后面的判断条件跟ack是重叠的, <=都可以弹出队列的。
      wait_ack_que_.pop();
    }
    //total_outstanding_ -= ( ack - 1UL );
    // 判断一下是收到ack就算是可以重置计数器，还是说必须收到准确的ack才能重置计时器?


    total_outstanding_ = abs_seqno_ - ack; //取差值就是还未收到ack的序列。
    checkpoint_ = msg.ackno.value().unwrap( isn_, checkpoint_ ); 
    //注意到接受到消息，窗口可能变大了，那么就可以继续发送消息了
  } 

}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  // (void)ms_since_last_tick;
  // (void)transmit;
  // (void)initial_RTO_ms_;
  if( Timer_.is_active() ) {
    Timer_.tick( ms_since_last_tick );
    if( Timer_.is_expired() && ( !wait_ack_que_.empty() ) ) {
      transmit( wait_ack_que_.front().msg_ ); 
      //这里其实就涉及到，为什么序列号会重复的问题，所以他窗口会有个要求，防止需要重复的问题。
    
      retransmissions += 1UL;
        //wait_ack_que_.pop();
        //发出去不能pop,能不能pop取决取ack
      Timer_.exponent_backoff(); //已经重传成功了，那么拥塞控制。
      Timer_.reset(); //启动下一个计时了.
    }
  } 
}


//记住，它这个close，实际上push了一个fin信号
//好像