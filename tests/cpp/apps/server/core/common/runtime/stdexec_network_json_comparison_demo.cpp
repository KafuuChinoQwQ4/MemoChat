// Reading demo only. This file is intentionally NOT listed in CMakeLists.txt.
//
// Goal: compare one small stateful async protocol in two styles:
//   1. stdexec/asioexec coroutine: retry, loop, branch, and local state stay in one function.
//   2. traditional Boost.Asio callback: the same flow is split across step functions and member state.

#include <exec/asio/asio_config.hpp>
#include <exec/asio/use_sender.hpp>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <stdexec/execution.hpp>

#include "runtime/AsioCoScheduler.h"

#include <boost/asio.hpp>

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace demo
{
constexpr std::string_view kAuth = "AUTH";
constexpr std::string_view kBad = "BAD!";
constexpr std::string_view kWork = "WORK";
constexpr std::string_view kStop = "STOP";

constexpr std::string_view kOk = "OK!!";
constexpr std::string_view kFail = "FAIL";
constexpr std::string_view kAck = "ACK!";
constexpr std::string_view kBye = "BYE!";

constexpr int mauth = 2;
constexpr int kmc = 2;

std::string_view FrameView(const std::array<char, 4>& frame)
{
    return std::string_view{frame.data(), frame.size()};
}

void SendFrame(boost::asio::ip::tcp::socket& socket, std::string_view frame)
{
    boost::asio::write(socket, boost::asio::buffer(frame.data(), frame.size()));
}

std::string ReceiveFrame(boost::asio::ip::tcp::socket& socket)
{
    std::array<char, 4> frame{};
    boost::asio::read(socket, boost::asio::buffer(frame), boost::asio::transfer_exactly(frame.size()));
    return std::string{frame.data(), frame.size()};
}

void DriveClient(boost::asio::ip::tcp::socket& socket, std::string_view label)
{
    SendFrame(socket, kBad);
    std::cout << label << " auth 1: " << ReceiveFrame(socket) << '\n';
    SendFrame(socket, kAuth);
    std::cout << label << " auth 2: " << ReceiveFrame(socket) << '\n';
    SendFrame(socket, kWork);
    std::cout << label << " work: " << ReceiveFrame(socket) << '\n';
    SendFrame(socket, kStop);
    std::cout << label << " stop: " << ReceiveFrame(socket) << '\n';
}

exec::task<void> CoroutineSession(boost::asio::ip::tcp::acceptor& acceptor)
{
    boost::asio::ip::tcp::socket socket(acceptor.get_executor());
    co_await acceptor.async_accept(socket, exec::asio::use_sender);
    bool atd = false;
    for (int att = 0; att < mauth; ++att)
    {
        std::array<char, 4> token{};
        co_await boost::asio::async_read(socket,
                                         boost::asio::buffer(token),
                                         boost::asio::transfer_exactly(token.size()),
                                         exec::asio::use_sender);
        if (FrameView(token) == kAuth)
        {
            co_await boost::asio::async_write(socket,
                                              boost::asio::buffer(kOk.data(), kOk.size()),
                                              exec::asio::use_sender);
            atd = true;
            break;
        }
        co_await boost::asio::async_write(socket,
                                          boost::asio::buffer(kFail.data(), kFail.size()),
                                          exec::asio::use_sender);
    }
    if (!atd)
        co_return;
    for (int cct = 0; cct < kmc; ++cct)
    {
        std::array<char, 4> command{};
        co_await boost::asio::async_read(socket,
                                         boost::asio::buffer(command),
                                         boost::asio::transfer_exactly(command.size()),
                                         exec::asio::use_sender);
        if (FrameView(command) == kWork)
        {
            co_await boost::asio::async_write(socket,
                                              boost::asio::buffer(kAck.data(), kAck.size()),
                                              exec::asio::use_sender);
        }
        else if (FrameView(command) == kStop)
        {
            co_await boost::asio::async_write(socket,
                                              boost::asio::buffer(kBye.data(), kBye.size()),
                                              exec::asio::use_sender);
            co_return;
        }
        else
        {
            co_await boost::asio::async_write(socket,
                                              boost::asio::buffer(kFail.data(), kFail.size()),
                                              exec::asio::use_sender);
        }
    }
}

void RunCoroutineSession()
{
    boost::asio::io_context io;
    auto work = boost::asio::make_work_guard(io);
    std::thread io_thread(
        [&io]
        {
            io.run();
        });

    boost::asio::ip::tcp::acceptor acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
    boost::asio::ip::tcp::socket client_socket(io);

    memochat::runtime::IoContextScheduler scheduler{&io};
    exec::async_scope scope;
    scope.spawn(stdexec::on(scheduler, CoroutineSession(acceptor)));

    client_socket.connect(acceptor.local_endpoint());
    DriveClient(client_socket, "coroutine");

    client_socket.close();
    stdexec::sync_wait(scope.on_empty());

    work.reset();
    io_thread.join();
}

// ============================================================================

class CallbackSession final : public std::enable_shared_from_this<CallbackSession>
{
public:
    explicit CallbackSession(boost::asio::io_context& io)
        : acceptor_(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0))
        , socket_(io)
    {
    }

    boost::asio::ip::tcp::endpoint endpoint() const
    {
        return acceptor_.local_endpoint();
    }

    void Start()
    {
        auto self = shared_from_this();
        acceptor_.async_accept(socket_,
                               [self](const boost::system::error_code& ec)
                               {
                                   if (ec)
                                       return;
                                   self->ReadAuth();
                               });
    }

private:
    void ReadAuth()
    {
        auto self = shared_from_this();
        boost::asio::async_read(socket_,
                                boost::asio::buffer(frame_),
                                boost::asio::transfer_exactly(frame_.size()),
                                [self](const boost::system::error_code& ec, std::size_t)
                                {
                                    if (ec)
                                        return;
                                    ++self->auth_atts_;
                                    if (FrameView(self->frame_) == kAuth)
                                        self->WriteAuthOk();
                                    else
                                        self->WriteAuthFail();
                                });
    }

    void WriteAuthOk()
    {
        auto self = shared_from_this();
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(kOk.data(), kOk.size()),
                                 [self](const boost::system::error_code& ec, std::size_t)
                                 {
                                     if (!ec)
                                         self->ReadCommand();
                                 });
    }

    void WriteAuthFail()
    {
        auto self = shared_from_this();
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(kFail.data(), kFail.size()),
                                 [self](const boost::system::error_code& ec, std::size_t)
                                 {
                                     if (ec)
                                         return;
                                     if (self->auth_atts_ >= mauth)
                                         self->Close();
                                     else
                                         self->ReadAuth();
                                 });
    }

    void ReadCommand()
    {
        auto self = shared_from_this();
        boost::asio::async_read(socket_,
                                boost::asio::buffer(frame_),
                                boost::asio::transfer_exactly(frame_.size()),
                                [self](const boost::system::error_code& ec, std::size_t)
                                {
                                    if (ec)
                                        return;
                                    ++self->cct_;
                                    if (FrameView(self->frame_) == kWork)
                                        self->WriteAck();
                                    else
                                        self->WriteBye();
                                });
    }

    void WriteAck()
    {
        auto self = shared_from_this();
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(kAck.data(), kAck.size()),
                                 [self](const boost::system::error_code& ec, std::size_t)
                                 {
                                     if (ec)
                                         return;
                                     if (self->cct_ >= kmc)
                                         self->WriteBye();
                                     else
                                         self->ReadCommand();
                                 });
    }

    void WriteBye()
    {
        auto self = shared_from_this();
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(kBye.data(), kBye.size()),
                                 [self](const boost::system::error_code&, std::size_t)
                                 {
                                     self->Close();
                                 });
    }

    void Close()
    {
        boost::system::error_code ignored;
        socket_.close(ignored);
    }

    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
    std::array<char, 4> frame_{};
    int auth_atts_{0};
    int cct_{0};
};

