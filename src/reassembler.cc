#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  // 插入结束后，要write一下..因为有可能插入完了，就已经有合法序列能被write了..
  // 还有要考虑str的内容，要考虑如果字符串已经被写入了
  // 考虑哪些已经交给上层了，用byte_record来记录,这个记录这个字节（不包含这个字节）
  vector<std::set<seg_node>::iterator> vec;//这一次哪些要删除
  uint64_t delete_total = 0;//总共删了多少
  auto ava_cap = output_.writer().available_capacity() - pending_size;
  if(is_last_substring) this->is_end = true;
  if(!ava_cap) return; //
  if(first_index + data.size() - 1 < byte_record ) return; //数据太老，直接return
  if(first_index < byte_record){
    data = data.substr(byte_record);// 截断一下..实际要插入的数据是有效位之后的..
  }
  if(ava_cap < data.size()) data = data.substr(0, ava_cap);


  if(first_index < byte_record && data.size()) first_index = byte_record;
  uint64_t stream_size = data.size();
  uint64_t stream_tail = first_index + stream_size - 1;


  auto cur_seg_node = st.upper_bound({stream_tail, 0, ""});

  uint64_t insert_tail;
  std::string record = "";
  std::string dif = "";
  

  bool flag_ = true;
  if(!stream_size && is_end){
    flag_ = false;
    output_.writer().close();
  }


  if(cur_seg_node == st.begin() && flag_ && stream_size <= output_.writer().available_capacity()){
    st.insert({first_index, stream_size, data});
    pending_size += stream_size;
    flag_ = false;
  }
  else if(flag_){
      --cur_seg_node;
      insert_tail = std::max(stream_tail, cur_seg_node->begin + cur_seg_node->siz - 1);
      if(cur_seg_node->begin + cur_seg_node->siz - 1 > stream_tail){
        dif = cur_seg_node->str.substr(stream_tail - cur_seg_node->begin + 1); //从零开始..
      }
      else{
        insert_tail = stream_tail;
      }
  }
  //这里就两个情况，取最长的就可以，可能会重复白删一次 
  while(flag_) {
    auto cur_stream_head = cur_seg_node->begin;
    auto cur_stream_tail = cur_seg_node->begin + cur_seg_node->siz - 1;
    if(first_index >= cur_stream_head && first_index <= cur_stream_tail) {
      delete_total += cur_seg_node->siz;
      vec.push_back(cur_seg_node);

      if(insert_tail - cur_stream_head + 1 - delete_total <= output_.writer().available_capacity()){
        for(auto &p:vec) st.erase(p);
        st.insert({cur_stream_head, insert_tail - cur_stream_head + 1, cur_seg_node->str.substr(0, first_index - cur_stream_head) + data + dif});
        pending_size += insert_tail - cur_stream_head + 1 - delete_total;
      }
      vec.clear();
      break;
    } // 这里是半插入
    else if(first_index > cur_stream_tail && insert_tail - cur_stream_head + 1 <= output_.writer().available_capacity()) { //这里是完全插入
      st.insert({cur_stream_head, insert_tail - cur_stream_head + 1, data});
      pending_size += insert_tail - cur_stream_head + 1;
    }
    else if(first_index < cur_stream_head) { //这里可以吞掉它
        if(cur_seg_node != st.begin()){
          --cur_seg_node;
          delete_total += std::next(cur_seg_node)->siz;
          vec.push_back(std::next(cur_seg_node));
        }
        else{
          delete_total += cur_seg_node->siz;
          vec.push_back(cur_seg_node);
          if(insert_tail - first_index + 1 - delete_total <= output_.writer().available_capacity()){
            for(auto &p:vec) st.erase(p);
            st.insert({first_index, insert_tail - first_index + 1, data + dif});
            pending_size += insert_tail - first_index + 1 - delete_total;
          }
          vec.clear();
          break;
        }
    }
  }
  if(byte_record == st.begin()->begin){
    output_.writer().push(st.begin()->str);
    byte_record = st.begin()->begin + st.begin()->siz;
    pending_size -= st.begin()->siz;
    st.erase(st.begin());
   //std::cout << is_end <<std::endl;
    if(st.size() == 1 && is_end == true){
        output_.writer().close();
    }
  }

  (void)first_index;
  (void)data;
  (void)is_last_substring;
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return pending_size;
}
