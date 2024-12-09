#pragma once

#include "reassembler.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
class TCPReceiver
{
public:
  // Construct with given Reassembler
  explicit TCPReceiver( Reassembler&& reassembler ) : reassembler_( std::move( reassembler ) ), offset_rand_(0),
  has_syn(false) {}

  /*
   * The TCPReceiver receives TCPSenderMessages, inserting their payload into the Reassembler
   * at the correct stream index.
   */
  void receive( TCPSenderMessage message );

  // The TCPReceiver sends TCPReceiverMessages to the peer's TCPSender.
  TCPReceiverMessage send() const;

  // Access the output (only Reader is accessible non-const)
  const Reassembler& reassembler() const { return reassembler_; }
  Reader& reader() { return reassembler_.reader(); }
  const Reader& reader() const { return reassembler_.reader(); }
  const Writer& writer() const { return reassembler_.writer(); }
  const Wrap32& get_offset_rand() const { return offset_rand_; }
  const bool& get_syn() const { return has_syn; }
  // const bool& get_fin() const { return has_fin; }
  void set_offset_rand( Wrap32 offset_rand ) { this->offset_rand_ = offset_rand; }
private:
  Reassembler reassembler_;
  Wrap32 offset_rand_ ;
  bool has_syn;
};
