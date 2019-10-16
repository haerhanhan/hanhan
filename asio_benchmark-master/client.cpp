#define ASIO_STANDALONE
#define ASIO_HAS_STD_CHRONO

#include "asio.hpp"
#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <mutex>
#include <assert.h>
#include <vector>
#include <memory>
#include <queue>
#include <stack>
#include <thread>
#include "base.h"

#include <ctime>
#include "crc.h"

using std::string;
using namespace _crc_;

namespace {

class stats
{
public:
    stats(int timeout) : timeout_(timeout)
    {
    }

    void add(bool error, size_t count_written, size_t count_read,
        size_t bytes_written, size_t bytes_read)
    {
        total_error_count_ += error ? 1 : 0;
        total_count_written_ += count_written;
        total_count_read_ += count_read;
        total_bytes_written_ += bytes_written;
        total_bytes_read_ += bytes_read;
    }

    void print()
    {
        std::cout << total_error_count_ << " total count error\n";
        std::cout << total_count_written_ << " total count written\n";
        std::cout << total_count_read_ << " total count read\n";
        std::cout << total_bytes_written_ << " total bytes written\n";
        std::cout << total_bytes_read_ << " total bytes read\n";
        std::cout << static_cast<double>(total_bytes_read_) /
            (timeout_ * 1024 * 1024) << " MiB/s read throughput\n";
        std::cout << static_cast<double>(total_bytes_written_) /
            (timeout_ * 1024 * 1024) << " MiB/s write throughput\n";
    }

private:
    size_t total_error_count_ = 0;
    size_t total_bytes_written_ = 0;
    size_t total_bytes_read_ = 0;
    size_t total_count_written_ = 0;
    size_t total_count_read_ = 0;
    int timeout_;
};
    
