#include <boost/lexical_cast.hpp>
#include <rrdb/rrdb.client.h>
#include <pegasus/schema.h>
#include "redis_parser.h"

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "redis.parser"

#define CR '\015'
#define LF '\012'

namespace dsn { namespace proxy { namespace redis {

std::unordered_map<std::string, redis_parser::redis_call_handler> redis_parser::s_dispatcher = {
    {"SET", redis_parser::g_set},
    {"GET", redis_parser::g_get},
    {"DEL", redis_parser::g_del}
};

redis_parser::redis_call_handler redis_parser::get_handler(const char *command, unsigned int length)
{
    std::string key(command, length);
    auto iter = s_dispatcher.find(key);
    if (iter == s_dispatcher.end())
        return redis_parser::g_default_handler;
    return iter->second;
}

redis_parser::redis_parser(proxy_stub *op, rpc_address remote):
    proxy_session(op, remote),
    next_seqid(0),
    current_msg(new message_entry()),
    status(start_array),
    current_size(),
    total_length(0),
    current_buffer(nullptr),
    current_buffer_length(0),
    current_cursor(0)
{
    apps::rrdb_client* r;
    if (op)
        r = new dsn::apps::rrdb_client(op->get_service_uri());
    else
        r = new dsn::apps::rrdb_client();
    client.reset(r);
}

redis_parser::~redis_parser()
{
    dinfo("redis parser destroyed");
}

void redis_parser::prepare_current_buffer()
{
    void* msg_buffer;
    if (current_buffer==nullptr)
    {
        dsn_message_t first_msg = recv_buffers.front();
        dassert(dsn_msg_read_next(first_msg, &msg_buffer, &current_buffer_length), "");
        current_buffer = reinterpret_cast<char*>(msg_buffer);
        current_cursor = 0;
    }
    else if (current_cursor >= current_buffer_length)
    {
        dsn_message_t first_msg = recv_buffers.front();
        dsn_msg_read_commit(first_msg, current_buffer_length);
        if (dsn_msg_read_next(first_msg, &msg_buffer, &current_buffer_length))
        {
            current_cursor = 0;
            current_buffer = reinterpret_cast<char*>(msg_buffer);
            return;
        }
        else
        {
            //we have consume this message all over
            dsn_msg_release_ref(first_msg); //added when messaged is received in proxy_stub
            recv_buffers.pop();
            current_buffer = nullptr;
            prepare_current_buffer();
        }
    }
    else
        return;
}

void redis_parser::reset()
{
    //clear the response pipeline
    while (!pending_response.empty())
    {
        message_entry* entry = pending_response.front().get();
        if (entry->response)
            dsn_msg_release_ref(entry->response);

        pending_response.pop_front();
    }
    next_seqid = 0;

    //clear the parser status
    current_msg->request.length = 0;
    current_msg->request.buffers.clear();
    status = start_array;
    current_size.clear();

    //clear the data stream
    total_length = 0;
    if (current_buffer)
    {
        dsn_msg_read_commit(recv_buffers.front(), current_buffer_length);
    }
    current_buffer = nullptr;
    current_buffer_length = 0;
    current_cursor = 0;
    while (!recv_buffers.empty())
    {
        dsn_msg_release_ref(recv_buffers.front());
        recv_buffers.pop();
    }
}

char redis_parser::peek()
{
    prepare_current_buffer();
    return current_buffer[current_cursor];
}

void redis_parser::eat(char c)
{
    prepare_current_buffer();
    if (current_buffer[current_cursor] == c)
    {
        ++current_cursor;
        --total_length;
    }
    else
    {
        derror("expect token: %c, got %c", c, current_buffer[current_cursor]);
        throw std::invalid_argument("");
    }
}

void redis_parser::eat_all(char* dest, size_t length)
{
    total_length -= length;
    while (length > 0)
    {
        prepare_current_buffer();

        size_t eat_size = current_buffer_length - current_cursor;
        if (eat_size > length)
            eat_size = length;
        memcpy(dest, current_buffer+current_cursor, eat_size);
        dest += eat_size;
        current_cursor += eat_size;
        length -= eat_size;
    }
}

void redis_parser::end_array_size()
{
    redis_request& current_array = current_msg->request;
    try
    {
        current_array.length = boost::lexical_cast<int>(current_size);
        current_size.clear();
        if (current_array.length <= 0)
        {
            derror("array size should be positive in redis request, but %d", current_array.length);
            throw std::invalid_argument("");
        }
        current_array.buffers.reserve(current_array.length);
        status = start_bulk_string;
    }
    catch (const boost::bad_lexical_cast& c)
    {
        derror("invalid size string \"%s\"", current_size.c_str());
        throw std::invalid_argument("");
    }
}

void redis_parser::append_current_bulk_string()
{
    redis_request& current_array = current_msg->request;
    current_array.buffers.push_back(current_str);
    if (current_array.buffers.size() == current_array.length)
    {
        //we get a full request command
        handle_command(std::move(current_msg));
        current_msg.reset(new message_entry());
        status = start_array;
    }
    else
    {
        status = start_bulk_string;
    }
}

void redis_parser::end_bulk_string_size()
{
    try
    {
        current_str.length = boost::lexical_cast<int>(current_size);
        current_size.clear();
        if (-1 == current_str.length)
        {
            append_current_bulk_string();
        }
        else if (current_str.length >= 0)
        {
            status = start_bulk_string_data;
        }
        else
        {
            derror("invalid bulk string length: %d", current_str.length);
            throw std::invalid_argument("");
        }
    }
    catch (const boost::bad_lexical_cast& c)
    {
        derror("invalid size string \"%s\"", current_size.c_str());
        throw std::invalid_argument("");
    }
}

void redis_parser::append_message(dsn_message_t msg)
{
    recv_buffers.push(msg);
    total_length += dsn_msg_body_size(msg);
    dinfo("recv message, currently total length is %d", total_length);
}

//refererence: http://redis.io/topics/protocol
void redis_parser::parse_stream()
{
    char t;
    while (total_length > 0)
    {
        switch (status)
        {
        case start_array:
            eat('*');
            status = in_array_size;
            break;
        case in_array_size:
        case in_bulk_string_size:
            t = peek();
            if (t == CR)
            {
                if (total_length > 1)
                {
                    eat(CR);
                    eat(LF);
                    if (in_array_size == status)
                        end_array_size();
                    else
                        end_bulk_string_size();
                }
                else
                    return;
            }
            else {
                current_size.push_back(t);
                eat(t);
            }
            break;
        case start_bulk_string:
            eat('$');
            status = in_bulk_string_size;
            break;
        case start_bulk_string_data:
            //string content + CR + LF
            if (total_length >= current_str.length + 2)
            {
                if (current_str.length > 0)
                {
                    char* ptr = reinterpret_cast<char*>(dsn_transient_malloc(current_str.length));
                    std::shared_ptr<char> str_data(ptr, [](char* ptr){dsn_transient_free(ptr);});
                    eat_all(str_data.get(), current_str.length);
                    current_str.data.assign(std::move(str_data), 0, current_str.length);
                }
                eat(CR);
                eat(LF);
                append_current_bulk_string();
            }
            else
                return;
            break;
        default:
            break;
        }
    }
}

bool redis_parser::parse(dsn_message_t msg)
{
    append_message(msg);
    try {
        parse_stream();
        return true;
    } catch (...) {
        reset();
        return false;
    }
}

void redis_parser::on_remove_session(std::shared_ptr<proxy_session> _this)
{
    check_hashed_access();
    reset();
    status = removed;
}

void redis_parser::reply_all_ready()
{
    while (!pending_response.empty())
    {
        message_entry* entry = pending_response.front().get();
        if (!entry->response)
            return;
        dsn_rpc_reply(entry->response, dsn::ERR_OK);
        //added when message is created
        dsn_msg_release_ref(entry->response);
        pending_response.pop_front();
    }
}

void redis_parser::default_handler(redis_parser::message_entry& entry)
{
    redis_simple_string result;
    result.is_error = true;
    result.message = "ERR not supported";
    reply_message(entry, result);
}

void redis_parser::set(redis_parser::message_entry& entry)
{
    redis_request& request = entry.request;
    if (request.buffers.size() < 3)
    {
        redis_simple_string result;
        result.is_error = true;
        result.message = "ERR invalid param count";
        reply_message(entry, result);
    }
    else
    {
        //with a reference to prevent the object from being destoryed
        std::shared_ptr<proxy_session> ref_this = shared_from_this();
        auto on_set_reply = [ref_this, this, &entry](dsn::error_code ec, dsn_message_t, dsn_message_t response)
        {
            check_hashed_access();
            if (status == removed) return;

            if (dsn::ERR_OK != ec)
            {
                redis_simple_string result;
                result.is_error = true;
                result.message = std::string("ERR ") + ec.to_string();
                reply_message(entry, result);
            }
            else
            {
                dsn::apps::update_response rrdb_response;
                dsn::unmarshall(response, rrdb_response);
                if (rrdb_response.error != 0)
                {
                    redis_simple_string result;
                    result.is_error = true;
                    result.message = "ERR internal error " + boost::lexical_cast<std::string>(rrdb_response.error);
                    reply_message(entry, result);
                }
                else
                {
                    redis_simple_string result;
                    result.is_error = false;
                    result.message = "OK";
                    reply_message(entry, result);
                }
            }
        };

        dsn::apps::update_request req;
        dsn::blob null_blob;
        pegasus::rrdb_generate_key(req.key, request.buffers[1].data, null_blob);
        req.value = request.buffers[2].data;
        auto partition_hash = pegasus::rrdb_key_hash(req.key);
        //TODO: set the timeout
        client->put(req, on_set_reply, std::chrono::milliseconds(2000), 0, partition_hash, proxy_session::hash());
    }
}

void redis_parser::get(message_entry &entry)
{
    redis_request& redis_req = entry.request;
    if (redis_req.buffers.size() != 2)
    {
        redis_simple_string result;
        result.is_error = true;
        result.message = "ERR invalid param count";
        reply_message(entry, result);
    }
    else
    {
        std::shared_ptr<proxy_session> ref_this = shared_from_this();
        auto on_get_reply = [ref_this, this, &entry](dsn::error_code ec, dsn_message_t, dsn_message_t response)
        {
            check_hashed_access();
            if (removed == status) return;

            if (dsn::ERR_OK != ec)
            {
                redis_simple_string result;
                result.is_error = true;
                result.message = std::string("ERR ") + ec.to_string();
                reply_message(entry, result);
            }
            else
            {
                dsn::apps::read_response rrdb_response;
                dsn::unmarshall(response, rrdb_response);
                if (rrdb_response.error != 0)
                {
                    if (rrdb_response.error == 1)
                    {
                        redis_bulk_string result;
                        result.length = -1;
                        reply_message(entry, result);
                    }
                    else
                    {
                        redis_simple_string result;
                        result.is_error = true;
                        result.message = "ERR internal error " + boost::lexical_cast<std::string>(rrdb_response.error);
                        reply_message(entry, result);
                    }
                }
                else
                {
                    redis_bulk_string result(rrdb_response.value);
                    reply_message(entry, result);
                }
            }
        };
        dsn::blob req;
        dsn::blob null_blob;
        pegasus::rrdb_generate_key(req, redis_req.buffers[1].data, null_blob);
        auto partition_hash = pegasus::rrdb_key_hash(req);
        //TODO: set the timeout
        client->get(req, on_get_reply, std::chrono::milliseconds(2000), 0, partition_hash, proxy_session::hash());
    }
}

void redis_parser::del(message_entry &entry)
{
    redis_request& redis_req = entry.request;
    if (redis_req.buffers.size() != 2)
    {
        redis_simple_string result;
        result.is_error = true;
        result.message = "ERR invalid command";
        reply_message(entry, result);
    }
    else
    {
        std::shared_ptr<proxy_session> ref_this = shared_from_this();
        auto on_del_reply = [ref_this, this, &entry](dsn::error_code ec, dsn_message_t, dsn_message_t response)
        {
            check_hashed_access();
            if (removed == status) return;

            if (dsn::ERR_OK != ec)
            {
                redis_simple_string result;
                result.is_error = true;
                result.message = std::string("ERR ") + ec.to_string();
                reply_message(entry, result);
            }
            else
            {
                dsn::apps::read_response rrdb_response;
                dsn::unmarshall(response, rrdb_response);
                if (rrdb_response.error != 0)
                {
                    redis_simple_string result;
                    result.is_error = true;
                    result.message = "ERR internal error " + boost::lexical_cast<std::string>(rrdb_response.error);
                    reply_message(entry, result);
                }
                else
                {
                    redis_integer result;
                    result.value = 1;
                    reply_message(entry, result);
                }
            }
        };
        dsn::blob req;
        dsn::blob null_blob;
        pegasus::rrdb_generate_key(req, redis_req.buffers[1].data, null_blob);
        auto partition_hash = pegasus::rrdb_key_hash(req);
        //TODO: set the timeout
        client->remove(req, on_del_reply, std::chrono::milliseconds(2000), 0, partition_hash, proxy_session::hash());
    }
}

void redis_parser::handle_command(std::unique_ptr<message_entry>&& entry)
{
    message_entry& e = *entry.get();
    redis_request& request = e.request;
    e.sequence_id = ++next_seqid;
    e.response = nullptr;
    pending_response.emplace_back(std::move(entry));

    dassert(request.length>0, "");
    dsn::blob& command = request.buffers[0].data;
    redis_call_handler handler = redis_parser::get_handler(command.data(), command.length());
    handler(this, e);
}

void redis_parser::marshalling(dsn::binary_writer& write_stream, const redis_bulk_string &bs)
{
    write_stream.write_pod('$');
    std::string result = boost::lexical_cast<std::string>(bs.length);
    write_stream.write(result.c_str(), result.length());
    write_stream.write_pod(CR);
    write_stream.write_pod(LF);
    if (bs.length < 0)
        return;
    if (bs.length > 0)
    {
        dassert(bs.data.length()==bs.length, "");
        write_stream.write(bs.data.data(), bs.length);
    }
    write_stream.write_pod(CR);
    write_stream.write_pod(LF);
}

void redis_parser::marshalling(dsn::binary_writer& write_stream, const redis_simple_string &data)
{
    write_stream.write_pod(data.is_error?'-':'+');
    write_stream.write(data.message.c_str(), data.message.length());
    write_stream.write_pod(CR);
    write_stream.write_pod(LF);
}

void redis_parser::marshalling(dsn::binary_writer& write_stream, const redis_integer& data)
{
    write_stream.write_pod(':');
    std::string result = boost::lexical_cast<std::string>(data.value);
    write_stream.write(result.c_str(), result.length());
    write_stream.write_pod(CR);
    write_stream.write_pod(LF);
}

}}}
