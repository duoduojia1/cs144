#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  std::optional<uint32_t> tmp;
  if( next_hop.has_value() ) { tmp = next_hop.value().ipv4_numeric(); }
  else tmp = nullopt;
  router_table_.push_back({ route_prefix, prefix_length, tmp, interface_num });
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for( auto &p: _interfaces ) {
    auto& q = p->datagrams_received();
    while( !q.empty() ) {
      const uint32_t& dst =  q.front().header.dst;
      uint8_t& ttl = q.front().header.ttl;
      if( !ttl ){
        q.pop();
         continue;
      }
      //match
      uint8_t record_max_len = 0;
      uint32_t record_index = 0;
      bool match_flag = false;
      for( uint32_t idx = 0; idx <  router_table_.size(); idx++ ) {
          uint64_t cons = ( 1 << router_table_[idx].prefix_length ) - 1UL;
          cons <<= ( 32 - router_table_[idx].prefix_length );
          cons = static_cast<uint32_t>(cons); //111000;
          uint32_t pre_match = dst & cons;
          if( pre_match == router_table_[idx].route_prefix ) {
            match_flag = true;
            if( router_table_[idx].prefix_length >= record_max_len ) { //特判一下默认路由
              record_max_len = std::max( record_max_len, router_table_[idx].prefix_length );
              record_index = idx;
            }
          } //如果匹配成功
      }
      if( !match_flag ){
        q.pop();
        continue;
      }
      ttl -= 1;
      if( !ttl ){
        q.pop();
        continue;
      } 
      if( router_table_[ record_index ].next_hop.has_value() ){
          _interfaces[ router_table_[record_index].interface_num ]->send_datagram( q.front(), Address::from_ipv4_numeric( router_table_[ record_index ].next_hop.value() ));
      }
      else  _interfaces[ router_table_[record_index].interface_num ]->send_datagram( q.front(), Address::from_ipv4_numeric( dst ));
      q.pop();
    }
  }
}