void RunCallbackSession()
{
    boost::asio::io_context io;
    auto server = std::make_shared<CallbackSession>(io);
    server->Start();

    std::thread io_thread(
        [&io]
        {
            io.run();
        });

    boost::asio::ip::tcp::socket client_socket(io);
    client_socket.connect(server->endpoint());
    DriveClient(client_socket, "callback");

    client_socket.close();
    io_thread.join();
}

// ============================================================================

class NestedCallbackSession final : public std::enable_shared_from_this<NestedCallbackSession>
{
public:
    explicit NestedCallbackSession(boost::asio::io_context& io)
        : acceptor_(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0))
        , socket_(io)
    {
    }

    boost::asio::ip::tcp::endpoint endpoint() const
    {
        return acceptor_.local_endpoint();
    }

    void Start()
    {
        auto self = shared_from_this();

        auto read_command = [self](auto&& read_command, int count) -> void
        {
            boost::asio::async_read(
                self->socket_,
                boost::asio::buffer(self->frame_),
                boost::asio::transfer_exactly(self->frame_.size()),
                [self, count, read_command](const boost::system::error_code& ec, std::size_t)
                {
                    if (ec)
                        return;
                    if (FrameView(self->frame_) == kWork)
                    {
                        boost::asio::async_write(
                            self->socket_,
                            boost::asio::buffer(kAck.data(), kAck.size()),
                            [self, count, read_command](const boost::system::error_code& ec, std::size_t)
                            {
                                if (ec)
                                    return;
                                if (count + 1 >= kmc)
                                {
                                    boost::asio::async_write(self->socket_,
                                                             boost::asio::buffer(kBye.data(), kBye.size()),
                                                             [self](const boost::system::error_code&, std::size_t)
                                                             {
                                                                 self->Close();
                                                             });
                                    return;
                                }
                                read_command(read_command, count + 1);
                            });
                        return;
                    }
                    boost::asio::async_write(self->socket_,
                                             boost::asio::buffer(kBye.data(), kBye.size()),
                                             [self](const boost::system::error_code&, std::size_t)
                                             {
                                                 self->Close();
                                             });
                });
        };

        auto read_auth = [self, read_command](auto&& read_auth, int attempt) -> void
        {
            boost::asio::async_read(
                self->socket_,
                boost::asio::buffer(self->frame_),
                boost::asio::transfer_exactly(self->frame_.size()),
                [self, attempt, read_auth, read_command](const boost::system::error_code& ec, std::size_t)
                {
                    if (ec)
                        return;
                    if (FrameView(self->frame_) == kAuth)
                    {
                        boost::asio::async_write(self->socket_,
                                                 boost::asio::buffer(kOk.data(), kOk.size()),
                                                 [self, read_command](const boost::system::error_code& ec, std::size_t)
                                                 {
                                                     if (ec)
                                                         return;
                                                     read_command(read_command, 0);
                                                 });
                        return;
                    }
                    boost::asio::async_write(
                        self->socket_,
                        boost::asio::buffer(kFail.data(), kFail.size()),
                        [self, attempt, read_auth](const boost::system::error_code& ec, std::size_t)
                        {
                            if (ec)
                                return;
                            if (attempt + 1 >= mauth)
                            {
                                self->Close();
                                return;
                            }
                            read_auth(read_auth, attempt + 1);
                        });
                });
        };

        acceptor_.async_accept(socket_,
                               [self, read_auth](const boost::system::error_code& ec)
                               {
                                   if (ec)
                                       return;
                                   read_auth(read_auth, 0);
                               });
    }

private:
    void Close()
    {
        boost::system::error_code ignored;
        socket_.close(ignored);
    }

    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
    std::array<char, 4> frame_{};
    int auth_atts_{0};
    int cct_{0};
};

void RunNestedCallbackSession()
{
    boost::asio::io_context io;
    auto server = std::make_shared<NestedCallbackSession>(io);
    server->Start();

    std::thread io_thread(
        [&io]
        {
            io.run();
        });

    boost::asio::ip::tcp::socket client_socket(io);
    client_socket.connect(server->endpoint());
    DriveClient(client_socket, "nested");

    client_socket.close();
    io_thread.join();
}

} // namespace demo
