#!/bin/bash

TMP_DIR=./tmp
rm -rf $TMP_DIR

mkdir -p $TMP_DIR
sh $DSN_ROOT/bin/dsn.cg.sh rrdb.thrift cpp $TMP_DIR
cp -v $TMP_DIR/rrdb.types.h ../include/rrdb/
cp -v $TMP_DIR/rrdb.code.definition.h ../include/rrdb/
cp -v $TMP_DIR/rrdb.client.h ../include/rrdb/
sed 's/#include "dsn_types.h"/#include <dsn\/service_api_cpp.h>/' $TMP_DIR/rrdb_types.h > ../include/rrdb/rrdb_types.h
sed 's/#include "rrdb_types.h"/#include <rrdb\/rrdb_types.h>/' $TMP_DIR/rrdb_types.cpp > ../base/rrdb_types.cpp
sed 's/# include "rrdb.code.definition.h"/# include <rrdb\/rrdb.code.definition.h>/' $TMP_DIR/rrdb.server.h > ../rrdb_server/rrdb.server.h

rm -rf $TMP_DIR

echo "done"
