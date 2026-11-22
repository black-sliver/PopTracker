//
// ssl/impl/rfc2818_verification.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_SSL_IMPL_RFC2818_VERIFICATION_IPP
#define ASIO_SSL_IMPL_RFC2818_VERIFICATION_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if !defined(ASIO_NO_DEPRECATED)

// NOTE: ASN1_STRING_length was deprecated in OpenSSL 4.1,
//       however the replacement ASN1_STRING_get_length is not available in 3.x, which we still support.
// TODO: update ASIO to get proper OpenSSL 4.x support?
#ifndef OPENSSL_API_COMPAT
#define OPENSSL_API_COMPAT
#endif
#ifndef OPENSSL_NO_DEPRECATED_4_1
#define OPENSSL_NO_DEPRECATED_4_1
#endif
#include <cctype>
#include <cstring>
#include "asio/ip/address.hpp"
#include "asio/ssl/rfc2818_verification.hpp"
#include "asio/ssl/detail/openssl_types.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ssl {

bool rfc2818_verification::operator()(
    const bool preverified, verify_context& ctx) const
{
  using namespace std; // For memcmp.

  // Don't bother looking at certificates that have failed pre-verification.
  if (!preverified)
    return false;

  // We're only interested in checking the certificate at the end of the chain.
  int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());
  if (depth > 0)
    return true;

  // Try converting the host name to an address. If it is an address then we
  // need to look for an IP address in the certificate rather than a host name.
  asio::error_code ec;
  ip::address address = ip::make_address(host_, ec);
  bool is_address = !ec;

  X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

  // Go through the alternate names in the certificate looking for matching DNS
  // or IP address entries.
  GENERAL_NAMES* gens = static_cast<GENERAL_NAMES*>(
      X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr));
  for (int i = 0; i < sk_GENERAL_NAME_num(gens); ++i)
  {
    GENERAL_NAME* gen = sk_GENERAL_NAME_value(gens, i);
    if (gen->type == GEN_DNS && !is_address)
    {
      ASN1_IA5STRING* domain = gen->d.dNSName;
      if (ASN1_STRING_type(domain) == V_ASN1_IA5STRING && ASN1_STRING_get0_data(domain) && ASN1_STRING_length(domain))
      {
        const char* pattern = reinterpret_cast<const char*>(ASN1_STRING_get0_data(domain));
        const std::size_t pattern_length = ASN1_STRING_length(domain);
        if (match_pattern(pattern, pattern_length, host_.c_str()))
        {
          GENERAL_NAMES_free(gens);
          return true;
        }
      }
    }
    else if (gen->type == GEN_IPADD && is_address)
    {
      const ASN1_OCTET_STRING* ip_address = gen->d.iPAddress;
      if (ASN1_STRING_type(ip_address) == V_ASN1_OCTET_STRING && ASN1_STRING_get0_data(ip_address))
      {
        if (address.is_v4() && ASN1_STRING_length(ip_address) == 4)
        {
          ip::address_v4::bytes_type bytes = address.to_v4().to_bytes();
          if (memcmp(bytes.data(), ASN1_STRING_get0_data(ip_address), 4) == 0)
          {
            GENERAL_NAMES_free(gens);
            return true;
          }
        }
        else if (address.is_v6() && ASN1_STRING_length(ip_address) == 16)
        {
          ip::address_v6::bytes_type bytes = address.to_v6().to_bytes();
          if (memcmp(bytes.data(), ASN1_STRING_get0_data(ip_address), 16) == 0)
          {
            GENERAL_NAMES_free(gens);
            return true;
          }
        }
      }
    }
  }
  GENERAL_NAMES_free(gens);

  // No match in the alternate names, so try the common names. We should only
  // use the "most specific" common name, which is the last one in the list.
  const X509_NAME* name = X509_get_subject_name(cert);
  int i = -1;
  const ASN1_STRING* common_name = nullptr;
  while ((i = X509_NAME_get_index_by_NID(name, NID_commonName, i)) >= 0)
  {
    const X509_NAME_ENTRY* name_entry = X509_NAME_get_entry(name, i);
    common_name = X509_NAME_ENTRY_get_data(name_entry);
  }
  if (common_name && ASN1_STRING_get0_data(common_name) && ASN1_STRING_length(common_name))
  {
    const char* pattern = reinterpret_cast<const char*>(ASN1_STRING_get0_data(common_name));
    const std::size_t pattern_length = ASN1_STRING_length(common_name);
    if (match_pattern(pattern, pattern_length, host_.c_str()))
      return true;
  }

  return false;
}

bool rfc2818_verification::match_pattern(const char* pattern,
    const std::size_t pattern_length, const char* host)
{
  using namespace std; // For tolower.

  const char* p = pattern;
  const char* p_end = p + pattern_length;
  const char* h = host;

  while (p != p_end && *h)
  {
    if (*p == '*')
    {
      ++p;
      while (*h && *h != '.')
        if (match_pattern(p, p_end - p, h++))
          return true;
    }
    else if (tolower(*p) == tolower(*h))
    {
      ++p;
      ++h;
    }
    else
    {
      return false;
    }
  }

  return p == p_end && !*h;
}

} // namespace ssl
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // !defined(ASIO_NO_DEPRECATED)

#endif // ASIO_SSL_IMPL_RFC2818_VERIFICATION_IPP
