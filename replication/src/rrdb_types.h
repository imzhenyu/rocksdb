/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef rrdb_TYPES_H
#define rrdb_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/cxxfunctional.h>
#include "dsn_types.h"


namespace dsn { namespace apps {

class update_request;

class read_response;

typedef struct _update_request__isset {
  _update_request__isset() : key(false), value(false) {}
  bool key :1;
  bool value :1;
} _update_request__isset;

class update_request {
 public:

  update_request(const update_request&);
  update_request(update_request&&);
  update_request& operator=(const update_request&);
  update_request& operator=(update_request&&);
  update_request() {
  }

  virtual ~update_request() throw();
   ::dsn::blob key;
   ::dsn::blob value;

  _update_request__isset __isset;

  void __set_key(const  ::dsn::blob& val);

  void __set_value(const  ::dsn::blob& val);

  bool operator == (const update_request & rhs) const
  {
    if (!(key == rhs.key))
      return false;
    if (!(value == rhs.value))
      return false;
    return true;
  }
  bool operator != (const update_request &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const update_request & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(update_request &a, update_request &b);

inline std::ostream& operator<<(std::ostream& out, const update_request& obj)
{
  obj.printTo(out);
  return out;
}

typedef struct _read_response__isset {
  _read_response__isset() : error(false), value(false) {}
  bool error :1;
  bool value :1;
} _read_response__isset;

class read_response {
 public:

  read_response(const read_response&);
  read_response(read_response&&);
  read_response& operator=(const read_response&);
  read_response& operator=(read_response&&);
  read_response() : error(0), value() {
  }

  virtual ~read_response() throw();
  int32_t error;
  std::string value;

  _read_response__isset __isset;

  void __set_error(const int32_t val);

  void __set_value(const std::string& val);

  bool operator == (const read_response & rhs) const
  {
    if (!(error == rhs.error))
      return false;
    if (!(value == rhs.value))
      return false;
    return true;
  }
  bool operator != (const read_response &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const read_response & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(read_response &a, read_response &b);

inline std::ostream& operator<<(std::ostream& out, const read_response& obj)
{
  obj.printTo(out);
  return out;
}

}} // namespace

#endif
