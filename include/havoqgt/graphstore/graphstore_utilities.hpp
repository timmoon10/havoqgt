/*
 * Written by Keita Iwabuchi.
 * LLNL / TokyoTech
 */
#ifndef GRAPHSTORE_UTILITIES_HPP
#define GRAPHSTORE_UTILITIES_HPP

#include <iostream>
#include <algorithm>
#include <string>
#include <cstring>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/mapped_region.hpp>

#if __linux__
#include <sys/sysinfo.h>
#endif


#define ENABLE_HAVOQGT_ERR_PROCEDURE 1
#if ENABLE_HAVOQGT_ERR_PROCEDURE
#include <havoqgt/error.hpp>
#endif


#define DISP_LOG(msg) \
  do { \
    std::cout << "DEG: " << __FILE__ << "(" << __LINE__ << ") " << msg << std::endl; } \
  while (0)

#define DISP_LOG_VAR(x) \
  do { \
    std::cout << "DEG: " << __FILE__ << "(" << __LINE__ << ") " << #x << " =\t" << x << std::endl; } \
  while (0)


namespace graphstore {
namespace utility {
/// --------- Deta Structure ------------ ///
#pragma pack(1)
template <typename T1, typename T2>
struct packed_pair
{
  T1 first;
  T2 second;

  packed_pair() {}

  packed_pair(const T1& t1, const T2& t2) :
    first(t1),
    second(t2)
  {}

  packed_pair(T1&& t1, T2&& t2) :
    first(std::move(t1)),
    second(std::move(t2))
  {}

  packed_pair(const packed_pair<T1, T2>& other) :
    first(other.first),
    second(other.second)
  {}

  packed_pair(packed_pair<T1, T2>&& other) noexcept :
    first(std::move(other.first)),
    second(std::move(other.second))
  {}

  packed_pair<T1, T2>& operator=(const packed_pair<T1, T2>& other)
  {
    first = other.first;
    second = other.second;
    return *this;
  }

  packed_pair<T1, T2>& operator=(packed_pair<T1, T2>&& other)
  {
    first = std::move(other.first);
    second = std::move(other.second);
    return *this;
  }

  void swap(packed_pair<T1, T2>& other)
  {
    using std::swap;
    swap(first, other.first);
    swap(second, other.second);
  }

};

template<typename T1, typename T2>
inline bool operator==(const packed_pair<T1, T2>& lhs, const packed_pair<T1, T2>& rhs) {
    return lhs.first == rhs.first && lhs.second == rhs.second;
}
template<typename T1, typename T2>
inline bool operator!=(const packed_pair<T1, T2>& lhs, const packed_pair<T1, T2>& rhs) {
    return !(lhs == rhs);
}

template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& stream, const packed_pair<T1, T2>& obj)
{
    stream << obj.first << ":" << obj.second;
    return stream;
}

#pragma pack(1)
template <typename T1, typename T2, typename T3, typename T4>
struct packed_tuple
{
  T1 first;
  T2 second;
  T3 third;
  T4 fourth;

  using self_type = packed_tuple<T1, T2, T3, T4>;

  /// ---- Constructors ----
  packed_tuple() {}

  packed_tuple(const T1& t1, const T2& t2, const T3& t3, const T4& t4) :
    first(t1),
    second(t2),
    third(t3),
    fourth(t4)
  {}

  packed_tuple(T1&& t1, T2&& t2, T3&& t3, T4&& t4) :
    first(std::move(t1)),
    second(std::move(t2)),
    third(std::move(t3)),
    fourth(std::move(t4))
  {}

  /// Copy constructor
  packed_tuple(const self_type& other) :
    first(other.first),
    second(other.second),
    third(other.third),
    fourth(other.fourth)
  {}

  /// Move constructor
  packed_tuple(self_type&& other) noexcept :
    first(std::move(other.first)),
    second(std::move(other.second)),
    third(std::move(other.third)),
    fourth(std::move(other.fourth))
  {}

  /// Copy assignment operators
  self_type& operator=(const self_type& other)
  {
    first  = other.first;
    second = other.second;
    third  = other.third;
    fourth = other.fourth;
    return *this;
  }

  /// Move assignment operators
  self_type& operator=(self_type&& other)
  {
    first  = std::move(other.first);
    second = std::move(other.second);
    third  = std::move(other.third);
    fourth = std::move(other.fourth);
    return *this;
  }

  void swap(self_type& other)
  {
    using std::swap;
    swap(first, other.first);
    swap(second, other.second);
    swap(third,  other.third);
    swap(fourth,  other.fourth);
  }

};

template<typename T1, typename T2, typename T3, typename T4>
inline bool operator==(const packed_tuple<T1, T2, T3, T4>& lhs, const packed_tuple<T1, T2, T3, T4>& rhs) {
    return lhs.first == rhs.first && lhs.second == rhs.second &&
           lhs.third == rhs.third && lhs.fourth == rhs.fourth;
}

template<typename T1, typename T2, typename T3, typename T4>
inline bool operator!=(const packed_tuple<T1, T2, T3, T4>& lhs, const packed_tuple<T1, T2, T3, T4>& rhs) {
    return !(lhs == rhs);
}

template<typename T1, typename T2, typename T3, typename T4>
std::ostream& operator<<(std::ostream& stream, const packed_tuple<T1, T2, T3, T4>& obj)
{
    stream << obj.first << ":" << obj.second << ":" << obj.third << ":" << obj.fourth;
    return stream;
}

#pragma pack(1)
template <typename T1, typename T2, typename T3>
struct packed_tuple <T1, T2, T3, void>
{
  T1 first;
  T2 second;
  T3 third;

  using self_type = packed_tuple<T1, T2, T3, void>;

  /// ---- Constructors ----
  packed_tuple() {}

  packed_tuple(const T1& t1, const T2& t2, const T3& t3) :
    first(t1),
    second(t2),
    third(t3)
  {}

  packed_tuple(T1&& t1, T2&& t2, T3&& t3) :
    first(std::move(t1)),
    second(std::move(t2)),
    third(std::move(t3))
  {}

  /// Copy constructor
  packed_tuple(const self_type& other) :
    first(other.first),
    second(other.second),
    third(other.third)
  {}

  /// Move constructor
  packed_tuple(self_type&& other) noexcept :
    first(std::move(other.first)),
    second(std::move(other.second)),
    third(std::move(other.third))
  {}

  /// Copy assignment operators
  self_type& operator=(const self_type& other)
  {
    first = other.first;
    second = other.second;
    third =  other.third;
    return *this;
  }

  /// Move assignment operators
  self_type& operator=(self_type&& other)
  {
    first = std::move(other.first);
    second = std::move(other.second);
    third =  std::move(other.third);
    return *this;
  }

  void swap(self_type& other)
  {
    using std::swap;
    swap(first, other.first);
    swap(second, other.second);
    swap(third,  other.third);
  }

};

template<typename T1, typename T2, typename T3>
inline bool operator==(const packed_tuple<T1, T2, T3, void>& lhs, const packed_tuple<T1, T2, T3, void>& rhs) {
    return lhs.first == rhs.first && lhs.second == rhs.second &&
           lhs.third == rhs.third;
}

template<typename T1, typename T2, typename T3>
inline bool operator!=(const packed_tuple<T1, T2, T3, void>& lhs, const packed_tuple<T1, T2, T3, void>& rhs) {
    return !(lhs == rhs);
}

template<typename T1, typename T2, typename T3>
std::ostream& operator<<(std::ostream& stream, const packed_tuple<T1, T2, T3, void>& obj)
{
    stream << obj.first << ":" << obj.second << ":" << obj.third;
    return stream;
}


template <typename T, std::size_t SIZE>
size_t array_length(const T (&)[SIZE])
{
    return SIZE;
}


template<typename array_type, typename key_type>
size_t binary_search(array_type& array, size_t len, key_type& key)
{
  size_t left = 0;
  size_t right = len;

  while (left <= right) {
    const size_t center = (right + left) / 2;
    if (array[center] == key) return center;

    if (key < array[center]) {
      right = center;
    } else {
      left = center;
    }
  }

  return len;
}


/// --------- For Debug --------------- ///
static void print_time() {
  std::time_t result = std::time(nullptr);
  std::cout << std::asctime(std::localtime(&result)); /// don't need std::endl
}

static std::chrono::high_resolution_clock::time_point duration_time()
{
  return std::chrono::high_resolution_clock::now();
}

static uint64_t duration_time_usec(const std::chrono::high_resolution_clock::time_point& tic)
{
  auto duration_time = std::chrono::high_resolution_clock::now() - tic;
  return std::chrono::duration_cast<std::chrono::microseconds>(duration_time).count();
}

static double duration_time_sec(const std::chrono::high_resolution_clock::time_point& tic)
{
  auto duration_time = std::chrono::high_resolution_clock::now() - tic;
  return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(duration_time).count() / 1000000.0);
}

class rhh_log_holder {

 public:

  using self_type = rhh_log_holder;

  static self_type& instance()
  {
    static self_type _instance;
    return _instance;
  }

  void tally()
  {
    ++max_distance_to_moved_histgram[max_distance_to_moved];
    max_distance_to_moved = 0;
  }

  void init()
  {
    max_distance_to_moved = 0;
    for (int i = 0; i < 1024; ++i) {
      max_distance_to_moved_histgram[i] = 0;
    }
  }

  size_t max_distance_to_moved;
  size_t max_distance_to_moved_histgram[1024];


 private:

  rhh_log_holder() {}

  rhh_log_holder(const self_type &)  = delete;
  rhh_log_holder(const self_type &&) = delete;
  self_type &operator=(const self_type &)   = delete;
  self_type &operator=(const self_type &&)  = delete;

};


/// -------------- For I/O ---------------- ///

static void sync_files()
{
#if _BSD_SOURCE || _XOPEN_SOURCE >= 500
  std::cout << "sync mmap: sync(2)" << std::endl;
  ::sync();
#else
#warning sync(2) is not supported
#endif
}

static int aligned_alloc(void** actual_buffer, size_t align_size, size_t length)
{
    int result = ::posix_memalign(actual_buffer, align_size, length);
    if (result != 0) {
#if ENABLE_HAVOQGT_ERR_PROCEDURE
        HAVOQGT_ERROR_MSG("Failed posix_memalign");
#else
        std::cerr << "Failed posix_memalign";
        ::exit(1);
#endif
    }
    return result;
}

static void aligned_free(void* ptr)
{
    ::free(ptr);
}


template <size_t AlignSize>
class direct_file_reader
{
 public:
    explicit direct_file_reader()
        : m_fd(-1)
        , m_buffer(nullptr)
        , m_buf_size(0)
        , m_current_buf_pos(0)
        , m_end_buf_pos(0)
        , m_is_reached_eof(false)
    { }

    ~direct_file_reader()
    {
        close_file();
        free_buffer();
    }

    /// --- Disable copy and move assigment and constructor --- ///
    direct_file_reader& operator=(direct_file_reader const&) = delete;
    direct_file_reader& operator=(direct_file_reader const&&) = delete;
    direct_file_reader(direct_file_reader const&) = delete;
    direct_file_reader(direct_file_reader const&&) = delete;

    bool set_file(const char* const fname)
    {
        free_buffer();
        alloc_buffer(kInitBufSize);
        return open_file_with_direct_io_mode(fname);
    }


    bool getline(std::string& str)
    {

        while (1) {
            ssize_t line_start_pos;
            ssize_t line_length;
            const bool is_line_found = find_line_from_bufffer(line_start_pos, line_length);

            if (is_line_found) {
                copy_string_to_userscape_buffer(str, line_start_pos, line_length);
                return true;
            }

            if (!m_is_reached_eof) {
                const off_t length_remains = m_end_buf_pos - line_start_pos;
                seek(-length_remains, SEEK_CUR);
                aligned_read(kChunkSize);
            } else {
                if (m_end_buf_pos-line_start_pos > 0ULL) {
                    /// Now, the file is reachaed to EOF, Therefore if data is remaining in the buffer, copy it to the user space
                    copy_string_to_userscape_buffer(str, line_start_pos, m_end_buf_pos-line_start_pos);
                    return true;
                } else {
                    /// File has already reached to EOF and there is no remaining data in the buffer
                    return false;
                }
            }
        }

        /// File has already reached to EOF and there is no remaining data in the buffer
        return false;
    }


 private:
    enum {
        kChunkSize = (1ULL << 25),
        kInitBufSize = kChunkSize + 2ULL * AlignSize
    };


    void alloc_buffer(const size_t size)
    {
        aligned_alloc(&m_buffer, AlignSize, size);
        m_buf_size = size;
    }

    void free_buffer()
    {
        if (m_buffer != nullptr)
            aligned_free(m_buffer);
        m_buffer = nullptr;
        m_buf_size = 0;
        m_current_buf_pos = 0;
        m_end_buf_pos = 0;
    }

    bool open_file_with_direct_io_mode(const char* const fname)
    {
        if (m_fd != -1)
            close_file();

#ifdef O_DIRECT
        const int flags = O_RDONLY | O_DIRECT;
#else
        const int flags = O_RDONLY;
        std::cerr << "O_DIRECT is not suported\n";
        std::cerr << "will use normal I/O\n";
#endif
        m_fd = ::open(fname, flags);
        if (m_fd == -1) {
            std::cerr << fname << std::endl;
            perror("Failed file open");
            return false;
        }

        return true;
    }

    void close_file()
    {
        m_fd = -1;
        ::close(m_fd);
    }

    off_t seek(off_t offset, int whence)
    {
        return ::lseek(m_fd, offset, whence);
    }

    /// Return: On success, the number of bytes read is returned
    ///         On error, -1 is returned
    ssize_t aligned_read(const size_t count)
    {
        /// Align the file offset of I/Os
        const off_t cur_pos = lseek(m_fd, 0, SEEK_CUR);
        const size_t off = cur_pos % AlignSize;
        seek(-off, SEEK_CUR);
        m_current_buf_pos = off;

        /// Align the lenght of read
        const size_t raw_read_size = count + off;
        const size_t aligned_read_size = calc_aligned_size(raw_read_size, AlignSize);

        /// If buffer is not allocated or current buffer is small, then allocate new
        if (m_buffer == nullptr || (m_buf_size <  aligned_read_size)) {
            free_buffer();
            alloc_buffer(aligned_read_size);
        }

        /// read data from the file using read(2)
        ssize_t read_size = ::read(m_fd, m_buffer, aligned_read_size);
        m_end_buf_pos = read_size;
        if (read_size < aligned_read_size) {
            m_is_reached_eof = true;
        }

        /// discarded unnecessary data
        if (read_size > 0) {
            read_size -= off;
        }


        return read_size;
    }

    bool find_line_from_bufffer(ssize_t& line_start_pos, ssize_t& line_length)
    {
        skip_contiguous_charctor('\n');
        char* const buf = reinterpret_cast<char*>(m_buffer);
        line_start_pos = m_current_buf_pos;
        for (; m_current_buf_pos < m_end_buf_pos; ++m_current_buf_pos) {
            if (buf[m_current_buf_pos] == '\n') {
                line_length = m_current_buf_pos - line_start_pos;
                return true;
            }
        }
        return false;
    }

    /// Example:
    ///  input: x = 0101, align_size = 10
    ///  output: 0110
    ///  Note: this function dose not check if align_size is power of 2
    inline size_t calc_aligned_size(const size_t x, const size_t align_size)
    {
        const size_t y = (align_size - 1);
        return ((x + y) & (~y));
    }

    void copy_string_to_userscape_buffer(std::string& str, const size_t start_pos, const size_t length)
    {
        char* buf = reinterpret_cast<char*>(m_buffer);
        str.clear();
        str.append(&buf[start_pos], length);
    }

    inline void skip_contiguous_charctor(const char delim)
    {
        char* buf = reinterpret_cast<char*>(m_buffer);
        for (; m_current_buf_pos < m_end_buf_pos; ++m_current_buf_pos) {
            if (buf[m_current_buf_pos] != delim) break;
        }
    }

    int m_fd;
    void *m_buffer;
    size_t m_buf_size;
    size_t m_current_buf_pos;
    ssize_t m_end_buf_pos;
    bool m_is_reached_eof;
};


/// --------- Boost Interprocess ------------ ///

template<typename segment_manager_type>
static double segment_size_gb(const segment_manager_type* const segment_manager)
{
  const size_t usages = segment_manager->get_size() - segment_manager->get_free_memory();
  return static_cast<double>(usages) / (1ULL << 30);
}

class interprocess_mmap_manager
{
 public:
  using mapped_file_type     = boost::interprocess::managed_mapped_file;
  using segment_manager_type = boost::interprocess::managed_mapped_file::segment_manager;

  interprocess_mmap_manager(const std::string segment_fname, const size_t capacity, const bool is_call_fallocate = true, const bool is_delete_file_on_exit = false) :
    m_fname(segment_fname),
    m_mapped_file(boost::interprocess::create_only, segment_fname.c_str(), capacity),
    m_is_delete_file_on_exit(is_delete_file_on_exit)
  {
    if (is_call_fallocate) {
      fallocate(capacity);
    }
  }

  ~interprocess_mmap_manager()
  {
    if (m_is_delete_file_on_exit) {
      delete_mapping();
    }
  }

  static void delete_file(const std::string& fname_segmentfile)
  {
    boost::interprocess::file_mapping::remove(fname_segmentfile.c_str());
  }

  segment_manager_type* get_segment_manager() const
  {
    return m_mapped_file.get_segment_manager();
  }

  double segment_size_gb() const
  {
    segment_manager_type* segment_manager = m_mapped_file.get_segment_manager();
    const size_t usages = segment_manager->get_size() - segment_manager->get_free_memory();
    return static_cast<double>(usages) / (1ULL << 30);
  }

  void zero_free_segment_memory ()
  {
    zero_free_memory();
  }

 private:
  void zero_free_memory()
  {
      segment_manager_type* segment_manager = m_mapped_file.get_segment_manager();
      std::cout << "Call segment_manager.zero_free_memory()" << std::endl;
      segment_manager->zero_free_memory();
      std::cout << "Call mapped_file.flush()" << std::endl;
      m_mapped_file.flush();
      std::cout << "Call sync" << std::endl;
      graphstore::utility::sync_files();
  }

  void fallocate(size_t size)
  {
  #ifdef __linux__
      std::cout << "Call fallocate()" << std::endl;
      int fd  = ::open(m_fname.c_str(), O_RDWR);
      assert(fd != -1);
      /// posix_fallocate dosen't work on XFS ?
      /// (dosen't actually expand the file size ?)
      /// int ret = ::fallocate(fd, 0, 0, size);
      int ret = posix_fallocate(fd, 0, size);
      assert(ret == 0);
      ::close(fd);
      m_mapped_file.flush();
  #else
  #warning fallocate() is not supported
  #endif
  }

  void flush(mapped_file_type& mapped_file)
  {
    std::cout << "flush mmap" << std::endl;
    m_mapped_file.flush();
  }

  void mmap_dontneed()
  {
    assert(false); /// TODO: implementation
//    std::cout << "Call advise_dontneed" << std::endl;
//    boost::interprocess::mapped_region::advice_types advise = boost::interprocess::mapped_region::advice_types::advice_dontneed;
//    boost::interprocess::mapped_region mregion(m_mapped_file, boost::interprocess::read_write);
//    mregion.advise(advise);
  }

  void delete_mapping()
  {
    boost::interprocess::file_mapping::remove(m_fname.c_str());
  }

  std::string m_fname;
  mapped_file_type m_mapped_file;
  bool m_is_delete_file_on_exit;
};



//// ---------- System Status --------- ///
static bool get_system_memory_usages( size_t* const mem_unit, size_t* const totalram, size_t* const freeram, size_t* const usedram,
                               size_t* const bufferram, size_t* const totalswap, size_t* const freeswap )
{
  bool is_succeed = false;
#if __linux__
  struct sysinfo info;
  if (sysinfo(&info) == 0) {
    *mem_unit = static_cast<size_t>(info.mem_unit);
    *totalram = static_cast<size_t>(info.totalram);
    *freeram = static_cast<size_t>(info.freeram);
    *usedram = static_cast<size_t>(info.totalram - info.freeram);
    *bufferram = static_cast<size_t>(info.bufferram);
    *totalswap = static_cast<size_t>(info.totalswap);
    *freeswap = static_cast<size_t>(info.freeswap);
    is_succeed = true;
  }
#endif
  return is_succeed;
}

static bool get_my_memory_usages(size_t* const size, size_t* const resident, size_t* const share,
                          size_t* const text, size_t* const lib, size_t* const data, size_t* const dt)
{
  bool is_succeed = false;
#ifdef __linux__
  const char* statm_path = "/proc/self/statm";
  FILE *f = fopen(statm_path, "r");
  if (f) {
    if (7 == fscanf(f,"%ld %ld %ld %ld %ld %ld %ld", size, resident, share, text, lib, data, dt)) {
      is_succeed = true;
    }
  }
  fclose(f);
#endif
  return is_succeed;
}

static bool get_stat_page_faults(size_t* minflt, size_t* majflt)
{
  bool is_succeed = false;
#ifdef __linux__
  const char* stat_path = "/proc/self/stat";
  FILE *f = fopen(stat_path, "r");
  if (f) {
    if (13 == fscanf(f,"%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %*lu %lu", minflt, majflt)) {
      is_succeed = true;
    }
  }
  fclose(f);
#endif
  return is_succeed;
}

///
/// \brief get_meminfo
/// \param key
///   exmample "MemTotal"
/// \return
///
static bool get_meminfo(std::string key, size_t& val) {

  key += ":";

  std::string token;
  std::ifstream file("/proc/meminfo");

  while(file >> token) {
      if(token == key) {
          size_t val;
          if(file >> val) {
              return true;
          } else {
              return false;
          }
      }
      file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return false;
}

} /// namespace utility
} /// namespace graphstore
#endif /// GRAPHSTORE_UTILITIES_HPP
