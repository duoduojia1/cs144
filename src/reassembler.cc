#include "reassembler.hh"

using namespace std;


//只返回重复的部分..
long Reassembler::jug_block(seg_node& a, const seg_node& b){
  seg_node x, y;
    if (a.begin > b.begin) {
        x = b;
        y = a;
    } else {
        x = a;
        y = b;
    }
    if (x.begin + x.siz < y.begin) {
        return -1;  // no intersection, couldn't merge
    } else if (x.begin + x.siz >= y.begin + y.siz) {
        a = x;
        return y.siz;
    } else {
        a.begin = x.begin;
        a.str = x.str + y.str.substr(x.begin + x.siz - y.begin);
        a.siz = a.str.size();
        return x.begin + x.siz - y.begin;
    }
}



void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  auto ava = output_.writer().available_capacity();
  if(!ava) return;
  auto tail = byte_record + ava; // 从tail(包含这一位)之后都是没用的..
  //应该是可用空间
  if(is_last_substring){
    is_end = true;
    end_byte = first_index + data.size();
  }
  if(first_index >= tail) return;
  if(first_index + data.size() > tail){
    data = data.substr(0, tail - first_index); //这部分才是有用的
  }
  // assign比substr少拷贝一次? 
  seg_node elm;
  if(first_index + data.size() <= byte_record){
    goto JUG_END;
  }
  else if(first_index < byte_record){
    auto offset = byte_record - first_index;
    elm.begin = byte_record;
    elm.str.assign(data.begin() + offset, data.end());
    elm.siz = elm.str.size();
  }
  else{
    elm.begin = first_index;
    elm.str = data;
    elm.siz = data.size();
  }
  pending_size += elm.siz; //特判后, 这是一定能加进来的..
  //这个很有用，比如不想执行后面了，可以直接break,保证至少执行一次。
  do{
    //开始向后合并它
    auto cur = st.lower_bound(elm);
    long merge_size = 0;
    //set容器中的元素依赖它的不可变性，所以上面得申明成const
    while(cur != st.end() && (merge_size = jug_block(elm, *cur)) >= 0){ 
      pending_size -= merge_size; //这是重叠的部分，需要减掉它。
      st.erase(cur);
      cur = st.lower_bound(elm);
    }
    //这个cur是动态变换的，所以最后一次lb,可能就变成st.begin了
    if(cur == st.begin()){
      break;
    }

    cur--;
    while(((merge_size = jug_block(elm, *cur)) >= 0)){
      pending_size -= merge_size;
      st.erase(cur);
      cur = st.lower_bound(elm);
      if(cur == st.begin()) break;
      --cur;
    }

  }while(0);
  st.insert(elm);
  //因为做了合并之后，就可能可以往byte_stream中push了


  if(!st.empty() && byte_record == st.begin()->begin){
    output_.writer().push(st.begin()->str);
    pending_size -= st.begin()->siz;
    byte_record = st.begin()->begin + st.begin()->siz;
    st.erase(st.begin());
    goto JUG_END;
  }

JUG_END:
   if(is_end && st.empty() && byte_record == end_byte) output_.writer().close();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return pending_size;
}
