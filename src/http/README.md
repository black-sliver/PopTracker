# HTTP

Simple HTTP/S Client around asio / boos::asio.

To enable platform independend certificate checking, set HTTP::certfile to a
file containing the ca certs in PEM fromat.\
See https://curl.se/docs/caextract.html for an example certfile.

Falls back to windows cert store on windows and default openssl paths on other
platforms.

## API

`int HTTP::Get(const std::string& uri, std::string& response[, const std::list<std::string>& headers]);`

Returns HTTP status code, `response` has the response body.

`bool HTTP::GetAsync(asio::io_context& io_context, const std::string& uri[, const std::list<std::string>& headers],
                     const response_callback cb, fail_callback fail = nullptr)`

Pass an asio::io_service as context.\
Returns false if the request could not be started (no callback gets called).\
Calls `cb` on success, or `fail` on error.

## How to use

* Add http.cpp and http.h to your project/Makefile
* Include ASIO or Boost
* For stand-alone ASIO, define ASIO_STANDALONE
* Link against openssl (libssl, libcrypto)