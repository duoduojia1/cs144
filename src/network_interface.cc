#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

ARPMessage make_arp( uint16_t opcode, const EthernetAddress& sender_ethernet_address, 
const uint32_t& sender_ip_address, EthernetAddress target_ethernet_address, const uint32_t& target_ip_address ) {
  ARPMessage arp_message;
  arp_message.opcode = opcode;
  arp_message.sender_ethernet_address = sender_ethernet_address;
  arp_message.sender_ip_address = sender_ip_address;
  arp_message.target_ethernet_address = target_ethernet_address;
  arp_message.target_ip_address = target_ip_address;  
  return arp_message;
}


//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  if( arp_table_.find( next_hop.ipv4_numeric() ) != arp_table_.end()) {
    EthernetFrame fram{};
    fram.header.dst = arp_table_[ next_hop.ipv4_numeric() ]; //已经获取到目标mac地址了
    fram.header.src = ethernet_address_;  //本机mac地址
    fram.header.type = EthernetHeader::TYPE_IPv4;
    fram.payload = serialize( dgram );
    transmit( fram );
  } // 如果能在arp表中找到
  else{ //如果没在arp表中找不到
    if( wait_set.find( next_hop.ipv4_numeric()) != wait_set.end() ){
      if(timer_.cur_time()- time_stamp_[ next_hop.ipv4_numeric() ] <= 5000) return;
    }//证明已经发过了，判断是不是 < 3000
    time_stamp_[ next_hop.ipv4_numeric() ] = timer_.cur_time(); 
    broad_wait_[ next_hop.ipv4_numeric() ].push_back( dgram );
    EthernetFrame fram{};
    fram.header.dst = ETHERNET_BROADCAST;
    fram.header.src = this->ethernet_address_;
    fram.header.type = EthernetHeader::TYPE_ARP;

    fram.payload = serialize( make_arp( ARPMessage::OPCODE_REQUEST, this->ethernet_address_, this->ip_address_.ipv4_numeric(), 
    {}, next_hop.ipv4_numeric() ) );
    transmit( fram );
    wait_set.insert( next_hop.ipv4_numeric() );
    time_stamp_[ next_hop.ipv4_numeric() ] = timer_.cur_time();
  }

  // (void)ethernet_address_;
  // (void)dgram;
  // (void)next_hop;
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  if (frame.header.dst != this->ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST) {
    return;//既不是发向自己，也不是广播帧。
  }
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
      InternetDatagram datagram{};
      if ( parse( datagram, frame.payload ) && (this->ethernet_address_ == frame.header.dst) ) {
        datagrams_received_.push( datagram );
      }

  }
  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arpmessage;
    if( not parse( arpmessage, frame.payload )) return;
    if( arpmessage.opcode == ARPMessage::OPCODE_REQUEST && arpmessage.target_ip_address == this->ip_address_.ipv4_numeric() ){
      arp_table_[ arpmessage.sender_ip_address ] = arpmessage.sender_ethernet_address;
      arp_table_time_[ arpmessage.sender_ip_address ] = timer_.cur_time();

      //在同一个局域网内。
      EthernetFrame fram{};
      fram.header.dst = arpmessage.sender_ethernet_address;
      fram.header.src = this->ethernet_address_;
      fram.header.type = EthernetHeader::TYPE_ARP;

      fram.payload = serialize( make_arp( ARPMessage::OPCODE_REPLY, this->ethernet_address_, this->ip_address_.ipv4_numeric(), 
      arpmessage.sender_ethernet_address, arpmessage.sender_ip_address ) );
      transmit( fram );

    }
    if( arpmessage.opcode == ARPMessage::OPCODE_REPLY && arpmessage.target_ethernet_address == this->ethernet_address_ ) {

      //不管是请求帧还是回复帧都要让arp表学习
      arp_table_[ arpmessage.sender_ip_address ] = arpmessage.sender_ethernet_address;
      arp_table_time_[ arpmessage.sender_ip_address ] = timer_.cur_time();

      for( auto &p:broad_wait_[ arpmessage.sender_ip_address ]) {
        EthernetFrame fram{};
        fram.header.dst = arpmessage.sender_ethernet_address;
        fram.header.src = ethernet_address_;
        fram.header.type = EthernetHeader::TYPE_IPv4;
        fram.payload = serialize( p );
        transmit( fram );
      }
      broad_wait_[ arpmessage.sender_ip_address ].clear();
    } //如果是请求帧
  }
  InternetDatagram datagram;
  if( frame.header.type == EthernetHeader::TYPE_IPv4 and ( not parse( datagram, frame.payload ) ) ) {
    if( frame.header.dst == this->ethernet_address_ ) datagrams_received_.push( datagram );
  } 
  //如果发的是数据帧
  //(void)frame;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  timer_.tick( ms_since_last_tick );
  for (auto it = arp_table_time_.begin(); it != arp_table_time_.end(); ) {
    if (timer_.cur_time() - it->second > 30000) {
        arp_table_.erase( it->first );
        it = arp_table_time_.erase(it);  // 使用 erase 返回新的迭代器
    } else {
        ++it;  // 继续遍历下一个元素
    }
}

  // (void)ms_since_last_tick;
}
