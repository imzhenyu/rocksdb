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

class update_response;

class read_response;

typedef struct _update_request__isset {
  _update_request__isset() : key(false), value(false) {}
  bool key :1;
  bool value :1;
} _update_request__isset;

class update_request {
 public:

  update_request(const update_request&);
  update_request& operator=(const update_request&);
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

typedef struct _update_response__isset {
  _update_response__isset() : error(false), app_id(false), pidx(false), ballot(false), decree(false), seqno(false), server(false) {}
  bool error :1;
  bool app_id :1;
  bool pidx :1;
  bool ballot :1;
  bool decree :1;
  bool seqno :1;
  bool server :1;
} _update_response__isset;

class update_response {
 public:

  update_response(const update_response&);
  update_response& operator=(const update_response&);
  update_response() : error(0), app_id(0), pidx(0), ballot(0), decree(0), seqno(0), server() {
  }

  virtual ~update_response() throw();
  int32_t error;
  int32_t app_id;
  int32_t pidx;
  int64_t ballot;
  int64_t decree;
  int64_t seqno;
  std::string server;

  _update_response__isset __isset;

  void __set_error(const int32_t val);

  void __set_app_id(const int32_t val);

  void __set_pidx(const int32_t val);

  void __set_ballot(const int64_t val);

  void __set_decree(const int64_t val);

  void __set_seqno(const int64_t val);

  void __set_server(const std::string& val);

  bool operator == (const update_response & rhs) const
  {
    if (!(error == rhs.error))
      return false;
    if (!(app_id == rhs.app_id))
      return false;
    if (!(pidx == rhs.pidx))
      return false;
    if (!(ballot == rhs.ballot))
      return false;
    if (!(decree == rhs.decree))
      return false;
    if (!(seqno == rhs.seqno))
      return false;
    if (!(server == rhs.server))
      return false;
    return true;
  }
  bool operator != (const update_response &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const update_response & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(update_response &a, update_response &b);

inline std::ostream& operator<<(std::ostream& out, const update_response& obj)
{
  obj.printTo(out);
  return out;
}

typedef struct _read_response__isset {
  _read_response__isset() : error(false), value(false), app_id(false), pidx(false), ballot(false), server(false) {}
  bool error :1;
  bool value :1;
  bool app_id :1;
  bool pidx :1;
  bool ballot :1;
  bool server :1;
} _read_response__isset;

class read_response {
 public:

  read_response(const read_response&);
  read_response& operator=(const read_response&);
  read_response() : error(0), app_id(0), pidx(0), ballot(0), server() {
  }

  virtual ~read_response() throw();
  int32_t error;
   ::dsn::blob value;
  int32_t app_id;
  int32_t pidx;
  int64_t ballot;
  std::string server;

  _read_response__isset __isset;

  void __set_error(const int32_t val);

  void __set_value(const  ::dsn::blob& val);

  void __set_app_id(const int32_t val);

  void __set_pidx(const int32_t val);

  void __set_ballot(const int64_t val);

  void __set_server(const std::string& val);

  bool operator == (const read_response & rhs) const
  {
    if (!(error == rhs.error))
      return false;
    if (!(value == rhs.value))
      return false;
    if (!(app_id == rhs.app_id))
      return false;
    if (!(pidx == rhs.pidx))
      return false;
    if (!(ballot == rhs.ballot))
      return false;
    if (!(server == rhs.server))
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