    char *rand_string(char *string, size_t len)
    {
        //const int SIZE_CHAR = 32; //生成32 + 1位C Style字符串
        const char CCH[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

        //srand((unsigned)time(NULL));
        //char ch[SIZE_CHAR + 1] = {0};
        size_t x = 0;
        for (size_t i = 0; i < len; ++i)
        {
            //int x = rand() / (RAND_MAX / (sizeof(CCH) - 1));    
            x = i%(sizeof(CCH)-1) ;  
            string[i] = CCH[x];
        } 

        return string;

    }


const size_t kMaxStackSize = 10;
class session : noncopyable
{
public:
    session(asio::io_service& ios, size_t block_size)
        : io_service_(ios)
        , socket_(ios)//每个session对应一个socket,每一个ios存在多个socket
        , block_size_(block_size)
    {

        for (size_t i = 0; i < kMaxStackSize; ++i) {
            buffers_.push(new char[block_size_]);//将数组作为一个单位压�，定义的时候就是数组指针
        }
    }

    ~session()
    {
        while (!buffers_.empty()) {
            auto buffer = buffers_.top();
            delete[] buffer;
            buffers_.pop();
        }

        free_table();
    }

    void write(char *buffer, size_t len) {

        socket_.async_write_some(asio::buffer(buffer, len),
            [this, buffer](const asio::error_code& err, size_t cb) {
            bool need_read = buffers_.empty();
            buffers_.push(buffer);
            if (!err) {
                bytes_written_ += cb;
                ++count_written_;
                if (need_read) {
                    read();
                }
            } else {
                if (!want_close_) {
                    //std::cout << "write failed: " << err.message() << "\n";
                    error_ = true;
                }
            }
        });
    }

    void read() {
        if (buffers_.empty()) {
            std::cout << "the cached buffer used out\n";
            return;
        }

        auto buffer = buffers_.top();
       //char  buffer[block_size_];
        buffers_.pop();

        socket_.async_read_some(asio::buffer(buffer,block_size_),
            [this, buffer](const asio::error_code& err, size_t cb) {
            if (!err && verify_crc8(buffer,cb)) {
                bytes_read_ += cb;
                ++count_read_;
              //  crc8_continue(0,buffer,cb);
              //  std::cout<< std::hex <<(unsigned int )(unsigned char)buffer[cb-1]<<std::endl;
                write(buffer, cb);
                read();
         
            } else {
                if (!want_close_) {
                    //std::cout << "read failed: " << err.message(s) << "\n";
                    error_ = true;
                }
            }
        });
    }

    void start(asio::ip::tcp::endpoint endpoint) 
    {

        socket_.async_connect(endpoint, [this](const asio::error_code& err) {
            if (!err)
            {
                asio::ip::tcp::no_delay no_delay(true);
                socket_.set_option(no_delay);
                //std::cout<< buffer << ":"<<sizeof(buffer)<< ":"<< cb <<std::endl;
                //read();
                // write first packet
                auto buffer = buffers_.top();
                buffers_.pop();

                rand_string(buffer, block_size_-1);
                add_crc8(buffer, block_size_-1);

                write(buffer, block_size_);
                read();
            }
        });
    }

    void stop()
    {
        io_service_.post([this]() {
            want_close_ = true;
            socket_.close();
        });
    }

    size_t bytes_written() const {
        return bytes_written_;
    }

    size_t bytes_read() const {
        return bytes_read_;
    }

    size_t count_written() const {
        return count_written_;
    }

    size_t count_read() const {
        return count_read_;
    }

    bool error() const {
        return error_;
    }
private:
    asio::io_service& io_service_;
    asio::ip::tcp::socket socket_;
    size_t const block_size_;
    size_t bytes_written_ = 0;
    size_t bytes_read_ = 0;
    size_t count_written_ = 0;
    size_t count_read_ = 0;
    std::stack<char*> buffers_;//数组的指针建立
    bool error_ = false;
    bool want_close_ = false;
    char *buffer;

};

class client : noncopyable
{
public:
    client(int thread_count,
        char const* host, char const* port,
        size_t block_size, size_t session_count, int timeout)
        : thread_count_(thread_count)
        , timeout_(timeout)
        , stats_(timeout)
    {
        io_services_.resize(thread_count_);
        io_works_.resize(thread_count_);
        threads_.resize(thread_count_);

        for (auto i = 0; i < thread_count_; ++i) {
            io_services_[i].reset(new asio::io_service);
            io_works_[i].reset(new asio::io_service::work(*io_services_[i]));
            threads_[i].reset(new std::thread([this, i]() {
                auto& io_service = *io_services_[i];

                try {
                    io_service.run();
                } catch (std::exception& e) {
                    std::cerr << "Exception: " << e.what() << "\n";
                }
            }));
        }

        stop_timer_.reset(new asio::steady_timer(*io_services_[0]));

        resolver_.reset(new asio::ip::tcp::resolver(*io_services_[0]));
        asio::ip::tcp::resolver::iterator iter =
            resolver_->resolve(asio::ip::tcp::resolver::query(host, port));
        endpoint_ = *iter;

        for (size_t i = 0; i < session_count; ++i)
        {
            auto& io_service = *io_services_[i % thread_count_];
            std::unique_ptr<session> new_session(new session(io_service,
                block_size));//一个io_service就能够对应处理N个session的操作
            sessions_.emplace_back(std::move(new_session));
        }
    }

    ~client()
    {
        for (auto& session : sessions_) {//要计算最后的字节数，所有的session都要保存起来统一处理
            stats_.add(
                session->error(),
                session->count_written(), session->count_read(),
                session->bytes_written(), session->bytes_read());
        }

        stats_.print();
    }

    void start() {
        stop_timer_->expires_from_now(std::chrono::seconds(timeout_));//定时timeout，结束就开始执行async_wait()包含的程序，程序异步执行
        stop_timer_->async_wait([this](const asio::error_code& error) {

            for (auto& io_work : io_works_) {
                io_work.reset();//守护io_service，即使所有io任务都执行完成，也不会退出，继续等待新的io任务
            }

            for (auto& session : sessions_) {
                session->stop();//timeout 之后停止所有session，由下面的start重新进行运行timeout长度的时间
            }
        });


        for (auto& session : sessions_) {
            session->start(endpoint_);
        }
    }

    void wait() {
        for (auto& thread : threads_) {
            thread->join();//每个线程其实就是提供一个io_service，全部运行起来，session从一个io_service处理还是多个io_service处理
        }
    }

private:
    int const thread_count_;
    int const timeout_;
    std::vector<std::unique_ptr<asio::io_service>> io_services_;
    std::vector<std::unique_ptr<asio::io_service::work>> io_works_;
    std::unique_ptr<asio::ip::tcp::resolver> resolver_;
    std::vector<std::unique_ptr<std::thread>> threads_;
    std::unique_ptr<asio::steady_timer> stop_timer_;
    std::vector<std::unique_ptr<session>> sessions_;
    stats stats_;
    asio::ip::tcp::endpoint endpoint_;

};

} // namespace


void client_test(int thread_count, char const* host, char const* port,
    size_t block_size, size_t session_count, int timeout) {
    try {

        client c(thread_count, host, port, block_size, session_count, timeout);
        c.start();
        c.wait();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
