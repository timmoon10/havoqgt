/*
 * Copyright (c) 2013, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Roger Pearce, Scott Sallinen <{rpearce, sallinen1}@llnl.gov>.
 * LLNL-CODE-644630.
 * All rights reserved.
 *
 * This file is part of HavoqGT, Version 0.1.
 * For details, see https://computation.llnl.gov/casc/dcca-pub/dcca/Downloads.html
 *
 * Please also read this link – Our Notice and GNU Lesser General Public License.
 *   http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the terms and conditions of the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * OUR NOTICE AND TERMS AND CONDITIONS OF THE GNU GENERAL PUBLIC LICENSE
 *
 * Our Preamble Notice
 *
 * A. This notice is required to be provided under our contract with the
 * U.S. Department of Energy (DOE). This work was produced at the Lawrence
 * Livermore National Laboratory under Contract No. DE-AC52-07NA27344 with the DOE.
 *
 * B. Neither the United States Government nor Lawrence Livermore National
 * Security, LLC nor any of their employees, makes any warranty, express or
 * implied, or assumes any liability or responsibility for the accuracy,
 * completeness, or usefulness of any information, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately-owned rights.
 *
 * C. Also, reference herein to any specific commercial products, process, or
 * services by trade name, trademark, manufacturer or otherwise does not
 * necessarily constitute or imply its endorsement, recommendation, or favoring by
 * the United States Government or Lawrence Livermore National Security, LLC. The
 * views and opinions of authors expressed herein do not necessarily state or
 * reflect those of the United States Government or Lawrence Livermore National
 * Security, LLC, and shall not be used for advertising or product endorsement
 * purposes.
 *
 */

#ifndef HAVOQGT_MPI_GRAPH_COLOUR_LDF_HPP_INCLUDED
#define HAVOQGT_MPI_GRAPH_COLOUR_LDF_HPP_INCLUDED


#include <havoqgt/visitor_queue.hpp>
#include <boost/container/deque.hpp>
#include <havoqgt/detail/visitor_priority_queue.hpp>

namespace havoqgt { namespace mpi {



// LDF Collector: This traversal will collect the number of vertices with a
// higher degree than the vertex.
template<typename Graph, typename CounterData>
class gc_ldf_collector_visitor {
 public:
  typedef typename Graph::vertex_locator vertex_locator;
  // Default constructor.
  gc_ldf_collector_visitor() :
      vertex(), caller(), incoming_degree(0) {  }

  // Baseline constructor.
  explicit gc_ldf_collector_visitor(vertex_locator _vertex) :
      vertex(_vertex), caller(_vertex), incoming_degree(0) {  }

  // Who I am, who notified me, and what degree do they have.
  gc_ldf_collector_visitor(vertex_locator _vertex, vertex_locator _caller,
                       uint64_t _incoming_degree) :
      vertex(_vertex), caller(_caller), incoming_degree(_incoming_degree) {  }


  bool pre_visit() const {
    // No one would ever send a message to us if their degree was 0,
    // So this is the initialization step.
    if (incoming_degree == 0) {  return true;  }

    // Don't count an edge to one's self!
    if (caller == vertex) {  return false;  }

    // Accumulate degree, if their degree is larger than ours.
    // (i.e. They will go first.)
    uint64_t my_degree = (*graph_ref()).degree(vertex);
    if ( my_degree <  incoming_degree ||
        (my_degree == incoming_degree && (vertex < caller))) {
      (*counter_data())[vertex]++;
    }

    return false;
  }


  template<typename VisitorQueueHandle>
  bool visit(const Graph& graph, VisitorQueueHandle vis_queue) const {
    // Simply send our degree to all our neighbours.
    typedef typename Graph::edge_iterator eitr_t;
    for (eitr_t edge  = graph.edges_begin(vertex);
                edge != graph.edges_end(vertex); edge++) {
      vertex_locator neighbour = edge.target();

      // Send the neighbour a visitor with our degree.
      gc_ldf_collector_visitor new_v(neighbour, vertex, graph.degree(vertex));
      vis_queue->queue_visitor(new_v);
    }
    return false;
  }

