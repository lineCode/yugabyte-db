// Copyright (c) YugaByte, Inc.

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "yb/util/net/inetaddress.h"

namespace yb {

InetAddress::InetAddress() {
}

InetAddress::InetAddress(const boost::asio::ip::address& address)
    : boost_addr_(address) {
}

InetAddress::InetAddress(const InetAddress& other) {
  boost_addr_ = other.boost_addr_;
}

CHECKED_STATUS ResolveInternal(const std::string& host,
                               boost::asio::ip::tcp::resolver::iterator* iter) {
  boost::system::error_code ec;
  boost::asio::io_service io;
  boost::asio::ip::tcp::resolver resolver(io);
  // Port 80 is just a placeholder since boost doesn't seem to support a DNS lookup API without
  // the port.
  boost::asio::ip::tcp::resolver::query query(host, "80");
  *iter = resolver.resolve(query, ec);
  if (ec.value()) {
    return STATUS_SUBSTITUTE(InvalidArgument, "$0 is an invalid host/ip address: $1", host,
                             ec.message());
  }
  return Status::OK();
}

CHECKED_STATUS InetAddress::Resolve(const std::string& host, std::vector<InetAddress>* addresses) {
  boost::asio::ip::tcp::resolver::iterator iter;
  RETURN_NOT_OK(ResolveInternal(host, &iter));
  boost::asio::ip::tcp::resolver::iterator end;
  while (iter != end) {
    addresses->push_back(InetAddress(iter->endpoint().address()));
    iter++;
  }
  return Status::OK();
}

CHECKED_STATUS InetAddress::FromString(const std::string& strval) {
  boost::asio::ip::tcp::resolver::iterator iter;
  RETURN_NOT_OK(ResolveInternal(strval, &iter));
  // Pick the first IP address in the list.
  boost_addr_ = iter->endpoint().address();
  return Status::OK();
}

std::string InetAddress::ToString() const {
  std::string strval;
  CHECK_OK(ToString(&strval));
  return strval;
}

CHECKED_STATUS InetAddress::ToString(std::string *strval) const {
  boost::system::error_code ec;
  *strval = boost_addr_.to_string(ec);
  if (ec.value()) {
    return STATUS(IllegalState, "InetAddress object cannot be converted to string: $0",
                  ec.message());
  }
  return Status::OK();
}

CHECKED_STATUS InetAddress::ToBytes(std::string* bytes) const {
  try {
    if (boost_addr_.is_v4()) {
      auto v4bytes = boost_addr_.to_v4().to_bytes();
      bytes->assign(reinterpret_cast<char *>(v4bytes.data()), v4bytes.size());
    } else if (boost_addr_.is_v6()) {
      auto v6bytes = boost_addr_.to_v6().to_bytes();
      bytes->assign(reinterpret_cast<char *>(v6bytes.data()), v6bytes.size());
    } else {
      return STATUS(Uninitialized, "InetAddress doesn't hold a valid IPv4 or IPv6 address");
    }
  } catch (std::exception& e) {
    return STATUS(Corruption, "Couldn't serialize InetAddress to raw bytes!");
  }
  return Status::OK();
}

CHECKED_STATUS InetAddress::FromSlice(const Slice& slice, size_t size_hint) {
  size_t expected_size = (size_hint == 0) ? slice.size() : size_hint;
  if (expected_size > slice.size()) {
    return STATUS_SUBSTITUTE(InvalidArgument, "Size of slice: $0 is smaller than provided "
        "size_hint: $1", slice.size(), expected_size);
  }
  if (expected_size == kV4Size) {
    boost::asio::ip::address_v4::bytes_type v4bytes;
    DCHECK_EQ(expected_size, v4bytes.size());
    memcpy(v4bytes.data(), slice.data(), v4bytes.size());
    boost::asio::ip::address_v4 v4address(v4bytes);
    boost_addr_ = v4address;
  } else if (expected_size == kV6Size) {
    boost::asio::ip::address_v6::bytes_type v6bytes;
    DCHECK_EQ(expected_size, v6bytes.size());
    memcpy(v6bytes.data(), slice.data(), v6bytes.size());
    boost::asio::ip::address_v6 v6address(v6bytes);
    boost_addr_ = v6address;
  } else {
    return STATUS_SUBSTITUTE(InvalidArgument, "Size of slice is invalid: $0", expected_size);
  }
  return Status::OK();
}

CHECKED_STATUS InetAddress::FromBytes(const std::string& bytes) {
  Slice slice (bytes.data(), bytes.size());
  return FromSlice(slice);
}

} // namespace yb