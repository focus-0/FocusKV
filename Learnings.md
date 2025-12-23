1. WORKSPACE is a specific configuration file used by the build system. It tells the build system about where the root of the directory is located.
2. Blaze is google internal build system, it creates a monorepo.
3. When writing tests in a professional environment, you are typically doing three things:

  Unit Testing: Isolating a single function or class (e.g., calculate_tax()) and proving it handles edge cases correctly (negatives, zeros, overflow).

  Regression Testing: ensuring that a new feature you added didn't break an old feature.  

  Mocking (using gMock): If your code needs to talk to a database or API, you "fake" (mock) that connection so your test runs instantly without internet.
4. .cc is same as .cpp 
5. How does a LSM Tree work ? Components: RAM and HardDisk. Keep Data in sorted format in RAM and write-ahead format in Disk. Once RAM is filled, create a binder out of it and store in Disk. After sometime you will have a lot of binders, so we perform compaction. Note: The WAL is cleared once Binder is created.



