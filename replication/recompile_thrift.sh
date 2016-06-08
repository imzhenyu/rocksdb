#!/bin/bash

TMP_DIR=./tmp
rm -rf $TMP_DIR

mkdir -p $TMP_DIR
sh $DSN_ROOT/bin/dsn.cg.sh rrdb.thrift cpp $TMP_DIR replication
cp -v $TMP_DIR/rrdb.types.h src/
cp -v $TMP_DIR/rrdb.code.definition.h src/
cp -v $TMP_DIR/rrdb.check.h src/
cp -v $TMP_DIR/rrdb.server.h src/
cp -v $TMP_DIR/rrdb.client.h src/
cp -v $TMP_DIR/rrdb.client.perf.h src/
cp -v $TMP_DIR/rrdb.app.example.h src/
sed -i 's/# include "rrdb.client.perf.h"/# include "rrdb.client.perf.impl.h"/' src/rrdb.app.example.h
sed -i 's/new rrdb_perf_test_client(/new rrdb_perf_test_client_impl(/' src/rrdb.app.example.h
rm -rf $TMP_DIR

mkdir $TMP_DIR
$DSN_ROOT/bin/Linux/thrift-0.9.3 --gen cpp -out $TMP_DIR rrdb.thrift
sed 's/#include "dsn_types.h"/#include <dsn\/service_api_cpp.h>/' $TMP_DIR/rrdb_types.h >src/rrdb_types.h
echo "#ifdef DSN_USE_THRIFT_SERIALIZATION" >src/rrdb_types.cpp
cat $TMP_DIR/rrdb_types.cpp >>src/rrdb_types.cpp
echo "#endif" >>src/rrdb_types.cpp
rm -rf $TMP_DIR

echo "done"