  // The queue is actually not necessary for this traversal.
  friend inline bool operator > (const gc_ldf_collector_visitor& v1,
                                 const gc_ldf_collector_visitor& v2) {
    return false;
  }


  // Static data fields.

  static void set_graph_ref(Graph* _graph_ref) {
    graph_ref() = _graph_ref;
  }
  static Graph*& graph_ref() {
    static Graph* _graph_ref;
    return _graph_ref;
  }

  static void set_counter_data(CounterData* _data) {
    counter_data() = _data;
  }
  static CounterData*& counter_data() {
    static CounterData* data;
    return data;
  }


  // Instance variables.

  vertex_locator vertex;
  vertex_locator caller;
  uint64_t       incoming_degree : 32;
} __attribute__((packed));



// SDF Collector: This traversal will collect the number of vertices with a
// smaller degree than the vertex.
template<typename Graph, typename CounterData>
class gc_sdf_collector_visitor {
 public:
  typedef typename Graph::vertex_locator vertex_locator;
  // Default constructor.
  gc_sdf_collector_visitor() :
      vertex(), caller(), incoming_degree(0) {  }

  // Baseline constructor.
  explicit gc_sdf_collector_visitor(vertex_locator _vertex) :
      vertex(_vertex), caller(_vertex), incoming_degree(0) {  }

  // Who I am, who notified me, and what degree do they have.
  gc_sdf_collector_visitor(vertex_locator _vertex, vertex_locator _caller,
                       uint64_t _incoming_degree) :
      vertex(_vertex), caller(_caller), incoming_degree(_incoming_degree) {  }


  bool pre_visit() const {
    // No one would ever send a message to us if their degree was 0,
    // So this is the initialization step.
    if (incoming_degree == 0) {  return true;  }

    // Don't count an edge to one's self!
    if (caller == vertex) {  return false;  }

    // Accumulate degree, if their degree is smaller than ours.
    // (i.e. They will go first.)
    uint64_t my_degree = (*graph_ref()).degree(vertex);
    if ( my_degree >  incoming_degree ||
        (my_degree == incoming_degree && (caller < vertex))) {
      (*counter_data())[vertex]++;
    }

    return false;
  }


  template<typename VisitorQueueHandle>
  bool visit(const Graph& graph, VisitorQueueHandle vis_queue) const {
    // Simply send our degree to all our neighbours.
    typedef typename Graph::edge_iterator eitr_t;
    for (eitr_t edge  = graph.edges_begin(vertex);
                edge != graph.edges_end(vertex); edge++) {
      vertex_locator neighbour = edge.target();

      // Send the neighbour a visitor with our degree.
      gc_sdf_collector_visitor new_v(neighbour, vertex, graph.degree(vertex));
      vis_queue->queue_visitor(new_v);
    }
    return false;
  }

  // The queue is actually not necessary for this traversal.
  friend inline bool operator > (const gc_sdf_collector_visitor& v1,
                                 const gc_sdf_collector_visitor& v2) {
    return false;
  }


  // Static data fields.

  static void set_graph_ref(Graph* _graph_ref) {
    graph_ref() = _graph_ref;
  }
  static Graph*& graph_ref() {
    static Graph* _graph_ref;
    return _graph_ref;
  }

  static void set_counter_data(CounterData* _data) {
    counter_data() = _data;
  }
  static CounterData*& counter_data() {
    static CounterData* data;
    return data;
  }


  // Instance variables.

  vertex_locator vertex;
  vertex_locator caller;
  uint64_t       incoming_degree : 32;
} __attribute__((packed));



// ID Collector: This traversal will collect the number of vertices with a
// larger hashed ID than the vertex.
template<typename Graph, typename CounterData>
class gc_id_collector_visitor {
 public:
  typedef typename Graph::vertex_locator vertex_locator;
  // Default constructor.
  gc_id_collector_visitor() :
      vertex(), caller() {  }

  // Baseline constructor.
  explicit gc_id_collector_visitor(vertex_locator _vertex) :
      vertex(_vertex), caller(_vertex) {  }

  // Who I am, who notified me.
  gc_id_collector_visitor(vertex_locator _vertex, vertex_locator _caller) :
      vertex(_vertex), caller(_caller) {  }


