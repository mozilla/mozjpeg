#!/bin/bash

set -u
set -e

cmake . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_STATIC=1 -DENABLE_SHARED=0 \
	-DCMAKE_C_FLAGS_RELWITHDEBINFO="-g -DNDEBUG" \
	-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-g -DNDEBUG" -DCMAKE_INSTALL_PREFIX=$WORK \
	-DWITH_FUZZ=1 -DFUZZ_BINDIR=$OUT -DFUZZ_LIBRARY=$LIB_FUZZING_ENGINE
make "-j$(nproc)" "--load-average=$(nproc)"
make install

cp $SRC/compress_fuzzer_seed_corpus.zip $OUT/cjpeg_fuzzer_seed_corpus.zip
cp $SRC/compress_fuzzer_seed_corpus.zip $OUT/compress_fuzzer_seed_corpus.zip
cp $SRC/compress_fuzzer_seed_corpus.zip $OUT/compress_yuv_fuzzer_seed_corpus.zip
cp $SRC/decompress_fuzzer_seed_corpus.zip $OUT/libjpeg_turbo_fuzzer_seed_corpus.zip
cp $SRC/decompress_fuzzer_seed_corpus.zip $OUT/decompress_yuv_fuzzer_seed_corpus.zip
cp $SRC/decompress_fuzzer_seed_corpus.zip $OUT/transform_fuzzer_seed_corpus.zip
