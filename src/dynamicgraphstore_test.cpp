
#include <iostream>
#include <fstream>
#include <random>
#include <unordered_set>

#include <boost/interprocess/managed_mapped_file.hpp>

#include <havoqgt/environment.hpp>
#include <havoqgt/parallel_edge_list_reader.hpp>
#include <havoqgt/rmat_edge_generator.hpp>

using mapped_file_type     = boost::interprocess::managed_mapped_file;
using segment_manager_type = boost::interprocess::managed_mapped_file::segment_manager;

using vertex_id_type        = uint64_t;
using edge_property_type    = uint64_t;
using vertex_property_type  = uint64_t;

enum : size_t {
  middle_high_degree_threshold = 10 // must be more or equal than 1
};

#if 0
#include <havoqgt/graphstore/baseline/baseline.hpp>
using graphstore_type       = graphstore::graphstore_baseline<vertex_id_type,
                                                              vertex_property_type,
                                                              edge_property_type,
                                                              segment_manager_type>;
#elif 0
#include <havoqgt/graphstore/baseline/baseline_map.hpp>
using graphstore_type       = graphstore::graphstore_baseline_map<vertex_id_type,
                                                                  vertex_property_type,
                                                                  edge_property_type,
                                                                  segment_manager_type>;
#else
#include <havoqgt/graphstore/degawarerhh/degawarerhh.hpp>
using graphstore_type       = graphstore::degawarerhh<vertex_id_type,
                                                      vertex_property_type,
                                                      edge_property_type,
                                                      segment_manager_type,
                                                      middle_high_degree_threshold>;
#endif


void fallocate(const char* const fname, size_t size, mapped_file_type& asdf)
{
#ifdef __linux__
    std::cout << "Call fallocate()" << std::endl;
    int fd  = open(fname, O_RDWR);
    assert(fd != -1);
    /// posix_fallocate dosen't work on XFS ?
    /// (dosen't actually expand the file size ?)
    int ret = fallocate(fd, 0, 0, size);
    assert(ret == 0);
    close(fd);
    asdf.flush();
#else
#warning fallocate() is not supported
#endif
}


void usage()  {
  if(havoqgt::havoqgt_env()->world_comm().rank() == 0) {
    std::cerr << "Usage: -i <string> -s <int>\n"
         << " -s <string>   - output graph base filename (default is /dev/shm/segment_file)\n"
//         << " -e <string>   - filename that has a list of edgelist files\n"
         << " -h            - print help and exit\n\n";
  }
}

void parse_cmd_line(int argc, char** argv, std::string& segmentfile_name, std::vector<std::string>& edgelist_files) {
  if(havoqgt::havoqgt_env()->world_comm().rank() == 0) {
    std::cout << "CMD line:";
    for (int i=0; i<argc; ++i) {
      std::cout << " " << argv[i];
    }
    std::cout << std::endl;
  }

  segmentfile_name = "/dev/shm/segment_file";

  bool found_segmentfile_name_ = true;
  bool found_edgelist_filename_ = true;

  char c;
  bool prn_help = false;
  while ((c = getopt(argc, argv, "s:e:h")) != -1) {
     switch (c) {
       case 'h':
         prn_help = true;
         break;
       case 's':
         found_segmentfile_name_ = true;
         segmentfile_name = optarg;
         break;
       case 'e':
       {
         found_edgelist_filename_ = true;
         std::string fname(optarg);
         std::ifstream fin(fname);
         std::string line;
         if (!fin.is_open()) {
           std::cerr << fname << std::endl;
           HAVOQGT_ERROR_MSG("Unable to open a file");
         }
         while (std::getline(fin, line)) {
           edgelist_files.push_back(line);
         }
         break;
       }
       default:
         std::cerr << "Unrecognized option: "<<c<<", ignore."<<std::endl;
         prn_help = true;
         break;
     }
   }
   if (prn_help || !found_segmentfile_name_ || !found_edgelist_filename_) {
     usage();
     exit(-1);
   }
}


namespace std {
  template <> struct hash<std::pair<vertex_id_type, vertex_id_type>>
  {
    size_t operator()(const std::pair<vertex_id_type, vertex_id_type> & x) const
    {
        std::size_t h1 = std::hash<vertex_id_type>()(x.first);
        std::size_t h2 = std::hash<vertex_id_type>()(x.second);
        return h1 ^ (h2 << 1);
    }
  };
}