  bool pre_visit() const {
    assert((*counter_data())[vertex] == 0);

    // Iterate over the vertex's edges and count those with higher priority.
    typedef typename Graph::edge_iterator eitr_t;
    for (eitr_t edge  = (*graph_ref()).edges_begin(vertex);
                edge != (*graph_ref()).edges_end(vertex); edge++) {
      vertex_locator nbr = edge.target();

      // Check if neighbour has a greater priority.
      if (vertex_locator::lesser_hash_priority(vertex, nbr)) {
        (*counter_data())[vertex]++;
      }
    }
    return false;
  }


  // Not needed!
  template<typename VisitorQueueHandle>
  bool visit(const Graph& graph, VisitorQueueHandle vis_queue) const {
    assert(false);
    return false;
  }

  // The queue is actually not necessary for this traversal.
  friend inline bool operator > (const gc_id_collector_visitor& v1,
                                 const gc_id_collector_visitor& v2) {
    return false;
  }


  // Static data fields.

  static void set_graph_ref(Graph* _graph_ref) {
    graph_ref() = _graph_ref;
  }
  static Graph*& graph_ref() {
    static Graph* _graph_ref;
    return _graph_ref;
  }

  static void set_counter_data(CounterData* _data) {
    counter_data() = _data;
  }
  static CounterData*& counter_data() {
    static CounterData* data;
    return data;
  }


  // Instance variables.

  vertex_locator vertex;
  vertex_locator caller;
} __attribute__((packed));



template<typename Graph, typename VertexColourData, typename CounterData,
         typename NbrColourData /* typename NbrIsColouredData */>
class gc_visitor {
 public:
  typedef typename Graph::vertex_locator vertex_locator;
  // Default constructor.
  gc_visitor() :
          vertex(), caller(), caller_colour(0) {  }

  // Baseline constructor.
  explicit gc_visitor(vertex_locator _vertex) :
        vertex(_vertex), caller(_vertex), caller_colour(0) {  }

  // Who I am, who notified me, what colour they notified me with.
  gc_visitor(vertex_locator _vertex, vertex_locator _caller,
                 uint32_t _colour) :
      vertex(_vertex), caller(_caller), caller_colour(_colour) {  }


  bool pre_visit() const {
    // We need to resize our bitmap to our degree if we haven't done so before.
    // We also add one, due to the graph colouring property of colour being
    // between [0, degree+1]
    auto* bitmap = &((*nbr_colour_data())[vertex]);
    if (bitmap->size() != (*graph_ref()).degree(vertex) + 1) {
      bitmap->resize((*graph_ref()).degree(vertex) + 1);
    }

    // No one will ever tell us their colour is zero, so must be initialization.
    if (caller_colour == 0) {
      if ((*counter_data())[vertex] == 0) {
        assert(((*vertex_colour_data())[vertex] == 0) && "visit point one");
        return true;
      } else {
        return false;
      }
    }

    // Don't count an edge to one's self!
    if (caller == vertex) {  return false;  }

    // If we get a message of someone telling us their colour, and if we know
    // we're already coloured, we don't have to do anything.
    if ((*counter_data())[vertex] == 0) {
      assert(((*vertex_colour_data())[vertex]) != caller_colour);
      return false;
    }

    // Recieved a colour.
    // Add incoming colour to invalid colour list, if needed.
    if (caller_colour <= bitmap->size() && caller_colour != 0) {
      bitmap->set(caller_colour - 1);  // One indexing offset.
    }

    // Decrease counter of number of vertices that need to colour before we do.
    (*counter_data())[vertex]--;

    // If we decrease to zero, we will full visit.
    if ((*counter_data())[vertex] == 0) {
      assert(((*vertex_colour_data())[vertex] == 0) && "visit point two");
      return true;
    }
    // Otherwise, we are not allowed to colour ourself yet.
    return false;
  }


