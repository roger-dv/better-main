# Taking Jason Turner's Better `main' A Step Further

Well known C++ trainer, C++ conference presenter, C++ book author, and former C++ podcaster, Jason Turner, recently took up the idea of fashioning a `main()` function that is better suited to Modern C++:

**C++ Weekly - Ep 361 - Is A Better `main` Possible?**  
Jan 30, 2023  
[https://www.youtube.com/watch?v=zCzD9uSDI8c](https://www.youtube.com/watch?v=zCzD9uSDI8c)

Jason incremented on his thoughts for this better `main()`, a first iteration looking like so:
```C++
  [[nodiscard]] int better_main(const std::span<const std::string_view> &args);
```

This first iteration relied on `std::vector` as the backing for the `std::span<> args`:
```C++
  std::vector<std::string_view> args(argv, std::next(argv, static_cast<std::ptrdiff_t>(argc)));
  better_main(args);
```

Jason wasn't totally satisfied with using `std::vector<>` due that it would be allocating memory from the heap. So Jason turned to looking at using `std::array<>` such that its memory for its `std::string_view` elements would be stack based. However, the specifying of a `std::array<>` would require using a literal integer numeric to specify its size - the `argc` parameter would not be such. Jason then took a guess at what an adequate size would be and tried `255` thinking that would probably be more than adequate. And then he further mused about the possibility of using `pmr` memory management as a possible next progression.

That is where this implementation of a better `main()` takes up the flag. I go back to using `vector<>`, however, the `pmr` variant:
```C++
  std::pmr::vector<std::string_view> args{&pmr_alloc};
```

I wouldn't agree that allocating `std::array<>` to be `255` would suffice, for if the program in question were invoked by the Unix `xargs` tool, it could potentially exceed that limitation (the output of, say, the `find` tool is frequently piped to `xargs` and there could be many thousands of discreet arguments to be processed in that kind of scenario).

I implement a customized `pmr` allocator by deriving from the `pmr` abstract base class:
```C++
  class my_cust_allocator : public std::pmr::memory_resource {...}
```
This custom allocator will insure that the elements of the `std::pmr::vector<> args` will be stack allocated. But the stack allocated storage has to be done prior to constructing this custom allocator, like so:
```C++
  const size_t buf_size = (argc + 1) * sizeof(std::string_view);
  void * const buf = alloca(buf_size);
```
The buffer pointer from this `alloca()` call is passed in to initialize the customer `pmr` allocator - to then be used as its memory buffer resource for fulfilling allocation request.

There is one more detail that is important - that is the filling in of the vector of args:
```C++
  args.reserve(argc);
  for(int i = 0; i < argc; i++) {
    args.emplace_back(argv[i]);
  }
```

The important step here is this statement `'args.reserve(argc);'` prior to iterating to fill in the args vector.

The `reserve()` call will up front cause a call to the `pmr` custom allocator - just one time and just sufficient to hold all the arg elements. If one instead just proceeded to incrementally `emplace()` elements at the end of the vector, this would result in a doubling algorithm for when the vector buffer needs to be enlarged. That would cause the overall memory request for the vector to exceed the stack memory buffer. The up front `reserve()` call prevents that from happening.

Now the `std::span<const std::string_view> args` passed to `better_main()` is entirely stack resident and it is sized based on the `argc` parameter passed to the C-style `main()`.

In theory this further improved implementation of the old-style `main()` could be rolled into the C++ runtime and we C++ programers would start using `better_main()` as the entry point to our Modern C++ programs - though perhaps we might want to rename it to, say, `cpp_main()`?
