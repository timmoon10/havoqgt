#ifndef LIST_RANDOM_EDGE_CONTAINER_HPP
#define LIST_RANDOM_EDGE_CONTAINER_HPP

#include <random>
/**********************************************************************
    Random Number Generator
***********************************************************************/
struct random_number_generator {
public:
  std::mt19937 en;
      
  static random_number_generator& get_rng() {
    if(val == 0) {
      _rng = random_number_generator();
      val = 1;
    }
    return _rng;
  }

  template<typename distribution>
  typename distribution::result_type operator()(distribution& dist){
    return dist(en);
  }

private:
  static random_number_generator _rng;
  static int val;
  random_number_generator(){ 
    std::random_device r;
    en.seed(r());
  }

};
typedef random_number_generator rng;
rng rng::_rng;
int rng::val = 0;

template<typename Graph, typename EdgeMetaData>
class list_random_edge_container{
public:
  typedef typename Graph::vertex_locator vertex_locator;
  typedef typename Graph::edge_iterator eitr_type;
  typedef typename EdgeMetaData::value_type metadata_type;
 
  list_random_edge_container(Graph* _g, EdgeMetaData *_edge_metadata) : g(_g), edge_metadata(_edge_metadata) { }

  template<typename interval>
  std::pair<bool, interval> find_intersection( const interval& cur_time, const metadata_type& metadata) const {
    interval intersect;
    intersect.first = std::max( cur_time.first, metadata.start_time());
    intersect.second = cur_time.second;
    if( metadata.end_time() != 0) {
      intersect.second = std::min( cur_time.second, metadata.end_time());
    }
    return std::make_pair( intersect.first <= intersect.second, intersect);
  }

  template<typename operation, typename interval>
  std::pair<bool, interval> get_random_weighted_edge(operation& op, interval cur_time, vertex_locator cur_vertex) const {
    std::vector<eitr_type> adjacents;
    for(eitr_type itr = g->edges_begin(cur_vertex); itr != g->edges_end(cur_vertex); ++itr ) {
      metadata_type& metadata = (*edge_metadata)[itr];
      std::pair<bool, interval> intersect_data = find_intersection( cur_time, metadata);
      if(intersect_data.first)
	adjacents.push_back(itr);
    }
    if( adjacents.size() == 0) return std::make_pair(false, cur_time);

    std::uniform_int_distribution<std::size_t> uniform_dist( 0, adjacents.size() - 1);
    std::size_t index;
    {
      index = (rng::get_rng())(uniform_dist);
    }
    auto& itr = adjacents[index];
    op( (*edge_metadata)[itr], itr.target());    
    return find_intersection(cur_time, (*edge_metadata)[itr]);
  }

private:
  Graph *g;
  EdgeMetaData *edge_metadata;
};
#endif