  template<typename VisitorQueueHandle>
  bool visit(const Graph& graph, VisitorQueueHandle vis_queue) const {
    assert((*counter_data())[vertex] == 0);

    // We should not re-colour a vertex!
    // The only case that this should ever be hit is when we recieved message
    // that decreased our counter enough that we are able to go, BEFORE
    // the actual initialization pre-vist struck.
    if ((*vertex_colour_data())[vertex] != 0) {  return false;  }

    // Colour the vertex.
    auto* bitmap = &((*nbr_colour_data())[vertex]);

    // Ignore singletons (degree zero), assign them to colour 1.
    uint32_t colour = 1;
    if (bitmap->size() != 0) {
      bitmap->flip();
      colour = (bitmap->find_first() + 1);  // One indexing offset.
    }
    (*vertex_colour_data())[vertex] = colour;

    assert(colour != 0);

    // Tell our neighbours that we have coloured ourself.
    typedef typename Graph::edge_iterator eitr_t;

    // TODO(Scott): This is not being compiled out. There is a noticable
    //              performance diference when hard-coding the alg type.
    //              Consider moving the type to a template parameter?
    // Extra ability for optimization with hash based gc.
    if ((*alg_type()) == 2) {
      for (eitr_t edge  = graph.edges_begin(vertex);
                  edge != graph.edges_end(vertex); edge++) {
        vertex_locator neighbour = edge.target();

        // When using hash based gc, we can tell if a nbr doesn't need this msg:
        // if nbr has a greater priority, then they are already coloured.
        if (vertex_locator::lesser_hash_priority(vertex, neighbour)) {
          continue;
        }

        // Send the neighbour a visitor with our colour.
        gc_visitor new_visitor(neighbour, vertex, colour);
        vis_queue->queue_visitor(new_visitor);
      }
    // Regular gc.
    } else {
      for (eitr_t edge  = graph.edges_begin(vertex);
                  edge != graph.edges_end(vertex); edge++) {
        vertex_locator neighbour = edge.target();

        // Send the neighbour a visitor with our colour.
        gc_visitor new_visitor(neighbour, vertex, colour);
        vis_queue->queue_visitor(new_visitor);
      }
    }
    return false;
  }


  friend inline bool operator > (const gc_visitor& v1, const gc_visitor& v2) {
    // No movement.
    // return false;
    // By ID.
    // return !(v1.vertex < v2.vertex);
    // By degree.
    return (*graph_ref()).degree(v1.vertex) > (*graph_ref()).degree(v2.vertex);
  }


  // Static data fields.

  static void set_graph_ref(Graph* _graph_ref) {
    graph_ref() = _graph_ref;
  }
  static Graph*& graph_ref() {
    static Graph* _graph_ref;
    return _graph_ref;
  }

  static void set_vertex_colour_data(VertexColourData* _data) {
    vertex_colour_data() = _data;
  }
  static VertexColourData*& vertex_colour_data() {
    static VertexColourData* data;
    return data;
  }

  static void set_counter_data(CounterData* _data) {
    counter_data() = _data;
  }
  static CounterData*& counter_data() {
    static CounterData* data;
    return data;
  }

  static void set_nbr_colour_data(NbrColourData* _data) {
    nbr_colour_data() = _data;
  }
  static NbrColourData*& nbr_colour_data() {
    static NbrColourData* data;
    return data;
  }

  static void set_alg_type(int* _alg_type) {
    alg_type() = _alg_type;
  }
  static int*& alg_type() {
    static int* alg_type;
    return alg_type;
  }


  // Instance variables.