void test4(graphstore_type& graphstore, size_t vertex_scale, size_t edge_factor, int delete_ratio)
{
  const size_t num_edges = (1ULL << vertex_scale) * edge_factor;
  havoqgt::rmat_edge_generator rmat(uint64_t(5489), vertex_scale, num_edges,
    0.57, 0.19, 0.19, 0.05, true, true);

  std::random_device rnd;
  std::mt19937_64 mt(rnd());
  std::uniform_int_distribution<> rand100(1, 100);

  std::unordered_multiset<std::pair<vertex_id_type, vertex_id_type>> table;

  for (auto edge = rmat.begin(), end = rmat.end();
       edge != end;
       ++edge) {
    const vertex_id_type& src = std::get<0>(*edge);
    const vertex_id_type& trg = std::get<1>(*edge);

    const bool is_delete = (rand100(mt) <= delete_ratio);

    if (is_delete) {
      const size_t num_deleted = table.erase(std::make_pair(src, trg));
      assert(graphstore.erase_edge_dup(src, trg) == num_deleted);

    } else {
      table.insert(std::make_pair(src, trg));
      graphstore.insert_edge_dup(src, trg, 1);
    }
  }


}


void test3(graphstore_type& graphstore, size_t vertex_scale, size_t edge_factor, int delete_ratio)
{
  const size_t num_edges = (1ULL << vertex_scale) * edge_factor;
  havoqgt::rmat_edge_generator rmat(uint64_t(5489), vertex_scale, num_edges,
    0.57, 0.19, 0.19, 0.05, true, true);

  std::random_device rnd;
  std::mt19937_64 mt(rnd());
  std::uniform_int_distribution<> rand100(1, 100);

  std::unordered_set<std::pair<vertex_id_type, vertex_id_type>> table;

  for (auto edge = rmat.begin(), end = rmat.end();
       edge != end;
       ++edge) {
    const vertex_id_type& src = std::get<0>(*edge);
    const vertex_id_type& trg = std::get<1>(*edge);

    const bool is_delete = (rand100(mt) <= delete_ratio);

    if (is_delete) {
      const bool is_deleted = table.erase(std::make_pair(src, trg));
      assert(graphstore.erase_edge(src, trg) == is_deleted);

    } else {
      const auto ret = table.insert(std::make_pair(src, trg));
      const bool is_inserted = ret.second;
      assert(graphstore.insert_edge(src, trg, 1) == is_inserted);
    }
  }


  /// check all elements
  /// can iterate all elements?
  {
    std::unordered_set<std::pair<vertex_id_type, vertex_id_type>> tmp_table(table);
    for (auto v_itr = graphstore.vertices_begin(), v_end = graphstore.vertices_end();
         v_itr != v_end;
         ++v_itr) {
      const vertex_id_type& src = v_itr.source_vertex();
      for (auto e_itr = graphstore.adjacent_edge_begin(src), e_end = graphstore.adjacent_edge_end(src);
           e_itr != e_end;
           ++e_itr) {
        const vertex_id_type& trg = e_itr.target_vertex();
        assert(tmp_table.erase(std::make_pair(src, trg)));
      }
    }
    assert(tmp_table.size() == 0);
  }


  /// check all elements from an another direction
  {
    for (const auto& edge : table) {
      assert(graphstore.erase_edge(edge.first, edge.second));
    }
    assert(graphstore.num_edges() == 0);
  }

}



