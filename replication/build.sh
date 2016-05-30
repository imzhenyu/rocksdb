#!/bin/bash
#
# Shell Options:
#    CLEAR          YES|NO
#    PART_CLEAR     YES|NO
#    BUILD_TYPE     debug|release
#    SERIALIZE_TYPE dsn|thrift|protobuf
#    RUN_VERBOSE    YES|NO
#    WARNING_ALL    YES|NO
#    ENABLE_GCOV    YES|NO
#    BOOST_DIR      <dir>|""
#    TEST_MODULE    "<module1> <module2> ..."
#
# CMake options:
#    -DCMAKE_C_COMPILER=gcc
#    -DCMAKE_CXX_COMPILER=g++
#    [-DCMAKE_BUILD_TYPE=Debug]
#    [-DWARNING_ALL=TRUE]
#    [-DENABLE_GCOV=TRUE]
#    [-DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT=$BOOST_DIR -DBoost_NO_SYSTEM_PATHS=ON]

ROOT=`pwd`
BUILD_DIR="$ROOT/builder"
CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++"
MAKE_OPTIONS="$MAKE_OPTIONS -j8"

if [ "$CLEAR" == "YES" ]
then
    echo "CLEAR=YES"
else
    echo "CLEAR=NO"
fi

if [ "$PART_CLEAR" == "YES" ]
then
    echo "PART_CLEAR=YES"
else
    echo "PART_CLEAR=NO"
fi

if [ "$BUILD_TYPE" == "debug" ]
then
    echo "BUILD_TYPE=debug"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_BUILD_TYPE=Debug"
else
    echo "BUILD_TYPE=release"
fi

echo "SERIALIZE_TYPE=$SERIALIZE_TYPE"
if [ -n "$SERIALIZE_TYPE" ]
then
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DDSN_SERIALIZATION_TYPE=$SERIALIZE_TYPE"
fi

if [ "$RUN_VERBOSE" == "YES" ]
then
    echo "RUN_VERBOSE=YES"
    MAKE_OPTIONS="$MAKE_OPTIONS VERBOSE=1"
else
    echo "RUN_VERBOSE=NO"
fi

if [ "$WARNING_ALL" == "YES" ]
then
    echo "WARNING_ALL=YES"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DWARNING_ALL=TRUE"
else
    echo "WARNING_ALL=NO"
fi

if [ "$ENABLE_GCOV" == "YES" ]
then
    echo "ENABLE_GCOV=YES"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DENABLE_GCOV=TRUE"
else
    echo "ENABLE_GCOV=NO"
fi

# You can specify customized boost by defining BOOST_DIR.
# Install boost like this:
#   wget http://downloads.sourceforge.net/project/boost/boost/1.54.0/boost_1_54_0.zip?r=&ts=1442891144&use_mirror=jaist
#   unzip -q boost_1_54_0.zip
#   cd boost_1_54_0
#   ./bootstrap.sh --with-libraries=system,filesystem --with-toolset=gcc
#   ./b2 toolset=gcc cxxflags="-std=c++11 -fPIC" -j8 -d0
#   ./b2 install --prefix=$DSN_ROOT -d0
# And set BOOST_DIR as:
#   export BOOST_DIR=/path/to/boost_1_54_0/output
if [ -n "$BOOST_DIR" ]
then
    echo "Use customized boost: $BOOST_DIR"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT=$BOOST_DIR -DBoost_NO_SYSTEM_PATHS=ON"
    export EXTRA_LDFLAGS="$EXTRA_LDFLAGS -Wl,-rpath $BOOST_DIR/lib"
else
    echo "Use system boost"
fi

echo "CMAKE_OPTIONS=$CMAKE_OPTIONS"
echo "MAKE_OPTIONS=$MAKE_OPTIONS"

echo "#############################################################################"

if [ -f $BUILD_DIR/CMAKE_OPTIONS ]
then
    LAST_OPTIONS=`cat $BUILD_DIR/CMAKE_OPTIONS`
    if [ "$CMAKE_OPTIONS" != "$LAST_OPTIONS" ]
    then
        echo "WARNING: CMAKE_OPTIONS has changed from last build, clear environment first"
        CLEAR=YES
    fi
fi

if [ "$CLEAR" == "YES" ]
then
    echo "Clear librocksdb.a ..."
    rm -rf $BUILD_DIR
    make -C .. clean
fi

if [ "$CLEAR" == "YES" -o "$PART_CLEAR" == "YES" ]
then
    echo "Clear $BUILD_DIR ..."
    rm -rf $BUILD_DIR
    rm -f ../rrdb_bench
fi

make -C .. static_lib_$BUILD_TYPE $MAKE_OPTIONS
if [ $? -ne 0 ]
then
    echo "ERROR: build librocksdb.a failed"
    exit -1
else
    echo "Build librocksdb.a succeed"
fi

if [ ! -d "$BUILD_DIR" ]
then
    echo "Running cmake..."
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    echo "$CMAKE_OPTIONS" >CMAKE_OPTIONS
    cmake .. -DCMAKE_INSTALL_PREFIX=$BUILD_DIR/output $CMAKE_OPTIONS
    if [ $? -ne 0 ]
    then
        echo "ERROR: cmake failed"
        exit -1
    fi
    cd ..
fi

cd $BUILD_DIR
echo "Building..."
make install $MAKE_OPTIONS
if [ $? -ne 0 ]
then
    echo "ERROR: build rrdb failed"
    exit -1
else
    echo "Build rrdb succeed"
fi
cd ..

make -C .. rrdb_bench_$BUILD_TYPE $MAKE_OPTIONS
if [ $? -ne 0 ]
then
    echo "ERROR: build rrdb_bench failed"
    exit -1
else
    echo "Build rrdb_bench succeed"
fi