  vertex_locator   vertex;
  vertex_locator   caller;
  uint32_t         caller_colour;
} __attribute__((packed));



// Largest Degree First collection.
template <typename TGraph, typename CounterData>
void gc_init_ldf(TGraph* graph, CounterData* counter_data) {
  MPI_Barrier(MPI_COMM_WORLD);
  double time_start = MPI_Wtime();

  {
    // Get degree info from a primary traversal.
    typedef gc_ldf_collector_visitor<TGraph, CounterData> collector_t;
    collector_t::set_graph_ref(graph);
    collector_t::set_counter_data(counter_data);

    typedef visitor_queue<collector_t, havoqgt::detail::visitor_priority_queue,
                          TGraph> collector_queue_t;
    collector_queue_t collector(graph);

    // Begin traversal with all veritices.
    collector.init_visitor_traversal_local_first();
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double time_end = MPI_Wtime();
  int mpi_rank(0);
  CHK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank));
  if (mpi_rank == 0) {
    std::cout << "LDF Collect time = " << time_end - time_start << std::endl;
  }
}



// Smallest Degree First collection.
template <typename TGraph, typename CounterData>
void gc_init_sdf(TGraph* graph, CounterData* counter_data) {
  MPI_Barrier(MPI_COMM_WORLD);
  double time_start = MPI_Wtime();

  {
    // Get degree info from a primary traversal.
    typedef gc_sdf_collector_visitor<TGraph, CounterData> collector_t;
    collector_t::set_graph_ref(graph);
    collector_t::set_counter_data(counter_data);

    typedef visitor_queue<collector_t, havoqgt::detail::visitor_priority_queue,
                          TGraph> collector_queue_t;
    collector_queue_t collector(graph);

    // Begin traversal with all veritices.
    collector.init_visitor_traversal_local_first();
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double time_end = MPI_Wtime();
  int mpi_rank(0);
  CHK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank));
  if (mpi_rank == 0) {
    std::cout << "SDF Collect time = " << time_end - time_start << std::endl;
  }
}



// Larger vertex ID collection.
template <typename TGraph, typename CounterData>
void gc_init_id(TGraph* graph, CounterData* counter_data) {
  MPI_Barrier(MPI_COMM_WORLD);
  double time_start = MPI_Wtime();

  {
    // Get ID info from a primary glance.
    typedef gc_id_collector_visitor<TGraph, CounterData> collector_t;
    collector_t::set_graph_ref(graph);
    collector_t::set_counter_data(counter_data);

    typedef visitor_queue<collector_t, havoqgt::detail::visitor_priority_queue,
                          TGraph> collector_queue_t;
    collector_queue_t collector(graph);

    // Begin traversal with all veritices.
    collector.init_visitor_traversal_local_first();
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double time_end = MPI_Wtime();
  int mpi_rank(0);
  CHK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank));
  if (mpi_rank == 0) {
    std::cout << "ID Collect time = " << time_end - time_start << std::endl;
  }
}



// Launch point for graph colouring.
template <typename TGraph,      typename VertexColourData,
          typename CounterData, typename NbrColourData>
void graph_colour(TGraph*           graph,
                  VertexColourData* vertex_colour_data,
                  CounterData*      counter_data,
                  NbrColourData*    nbr_colour_data,
                  int               alg_type) {
  // Initialize counter based on type of comparison (id, degree).
  switch (alg_type) {
    case 0:
      // Collect larger degree count for each vertex.
      gc_init_ldf(graph, counter_data);
      break;
    case 1:
      // Collect smaller degree count for each vertex.
      gc_init_sdf(graph, counter_data);
      break;
    case 2:
      // Collect larger hashed id count for each vertex.
      gc_init_id(graph, counter_data);
      break;
    default:
      std::cerr << "Bad GC type!" << std::endl;
      exit(-1);
  }

  double time_start = MPI_Wtime();

  // Finished collecting counter info, begin colouring.
  {
    typedef gc_visitor<TGraph, VertexColourData, CounterData,
                       NbrColourData> colourer_t;
    colourer_t::set_graph_ref(graph);
    colourer_t::set_vertex_colour_data(vertex_colour_data);
    colourer_t::set_counter_data(counter_data);
    colourer_t::set_nbr_colour_data(nbr_colour_data);
    colourer_t::set_alg_type(&alg_type);

    typedef visitor_queue<colourer_t, havoqgt::detail::visitor_priority_queue,
                          TGraph> colourer_queue_t;
    colourer_queue_t colourer(graph);

    // Begin traversal with all veritices.
    colourer.init_visitor_traversal();
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double time_end = MPI_Wtime();
  int mpi_rank(0);
  CHK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank));
  if (mpi_rank == 0) {
    std::cout << "Colour time = " << time_end - time_start << std::endl;
  }
}



}  // namespace mpi
}  // namespace havoqgt



#endif  // HAVOQGT_MPI_GRAPH_COLOUR_LDF_HPP_INCLUDED
