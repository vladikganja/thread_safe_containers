cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release -DARGPARSE_BUILD_SAMPLES=on -DARGPARSE_BUILD_TESTS=on -DCMAKE_CXX_FLAGS="-fsanitize=thread" -DLOG_ENABLED=on ..

cmake --build . --config Release