/// duplicated insertion test cases
void test2(graphstore_type& graphstore, size_t num_vertices, size_t num_edges, size_t fact_dup)
{
  /// insertion and deletion
  {
    /// init (note the order)
    for (size_t d = 0; d < fact_dup; ++d) {
      for (uint64_t j = 0; j < num_edges; ++j) {
        for (uint64_t i = 0; i < num_vertices; ++i) {
          graphstore.insert_edge_dup(i, j, i+2);
        }
      }
    }
    /// num edges
    assert(graphstore.num_edges() == num_vertices * num_edges * fact_dup);
//    graphstore.print_status(1);

    /// delete all edges  (note the order)
    for (uint64_t j = 0; j < num_edges; ++j) {
      for (uint64_t i = 0; i < num_vertices; ++i) {
        assert(graphstore.erase_edge_dup(i, j) == fact_dup);
      }
    }

    /// num edges
    assert(graphstore.num_edges() == 0);
  }


  /// vertex
  {
    /// init
    for (size_t d = 0; d < fact_dup; ++d) {
      for (uint64_t i = 0; i < num_vertices; ++i) {
        for (uint64_t j = 0; j < num_edges; ++j) {
          graphstore.insert_edge_dup(i, j, i+2);
        }
      }
    }

    {
      std::vector<bool> flags(num_vertices, false);
      for (auto itr = graphstore.vertices_begin(), end = graphstore.vertices_end(); itr != end; ++itr) {
        flags[itr.source_vertex()] = true;
      }
      for (const auto flg : flags) assert(flg);
    }

    /// update vertex property
    for (auto itr = graphstore.vertices_begin(), end = graphstore.vertices_end(); itr != end; ++itr) {
      itr.property_data() = itr.source_vertex() + 2;
    }

    /// check vertex property
    {
      std::vector<bool> flags(num_vertices, false);
      for (auto itr = graphstore.vertices_begin(), end = graphstore.vertices_end(); itr != end; ++itr) {
        assert(itr.property_data() == itr.source_vertex() + 2);
        flags[itr.property_data() - 2] = true;
      }
      for (const auto flg : flags) assert(flg);
    }

    /// delete all edges
    for (uint64_t i = 0; i < num_vertices; ++i) {
      for (uint64_t j = 0; j < num_edges; ++j) {
        assert(graphstore.erase_edge_dup(i, j) == fact_dup);
      }
    }
    assert(graphstore.num_edges() == 0);
  }


  /// edge
  {
    /// init
    for (size_t d = 0; d < fact_dup; ++d) {
      for (uint64_t i = 0; i < num_vertices; ++i) {
        for (uint64_t j = 0; j < num_edges; ++j) {
          graphstore.insert_edge_dup(i, j, j+2);
        }
      }
    }

    /// adjacent edge iterator
    {
      for (uint64_t i = 0; i < num_vertices; ++i) {
        std::vector<size_t> cnts1(num_edges, 0);
        std::vector<size_t> cnts2(num_edges, 0);
        for (auto adj_edge = graphstore.adjacent_edge_begin(i), end = graphstore.adjacent_edge_end(i);
             adj_edge != end;
             ++adj_edge) {
          assert(adj_edge.target_vertex() + 2 == adj_edge.property_data());
          ++cnts1[adj_edge.target_vertex()];
          ++cnts2[adj_edge.property_data() - 2];
          ++adj_edge.property_data(); /// set new value via adjacent iterator
        }
        for (auto cnt : cnts1) assert(cnt == fact_dup);
        for (auto cnt : cnts2) assert(cnt == fact_dup);
      }

      for (uint64_t i = 0; i < num_vertices; ++i) {
        for (auto adj_edge = graphstore.adjacent_edge_begin(i), end = graphstore.adjacent_edge_end(i);
             adj_edge != end;
             ++adj_edge) {
          assert(adj_edge.property_data() == adj_edge.target_vertex() + 3);
        }
      }
    }

    /// delete all edges
    for (uint64_t i = 0; i < num_vertices; ++i) {
      for (uint64_t j = 0; j < num_edges; ++j) {
        assert(graphstore.erase_edge_dup(i, j) == fact_dup);
      }
    }
    assert(graphstore.num_edges() == 0);
  }
}


/// basic test cases
void test1(graphstore_type& graphstore, size_t num_vertices, size_t num_edges)
{

  /// unique insertion and deletion
  {
    /// init (note the order)
    for (uint64_t j = 0; j < num_edges; ++j) {
      for (uint64_t i = 0; i < num_vertices; ++i) {
        assert(graphstore.insert_edge(i, j, i+2));
      }
    }
    /// num edges
    assert(graphstore.num_edges() == num_vertices * num_edges);

    /// unique insertion
    for (uint64_t i = 0; i < num_vertices; ++i) {
      for (uint64_t j = 0; j < num_edges; ++j) {
        assert(!graphstore.insert_edge(i, j, i+2));
      }
    }
    /// num edges
    assert(graphstore.num_edges() == num_vertices * num_edges);

    /// delete all edges  (note the order)
    for (uint64_t j = 0; j < num_edges; ++j) {
      for (uint64_t i = 0; i < num_vertices; ++i) {
        assert(graphstore.erase_edge(i, j));
      }
    }
    /// num edges
    assert(graphstore.num_edges() == 0);
  }


  /// vertex
  {
    /// init
    for (uint64_t i = 0; i < num_vertices; ++i) {
      for (uint64_t j = 0; j < num_edges; ++j) {
        assert(graphstore.insert_edge(i, j, i+2));
      }
    }

    {
      std::vector<bool> flags(num_vertices, false);
      for (auto itr = graphstore.vertices_begin(), end = graphstore.vertices_end(); itr != end; ++itr) {
        flags[itr.source_vertex()] = true;
      }
      for (const auto flg : flags) assert(flg);
    }

    /// update vertex property
    for (auto itr = graphstore.vertices_begin(), end = graphstore.vertices_end(); itr != end; ++itr) {
      itr.property_data() = itr.source_vertex() + 2;
    }

    /// check vertex property
    {
      std::vector<bool> flags(num_vertices, false);
      for (auto itr = graphstore.vertices_begin(), end = graphstore.vertices_end(); itr != end; ++itr) {
        assert(itr.property_data() == itr.source_vertex() + 2);
        flags[itr.property_data() - 2] = true;
      }
      for (const auto flg : flags) assert(flg);
    }

    /// delete all edges
    for (uint64_t i = 0; i < num_vertices; ++i) {
      for (uint64_t j = 0; j < num_edges; ++j) {
        assert(graphstore.erase_edge(i, j));
      }
    }
    assert(graphstore.num_edges() == 0);
  }


  /// edge
  {
    /// init
    for (uint64_t i = 0; i < num_vertices; ++i) {
      for (uint64_t j = 0; j < num_edges; ++j) {
        assert(graphstore.insert_edge(i, j, j+2));
      }
    }

    /// adjacent edge iterator
    {
      for (uint64_t i = 0; i < num_vertices; ++i) {
        std::vector<bool> flags1(num_edges, false);
        std::vector<bool> flags2(num_edges, false);
        for (auto adj_edge = graphstore.adjacent_edge_begin(i), end = graphstore.adjacent_edge_end(i);
             adj_edge != end;
             ++adj_edge) {
          assert(adj_edge.target_vertex() + 2 == adj_edge.property_data());
          flags1[adj_edge.target_vertex()] = true;
          flags2[adj_edge.property_data() - 2] = true;
          ++adj_edge.property_data(); /// set new value via adjacent iterator
        }
        for (const auto flg : flags1) assert(flg);
        for (const auto flg : flags2) assert(flg);
      }

      for (uint64_t i = 0; i < num_vertices; ++i) {
        for (auto adj_edge = graphstore.adjacent_edge_begin(i), end = graphstore.adjacent_edge_end(i);
             adj_edge != end;
             ++adj_edge) {
          assert(adj_edge.property_data() == adj_edge.target_vertex() + 3);
        }
      }
    }

    /// delete all edges
    for (uint64_t i = 0; i < num_vertices; ++i) {
      for (uint64_t j = 0; j < num_edges; ++j) {
        assert(graphstore.erase_edge(i, j));
      }
    }
    assert(graphstore.num_edges() == 0);
  }
}


