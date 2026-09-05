// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "http_io.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <sstream>

namespace arq_storage {
namespace {
char hex_nibble(unsigned v) noexcept
{
  return static_cast<char>(v < 10 ? '0' + v : 'a' + (v - 10));
}

int from_hex(char c) noexcept
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return -1;
}
} // namespace

std::string url_encode(const std::string_view raw)
{
  std::string out;
  out.reserve(raw.size() * 3);
  for (unsigned char c : raw)
  {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
      out.push_back(static_cast<char>(c));
    else
    {
      out.push_back('%');
      out.push_back(hex_nibble(c >> 4));
      out.push_back(hex_nibble(c & 0xf));
    }
  }
  return out;
}

std::string url_decode(const std::string_view raw)
{
  std::string out;
  out.reserve(raw.size());
  for (std::size_t i = 0; i < raw.size(); ++i)
  {
    if (raw[i] == '%' && i + 2 < raw.size())
    {
      const int hi = from_hex(raw[i + 1]);
      const int lo = from_hex(raw[i + 2]);
      if (hi >= 0 && lo >= 0)
      {
        out.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    out.push_back(raw[i] == '+' ? ' ' : raw[i]);
  }
  return out;
}

std::string query_get(const std::string_view path, const std::string_view key)
{
  const auto q = path.find('?');
  if (q == std::string_view::npos)
    return {};
  auto rest = path.substr(q + 1);
  while (!rest.empty())
  {
    const auto amp = rest.find('&');
    const auto pair = amp == std::string_view::npos ? rest : rest.substr(0, amp);
    const auto eq = pair.find('=');
    const auto k = eq == std::string_view::npos ? pair : pair.substr(0, eq);
    const auto v = eq == std::string_view::npos ? std::string_view{} : pair.substr(eq + 1);
    if (url_decode(k) == key)
      return url_decode(v);
    if (amp == std::string_view::npos)
      break;
    rest.remove_prefix(amp + 1);
  }
  return {};
}

std::string kv_path(const std::string_view ns, const std::string_view key)
{
  return "/v1/kv?ns=" + url_encode(ns) + "&key=" + url_encode(key);
}

std::string snodes_path(const std::string_view pubkey)
{
  return "/v1/snodes?pubkey=" + url_encode(pubkey);
}

std::string format_http_request(const std::string_view method, const std::string_view path,
                                const std::string_view host, const std::string_view body)
{
  std::ostringstream oss;
  oss << method << ' ' << path << " HTTP/1.1\r\nHost: " << host << "\r\nConnection: close\r\n";
  if (!body.empty())
    oss << "Content-Length: " << body.size() << "\r\n";
  oss << "\r\n" << body;
  return oss.str();
}

HttpResult http_exchange(const Endpoint& endpoint, const std::string_view method, const std::string_view path,
                         const std::string_view body, const std::chrono::milliseconds timeout)
{
  HttpResult out;
  if (!endpoint || endpoint.tls)
  {
    out.error = std::make_error_code(std::errc::not_connected);
    return out;
  }
  try
  {
    boost::asio::io_context io;
    boost::asio::ip::tcp::resolver resolver{io};
    const auto results = resolver.resolve(endpoint.host, std::to_string(endpoint.port));
    boost::asio::ip::tcp::socket socket{io};
    boost::system::error_code ec;
    boost::asio::connect(socket, results, ec);
    if (ec)
    {
      out.error = std::make_error_code(std::errc::not_connected);
      return out;
    }
    (void)timeout;
    const auto req = format_http_request(method, path, endpoint.host, body);
    boost::asio::write(socket, boost::asio::buffer(req), ec);
    if (ec)
    {
      out.error = std::make_error_code(std::errc::io_error);
      return out;
    }

    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, "\r\n\r\n", ec);
    if (ec && ec != boost::asio::error::eof)
    {
      out.error = std::make_error_code(std::errc::io_error);
      return out;
    }
    std::istream is{&buf};
    std::string status_line;
    std::getline(is, status_line);
    if (!status_line.empty() && status_line.back() == '\r')
      status_line.pop_back();
    {
      std::istringstream ls{status_line};
      std::string http;
      ls >> http >> out.status;
    }
    std::size_t content_length = 0;
    std::string header;
    while (std::getline(is, header))
    {
      if (!header.empty() && header.back() == '\r')
        header.pop_back();
      if (header.empty())
        break;
      const auto colon = header.find(':');
      if (colon == std::string::npos)
        continue;
      std::string name = header.substr(0, colon);
      for (char& c : name)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      if (name == "content-length")
        content_length = static_cast<std::size_t>(std::strtoul(header.c_str() + colon + 1, nullptr, 10));
    }
    out.body.assign(std::istreambuf_iterator<char>(is), {});
    if (content_length > out.body.size())
    {
      const auto need = content_length - out.body.size();
      std::string rest(need, '\0');
      boost::asio::read(socket, boost::asio::buffer(rest), boost::asio::transfer_exactly(need), ec);
      if (!ec)
        out.body.append(rest);
    }
    socket.close();
    return out;
  }
  catch (...)
  {
    out.error = std::make_error_code(std::errc::io_error);
    return out;
  }
}
} // namespace arq_storage
