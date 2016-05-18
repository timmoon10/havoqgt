
#ifndef _SIMPLE_RANDOM_WALKER_HPP
#define _SIMPLE_RANDOM_WALKER_HPP

#define SZ 10

#include <unordered_set>
#include <iostream>
#include <tuple>

template <typename VertexType>
class simple_random_walker {
public:
  using vertex_locator = VertexType;

  //This is so insane because we have to use const method
  template<int size>
  class memory{
  public:
    memory() : curr(-1){ }

    memory operator=(const memory& m) {
      curr = m.curr;
      for(int i = 0; i <= m.curr; i++) {
	arr[i] = m.arr[i];
      }
      return *this;
    }

    memory push( vertex_locator v) const {
      memory m;
      int offset = 0;
      if( curr + 1 == size ) {
	offset = 1;
      }
      int i = 0;
      for(; i <= curr - offset; i++){
	m.arr[i] = arr[i + offset];
      }
      m.arr[i] = v;
      m.curr = curr + (1 - offset);
      return m;
    }

    bool empty() const {
      return curr == -1;
    }

    memory pop(vertex_locator& v) const{      
      assert(curr != -1);
      memory m;
      v = arr[curr];
      m.curr--;
      for(int i = 0; i <= m.curr; i++) {
	m.arr[i] = arr[i];
      }
      return m;
    }

    vertex_locator arr[size];
    int curr; //points to the unfilled region
  };

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

  simple_random_walker() : id(0) {}
  
  simple_random_walker( uint64_t _id
			,memory<SZ> _mem = memory<SZ>()
			,uint64_t _steps = 0
			)
    : id(_id), steps(_steps), mem(_mem) {}

  bool is_complete(vertex_locator vertex) const {
    return (local_targets.find(vertex) != local_targets.end() ) || steps >= simple_random_walker::max_steps;
  }

  //Function returning the next state for the random walkers
  template<typename Graph>
  std::tuple<int, vertex_locator, simple_random_walker> next(Graph* g, vertex_locator cur_vertex ) const{
    uint64_t size = g->edges_end(cur_vertex) - g->edges_begin(cur_vertex);
    double fwd = (size > 0) ? forward_jump : 0;
    double bkp = (mem.empty()) ? 0 : backup_jump;
    std::discrete_distribution<uint32_t> discrete_dist {fwd, bkp, 1 - (forward_jump + backup_jump)};
    uint64_t index;
    {
      index = (rng::get_rng())(discrete_dist);
    }
    switch(index) {
    case 0: //Forward jump
      {
	std::uniform_int_distribution<uint64_t> uniform_dist( 0, size - 1);
	index = (rng::get_rng())(uniform_dist);
	typename Graph::edge_iterator itr(g);
	itr = g->edges_begin(cur_vertex);
	itr += index;
	vertex_locator target = itr.target();
	memory<SZ> m = mem.push( cur_vertex );
	simple_random_walker next_rw( id, m, steps + 1);
	return std::make_tuple( 0, target, next_rw );
      }
    case 1: //Backup
      {
	vertex_locator target;
	memory<SZ> m = mem.pop(target);
	simple_random_walker next_rw( id, m, steps + 1);
	return std::make_tuple( 1, target, next_rw );
      }
    case 2: //Random jump
      {
	std::uniform_int_distribution<uint64_t> uniform_dist( 0, g->max_global_vertex_id() );
	index = (rng::get_rng())(uniform_dist);
	vertex_locator target = g->label_to_locator( index );
	memory<SZ> m = mem.push( cur_vertex );
	simple_random_walker next_rw( id, m, steps + 1);
	return std::make_tuple( 2, target, next_rw );
      }
    }
  }

  friend std::ostream& operator<<(std::ostream& o, const simple_random_walker& rw) {
    return o << rw.id;
    //	     << " " << rw.cost << " " << rw.steps;
  }  
  // Just making them public
  uint64_t id;
  //  uint64_t cost;
  uint64_t steps;
  memory<SZ> mem;
  static std::unordered_set<vertex_locator, typename vertex_locator::hasher> local_targets;
  static uint64_t max_steps;
  static double forward_jump;
  static double backup_jump;
};

template<typename VertexType>
std::unordered_set< VertexType, typename VertexType::hasher> simple_random_walker<VertexType>::local_targets;

template<typename VertexType>
uint64_t simple_random_walker<VertexType>::max_steps;

template<typename VertexType>
double simple_random_walker<VertexType>::forward_jump;

template<typename VertexType>
double simple_random_walker<VertexType>::backup_jump;

template<typename VertexType>
typename simple_random_walker<VertexType>::rng simple_random_walker<VertexType>::rng::_rng;

template<typename VertexType>
int simple_random_walker<VertexType>::rng::val = 0;
#endif
