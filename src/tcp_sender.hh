#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>



struct wait_message {
  TCPSenderMessage  msg_;// 记录消息。
  uint64_t abs_head_;
};

class RetransmissionTimer
{
  public: 
    explicit RetransmissionTimer( uint64_t initial_RTO_ms ) : RTO_ms_( initial_RTO_ms )
    ,initial_{ initial_RTO_ms } {}
    constexpr auto is_active() -> bool { return is_active_; }
    constexpr auto is_expired() ->bool { return is_active_ and timer_ >= RTO_ms_; }
    constexpr auto reset() -> void { timer_ = 0; }
    constexpr auto clear() -> void { timer_ = 0, RTO_ms_ = initial_; }//收到ack后一定要重置一下计时器 
    constexpr auto exponent_backoff() -> void { RTO_ms_ *= 2; }
    constexpr auto start() ->void { is_active_ = true, reset(); }
    constexpr auto stop() -> void { is_active_ = false , reset(); }
    constexpr auto tick(uint64_t ms_since_last_tick ) -> RetransmissionTimer&
    {
      timer_ += is_active_ ? ms_since_last_tick : 0;
      return *this;
    }
  private:
    bool is_active_ {};
    uint64_t RTO_ms_; //编译器不允许没法使用的值，我干脆绑定原先类的值就好了。
    uint64_t initial_;
    uint64_t timer_ {};
};
class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), Timer_( initial_RTO_ms_ ){}
    //这里为什么用this，就是希望它绑定到的值是成员变量，而不是形参。
  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessorsnce numbers are o
  uint64_t sequence_numbers_in_flight() const;  // How many sequeutstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  bool is_syn_{};
  bool is_fin_{};
  uint64_t end_{};//引入这个是因为，它最后收到fin信号的时候又push了，我又发了一次消息。。。
  uint64_t next_accpet_senum_{ };// 这个是收到的ack表示期望收到的下一个序号。
  uint64_t abs_seqno_{}; //下一个该发送的序号..(之前都已经发了, 但是不一定收到了)
  uint64_t total_outstanding_{};
  uint64_t window_size_{};
  uint64_t checkpoint_{};
  uint64_t retransmissions {};//用来计算重传次数。
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_; 
  std::queue<wait_message>  wait_ack_que_{};
  //好像每次只重传第一个段？
  RetransmissionTimer Timer_;
  //这个就是为了等待ack到来，判断计时用的，每次只需要从头pop即可。
};
