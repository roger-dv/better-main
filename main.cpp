#include <span>
#include <memory_resource>
#include <string_view>
#include <iostream>

/**
 * A program that takes up the mantle from Jason Turner's idea for a
 * better `main`, one that would better suit Moder C++ programming.
 * <p>
 * The program here takes up Jason's suggestion of perhaps using pmr
 * memory allocation strategy, and that is done so here and in a way
 * that keeps all memory for the args vector resident on the stack
 * (i.e., no heap memory allocations are involved).
 * </p>
 * <p>
 * Refer to the README.md for more detailed explanation.
 * </p>
 *
 * @param argc number of string arguments passed via argv
 * @param argv array to command line arguments as strings
 *             (an array of character pointers)
 * @return the completion status code (zero indicates success)
 */
int main(int argc, const char *argv[]) {
  [[nodiscard]] int better_main(const std::span<const std::string_view> &args);

  std::cerr << "DEBUG: argc: " << argc << ", sizeof(std::string_view): " << sizeof(std::string_view) << '\n';

  const size_t buf_size = (argc + 1) * sizeof(std::string_view);
  void * const buf = alloca(buf_size);

  class my_cust_allocator : public std::pmr::memory_resource {
  private:
    void * const buf;
    size_t buf_size;
  public:
    const size_t max_buf_size;
    my_cust_allocator(void* buf, size_t buf_size) : buf(buf), buf_size(buf_size), max_buf_size(buf_size) {}
  private:
    virtual void* do_allocate(size_t bytes, size_t alignment) {
      if (bytes > buf_size) {
        std::cerr << "requested bytes: " << bytes << ", remaining bytes capacity: " << buf_size << '\n';
        throw std::bad_alloc();
      }
      buf_size -= bytes;
      return buf;
    }
    virtual void do_deallocate(void* p, size_t bytes, size_t alignment) {}
    virtual bool do_is_equal(const memory_resource& __other) const noexcept { return false; }
  }
  pmr_alloc{buf, buf_size};

  std::pmr::vector<std::string_view> args{&pmr_alloc};
  args.reserve(argc);
  for(int i = 0; i < argc; i++) {
    args.emplace_back(argv[i]);
  }

  return better_main(args);
}

/**
 * Here is an example of what the better `main` for Modern C++ would look like.
 * <p>
 * The implementation merely prints out the args that have been passed in (there will
 * always be at least one argument - the path to the program that's been invoked).
 * </p>
 * @param args a span of string_view arguments, representing command line arguments
 * @return the completion status code (zero indicates success)
 */
[[nodiscard]] int better_main([[maybe_unused]] const std::span<const std::string_view> &args) {

  // iterate and display each element of args as a quoted string
  auto const prn_arg = [&out=std::cout]
      (const std::string_view &arg) -> auto&
  {
    out << '"' << arg << '"';
    return out;
  };
  std::cout << "DEBUG: ";
  const int last_i = args.size() - 1;
  for(int i = 0; i < last_i; i++) {
    prn_arg(args[i]) << ' ';
  }
  prn_arg(args[last_i]) << '\n';

  return 0;
}