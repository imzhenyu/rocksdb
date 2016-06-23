/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "rrdb_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>

namespace dsn { namespace apps {


update_request::~update_request() throw() {
}


void update_request::__set_key(const  ::dsn::blob& val) {
  this->key = val;
}

void update_request::__set_value(const  ::dsn::blob& val) {
  this->value = val;
}

uint32_t update_request::read(::apache::thrift::protocol::TProtocol* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
          xfer += this->key.read(iprot);
          this->__isset.key = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
          xfer += this->value.read(iprot);
          this->__isset.value = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t update_request::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("update_request");

  xfer += oprot->writeFieldBegin("key", ::apache::thrift::protocol::T_STRUCT, 1);
  xfer += this->key.write(oprot);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("value", ::apache::thrift::protocol::T_STRUCT, 2);
  xfer += this->value.write(oprot);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(update_request &a, update_request &b) {
  using ::std::swap;
  swap(a.key, b.key);
  swap(a.value, b.value);
  swap(a.__isset, b.__isset);
}

update_request::update_request(const update_request& other0) {
  key = other0.key;
  value = other0.value;
  __isset = other0.__isset;
}
update_request& update_request::operator=(const update_request& other1) {
  key = other1.key;
  value = other1.value;
  __isset = other1.__isset;
  return *this;
}
void update_request::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "update_request(";
  out << "key=" << to_string(key);
  out << ", " << "value=" << to_string(value);
  out << ")";
}


update_response::~update_response() throw() {
}


void update_response::__set_error(const int32_t val) {
  this->error = val;
}

void update_response::__set_app_id(const int32_t val) {
  this->app_id = val;
}

void update_response::__set_partition_index(const int32_t val) {
  this->partition_index = val;
}

void update_response::__set_decree(const int64_t val) {
  this->decree = val;
}

void update_response::__set_server(const std::string& val) {
  this->server = val;
}

uint32_t update_response::read(::apache::thrift::protocol::TProtocol* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->error);
          this->__isset.error = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->app_id);
          this->__isset.app_id = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->partition_index);
          this->__isset.partition_index = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 4:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->decree);
          this->__isset.decree = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 5:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->server);
          this->__isset.server = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t update_response::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("update_response");

  xfer += oprot->writeFieldBegin("error", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32(this->error);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("app_id", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32(this->app_id);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("partition_index", ::apache::thrift::protocol::T_I32, 3);
  xfer += oprot->writeI32(this->partition_index);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("decree", ::apache::thrift::protocol::T_I64, 4);
  xfer += oprot->writeI64(this->decree);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("server", ::apache::thrift::protocol::T_STRING, 5);
  xfer += oprot->writeString(this->server);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(update_response &a, update_response &b) {
  using ::std::swap;
  swap(a.error, b.error);
  swap(a.app_id, b.app_id);
  swap(a.partition_index, b.partition_index);
  swap(a.decree, b.decree);
  swap(a.server, b.server);
  swap(a.__isset, b.__isset);
}

update_response::update_response(const update_response& other2) {
  error = other2.error;
  app_id = other2.app_id;
  partition_index = other2.partition_index;
  decree = other2.decree;
  server = other2.server;
  __isset = other2.__isset;
}
update_response& update_response::operator=(const update_response& other3) {
  error = other3.error;
  app_id = other3.app_id;
  partition_index = other3.partition_index;
  decree = other3.decree;
  server = other3.server;
  __isset = other3.__isset;
  return *this;
}
void update_response::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "update_response(";
  out << "error=" << to_string(error);
  out << ", " << "app_id=" << to_string(app_id);
  out << ", " << "partition_index=" << to_string(partition_index);
  out << ", " << "decree=" << to_string(decree);
  out << ", " << "server=" << to_string(server);
  out << ")";
}


read_response::~read_response() throw() {
}


void read_response::__set_error(const int32_t val) {
  this->error = val;
}

void read_response::__set_value(const  ::dsn::blob& val) {
  this->value = val;
}

void read_response::__set_app_id(const int32_t val) {
  this->app_id = val;
}

void read_response::__set_partition_index(const int32_t val) {
  this->partition_index = val;
}

void read_response::__set_server(const std::string& val) {
  this->server = val;
}

uint32_t read_response::read(::apache::thrift::protocol::TProtocol* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->error);
          this->__isset.error = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
          xfer += this->value.read(iprot);
          this->__isset.value = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->app_id);
          this->__isset.app_id = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 4:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->partition_index);
          this->__isset.partition_index = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 6:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->server);
          this->__isset.server = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t read_response::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("read_response");

  xfer += oprot->writeFieldBegin("error", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32(this->error);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("value", ::apache::thrift::protocol::T_STRUCT, 2);
  xfer += this->value.write(oprot);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("app_id", ::apache::thrift::protocol::T_I32, 3);
  xfer += oprot->writeI32(this->app_id);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("partition_index", ::apache::thrift::protocol::T_I32, 4);
  xfer += oprot->writeI32(this->partition_index);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("server", ::apache::thrift::protocol::T_STRING, 6);
  xfer += oprot->writeString(this->server);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(read_response &a, read_response &b) {
  using ::std::swap;
  swap(a.error, b.error);
  swap(a.value, b.value);
  swap(a.app_id, b.app_id);
  swap(a.partition_index, b.partition_index);
  swap(a.server, b.server);
  swap(a.__isset, b.__isset);
}

read_response::read_response(const read_response& other4) {
  error = other4.error;
  value = other4.value;
  app_id = other4.app_id;
  partition_index = other4.partition_index;
  server = other4.server;
  __isset = other4.__isset;
}
read_response& read_response::operator=(const read_response& other5) {
  error = other5.error;
  value = other5.value;
  app_id = other5.app_id;
  partition_index = other5.partition_index;
  server = other5.server;
  __isset = other5.__isset;
  return *this;
}
void read_response::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "read_response(";
  out << "error=" << to_string(error);
  out << ", " << "value=" << to_string(value);
  out << ", " << "app_id=" << to_string(app_id);
  out << ", " << "partition_index=" << to_string(partition_index);
  out << ", " << "server=" << to_string(server);
  out << ")";
}

}} // namespace