#define run_time(DSC, FNC) \
  do {\
    const double ts = MPI_Wtime();\
    std::cout << "\n=================================================================" << std::endl;\
    std::cout << "Run a test: " << DSC << std::endl;\
    FNC;\
    const double te = MPI_Wtime();\
    std::cout << " done: " << te - ts << " sec" << std::endl;\
    std::cout << "=================================================================" << std::endl;\
  } while(0)

int main(int argc, char** argv) {
  int mpi_rank(0), mpi_size(0);

  havoqgt::havoqgt_init(&argc, &argv);
  {
  CHK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank));
  CHK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &mpi_size));
  havoqgt::get_environment();

  assert(mpi_size == 1);

  std::cout << "MPI initialized with " << mpi_size << " ranks." << std::endl;
  havoqgt::get_environment().print();
  MPI_Barrier(MPI_COMM_WORLD);


  /// --- parse argments ---- ///
  std::string segmentfile_name;
  std::vector<std::string> edgelist_files;
  parse_cmd_line(argc, argv, segmentfile_name, edgelist_files);
  MPI_Barrier(MPI_COMM_WORLD);


  /// --- create a segument file --- ///
  size_t graph_capacity = std::pow(2, 32);
  std::stringstream fname_local_segmentfile;
  fname_local_segmentfile << segmentfile_name << "_" << mpi_rank;
  graphstore::utility::interprocess_mmap_manager::delete_file(fname_local_segmentfile.str());
  graphstore::utility::interprocess_mmap_manager mmap_manager(fname_local_segmentfile.str(), graph_capacity);

  /// --- allocate a graphstore --- ///
  graphstore_type graphstore(mmap_manager.get_segment_manager());

  run_time("can handle basic operations on a low degree graph?", test1(graphstore, 1024, 2));
  run_time("can handle long probe distances on the graph?", test2(graphstore, 1024, 1, middle_high_degree_threshold));

  run_time("can handle basic operations on a middle-high degree graph?", test1(graphstore, 1024, middle_high_degree_threshold * 2)); // There is a possibility of long probedistances
  run_time("can handle long probe distances on the graph?", test2(graphstore, 1024, middle_high_degree_threshold * 2, 64));

//  run_time("can handle basic operations on a large graph?", test1(graphstore, 1<<22ULL, 128));
//  run_time("can handle long probe distances on the graph?", test2(graphstore, 1<<22ULL, 128, 64));

  run_time("can handle basic operations on a rmat graph?", test3(graphstore, 17, 4, 10));
  run_time("can handle duplicate operations on the graph?", test4(graphstore, 17, 4, 10));

  std::cout << "All tests completed!!!" << std::endl;
  }  // END Main MPI
  havoqgt::havoqgt_finalize();

  return 0;
}
