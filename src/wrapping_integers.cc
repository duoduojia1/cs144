#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.

  Wrap32 new_val = Wrap32(static_cast<uint32_t>(n)) + zero_point.raw_value_;
  // (void)n;
  // (void)zero_point;
  return new_val;
}
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  //找到这个checkpoints有多大
  uint32_t i = 63;
  uint64_t offset = 0; 
  while(i >= 32){
    if((checkpoint >> i) & 1){
      offset = i;
      break;
    }
    i--; 
  }
  if(!offset && (checkpoint > (this->raw_value_ - zero_point.raw_value_))) offset = 32; 
  //猜的，如果要比这个大，肯定是1 << 32 之后的了..
  return (static_cast<uint64_t>(( ~zero_point + this->raw_value_ +  1UL ).raw_value_) + (offset >= 32UL ?  (1UL << (offset)) : 0UL));
  //猜的
  // (void)zero_point;
  // (void)checkpoint;
  // return {};
}
