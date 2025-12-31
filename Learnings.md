1. WORKSPACE is a specific configuration file used by the build system. It tells the build system about where the root of the directory is located.
2. Blaze is google internal build system, it creates a monorepo.
3. When writing tests in a professional environment, you are typically doing three things:

  Unit Testing: Isolating a single function or class (e.g., calculate_tax()) and proving it handles edge cases correctly (negatives, zeros, overflow).

  Regression Testing: ensuring that a new feature you added didn't break an old feature.  

  Mocking (using gMock): If your code needs to talk to a database or API, you "fake" (mock) that connection so your test runs instantly without internet.
4. .cc is same as .cpp 
5. How does a LSM Tree work ? Components: RAM and HardDisk. Keep Data in sorted format in RAM and write-ahead format in Disk. Once RAM is filled, create a binder out of it and store in Disk. After sometime you will have a lot of binders, so we perform compaction. Note: The WAL is cleared once Binder is created.

6. Types of Pointers: 
std::unique_ptr: When you need exclusive ownership (90% of use cases). The object belongs to one owner and dies when that owner dies.

std::shared_ptr: When you need shared ownership. Multiple parts of the code need to keep the object alive; it dies only when the last owner is done.

std::weak_ptr: When you need to break cycles in shared pointers (e.g., Parent owns Child, Child has weak_ptr to Parent) or observe an object without keeping it alive.

Raw Pointer (T*): When you need a non-owning view. Use this for function parameters or observing data when you know the object will outlive your usage.

void*: When you need to point to memory of unknown type (e.g., low-level memory allocators or C-style APIs).
11. #pragma once Its job is to ensure that the header file is included only once during the compilation of a single file, even if you include it multiple times in different places. We can do the same by using # ifndef headerfile and #define headerfile with #endif at last
12. char* points to a raw block of memory of 8 bits= 1byte.

13. What is the project in my own language: It consists of 3 parts, building a storage engine, post this we will convert our storage engine into a server. After this we will perform raft concensus. After this we add observability and benchmarking.

14. Header files are only to define the components, the working of the components is implemented in .cc files

15. We used pointers and arithmetic instead of new and delete because they are expensive operations

16. Why we use function.h and function.cc files ?: First all .cc  files are converted into .o files. A .o file is literally .c + .h written at its top and compiled. When a file is using a .h file it only puts placeholders and not the actual working of the .h file (which is in .cc file). This shows that .h files needs to be copied a lot of times but not its functioning. The functioning of the .h, the .cc file is compiled only once and used for everything. Post this what we realise is that all .o objects are then connected using a linker.
17. Because the arena and skiplist are for fundamentally different purpose both are not implemented in the same way, what I mean by this is that Skiplist will be a template class, we can't write a .cc file for it, wihtout a datatype it's not possible to write to write a .cc file.
Another question you might think is why not write a template .cc file. well that is not
how stuff works. The .cc file must have a data type.


