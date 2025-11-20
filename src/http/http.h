#ifndef _HTTP_HTTP_H
#define _HTTP_HTTP_H

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <string>
#include <iostream>
#include <functional>
#include <cstring>
#include <list>
#include <set>
#include <map>

#ifdef _MSC_VER 
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#ifdef _WIN32
#include <wincrypt.h>
#endif

//#define HTTP_DEBUG
#ifndef HTTP_DEBUG
#define http_debug \
    if (true) {} \
    else std::cout
#else
#define http_debug std::cout
#endif

using asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

class HTTP {
public:
    struct ci_less
    {
        // case-independent (ci) compare_less binary function
        struct nocase_compare
        {
            bool operator() (const unsigned char& c1, const unsigned char& c2) const {
                return tolower(c1) < tolower(c2);
            }
        };
        bool operator() (const std::string & s1, const std::string & s2) const {
            return std::lexicographical_compare(
                    s1.begin(), s1.end(),   // source range
                    s2.begin(), s2.end(),   // dest range
                    nocase_compare());      // comparison
        }
    };
    typedef std::map<std::string, std::string, ci_less> Headers;

    typedef std::function<void(int code, const std::string&, const Headers&)> response_callback;
    typedef std::function<void(void)> fail_callback;
    typedef std::function<void(int received, int total)> progress_callback;
    
    static std::string certfile; // https://curl.se/docs/caextract.html
    static std::set<std::string> dnt_trusted_hosts; // try not to send trackable headers to hosts other than these

    enum {
        INTERNAL_ERROR = -1,
        OK = 200,
        REDIRECT = 302,
        NOT_MODIFIED = 304
    };

    static int Get(const std::string& uri, std::string& response,
            const std::list<std::string>& headers = {}, int redirect_limit = 3)
    {
        asio::io_context io_context;

        int res = -1;
        if (!GetAsync(io_context, uri, headers, [&response, &res, headers, redirect_limit]
                (const int code, const std::string& r, const Headers&)
        {
            if (code == REDIRECT && redirect_limit != 0) {
                // NOTE: 302 puts location into r to make life easier
                res = Get(r, response, headers, redirect_limit - 1);
                if (res == REDIRECT) {
                    response = "Too many redirects";
                    res = INTERNAL_ERROR;
                }
            } else {
                response = r;
                res = code;
            }
        })) {
            response.clear();
            return -1;
        }

        io_context.run();
        return res;
    }

    static bool GetAsync(asio::io_context& io_context, const std::string& uri, response_callback cb, fail_callback fail = nullptr, FILE* f = nullptr, progress_callback progress = nullptr)
    {
        return GetAsync(io_context, uri, {}, cb, fail, f, progress);
    }
    static bool GetAsync(asio::io_context& io_context, const std::string& uri, const std::list<std::string>& headers, response_callback cb, fail_callback fail = nullptr, FILE* f = nullptr, progress_callback progress = nullptr)
    {
        std::string proto, host, port, path;
        if (!parse_uri(uri, proto, host, port, path)) return false;
        if (path.empty()) path = "/";

        long dnt = -1;
        for (const auto& header: headers) {
            if (strncasecmp(header.c_str(), "dnt:", 4) == 0) {
                if (parse_long(header.c_str()+4, dnt)) break;
            }
        }

        std::string request = 
                "GET " + path + " HTTP/1.1\r\n"
                //"Accept: */*\r\n"
                "Connection: close\r\n"
                "Host: " + host + "\r\n"
                "User-Agent: PopTracker\r\n";
        for (const auto& header: headers) {
            if (header.empty() || header.find("\n") != header.npos) {
                std::cerr << "Invalid headers supplied to GetAsync:\n  " << header << "\n";
                return false;
            }
            if (dnt != 0 && strncasecmp(header.c_str(), "If-None-Match:", 14) == 0 &&
                    dnt_trusted_hosts.find(host) == dnt_trusted_hosts.end()) {
                continue;
            }
            request.append(header + "\r\n");
        }
        request += "\r\n";

        std::cout << "HTTP: connecting via " << proto << " to " << host << " " << port << " for " << path << "\n";

        base_client *c;
        tcp::resolver *resolver;
        if (strcasecmp(proto.c_str(), "https") == 0) {
            if (port.empty()) port = "443";
            resolver = new tcp::resolver(io_context);
            asio::error_code ec;
            auto endpoints = resolver->resolve(host, port, ec);
            if (ec) {
                std::cout << "HTTP: failed to resolve host: " << ec.message() << "\n";
                return false;
            }

            asio::ssl::context ctx(asio::ssl::context::tlsv12_client);
            ctx.set_options(asio::ssl::context::default_workarounds
                            | asio::ssl::context::no_sslv2
                            | asio::ssl::context::no_sslv3
                            | asio::ssl::context::no_tlsv1
                            | asio::ssl::context::no_tlsv1_1
                            | asio::ssl::context::single_dh_use);

            bool load_system_certs = true;
            if (!certfile.empty()) {
                http_debug << "HTTP: loading " << certfile << "\n";
                asio::error_code ec;
                ctx.load_verify_file(certfile, ec);
                if (ec) {
                    std::cout << "HTTP: error loading certs from "
                              << certfile << ": "
                              << ec.message() << "\n";
                } else {
                    http_debug << "HTTP: loaded certs from "
                          << certfile << "\n";
                    load_system_certs = false;
                }
            }
            if (load_system_certs) {
#ifdef _WIN32
                http_debug << "HTTP: loading windows cert store\n";
                add_windows_root_certs(ctx);
#else
                http_debug << "HTTP: adding system certs\n";
                ctx.set_default_verify_paths();
#endif
            }

            ctx.set_verify_mode(asio::ssl::verify_peer
                                | asio::ssl::verify_fail_if_no_peer_cert
                                | asio::ssl::verify_client_once);
            ctx.set_verify_callback(asio::ssl::rfc2818_verification(host)); // overwritten later on socket level
            c = new ssl_client(io_context, ctx, endpoints, request, host);
        }
        else if (strcasecmp(proto.c_str(), "http") == 0) {
            if (port.empty()) port = "80";
            resolver = new tcp::resolver(io_context);
            asio::error_code ec;
            auto endpoints = resolver->resolve(host, port, ec);
            if (ec) {
                std::cout << "HTTP: failed to resolve host: " << ec.message() << "\n";
                return false;
            }
            c = new tcp_client(io_context, endpoints, request);
        }
        else {
            // unsupported protocol
            return false;
        }
        c->set_response_file(f); // if f is not null, output to file
        c->set_progress_handler(progress);

        c->set_fail_handler([fail,resolver,c](...) {
            c->stop();
            if (fail) fail();
            delete resolver;
            delete c;
        });
        c->set_response_handler([cb,resolver,c](int code, const std::string& r, HTTP::Headers h) {
            c->stop();
            if (cb) cb(code, r, h);
            delete resolver;
            delete c;
        });

        return true;
    }

    static bool is_uri(const std::string& uri)
    {
        std::string proto, host, port, path;
        return parse_uri(uri, proto, host, port, path);
    }

    static bool parse_uri(const std::string& uri, std::string& proto, std::string& host, std::string& port, std::string& path)
    {
        auto allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=%";
        for (const auto c: uri)
            if (!strchr(allowed, c))
                return false;
        std::string::size_type pos = uri.find("://");
        if (pos == uri.npos)
            return false;
        proto = uri.substr(0, pos);
        std::string::size_type pos2 = uri.find("/", pos+3);
        std::string::size_type pos3 = uri.find(":", pos+3);
        if (pos2 == uri.npos && pos3 == uri.npos) {
            host = uri.substr(pos+3);
            port = "";
            path = "";
        } else if (pos2 < pos3) {
            host = uri.substr(pos+3, pos2-(pos+3));
            port = "";
            path = uri.substr(pos2);
        } else {
            host = uri.substr(pos+3, pos3-(pos+3));
            if (pos2 == uri.npos) {
                port = uri.substr(pos3+1);
                path = "";
            } else {
                port = uri.substr(pos3+1, pos2-pos3-1);
                path = uri.substr(pos2);
            }
        }
        return true;
    }

private:
    static bool parse_long(const char* s, long& out)
    {
        char* next = NULL;
        long res = strtol(s, & next, 10);
        if (errno == 0 && next != s) {
            out = res;
            return true;
        }
        return false;
    }
    
#ifdef _WIN32
    // https://stackoverflow.com/questions/39772878/reliable-way-to-get-root-ca-certificates-on-windows/40710806
    static void add_windows_root_certs(asio::ssl::context &ctx)
    {
        HCERTSTORE hStore = CertOpenSystemStore(0, "ROOT");
        if (hStore == NULL) {
            return;
        }

        X509_STORE *store = X509_STORE_new();
        PCCERT_CONTEXT pContext = NULL;
        while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL) {
            // convert from DER to internal format
            X509 *x509 = d2i_X509(NULL,
                                  (const unsigned char **)&pContext->pbCertEncoded,
                                  pContext->cbCertEncoded);
            if(x509 != NULL) {
                X509_STORE_add_cert(store, x509);
                X509_free(x509);
            }
        }

        CertFreeCertificateContext(pContext);
        CertCloseStore(hStore, 0);

        // attach X509_STORE to boost ssl context
        SSL_CTX_set_cert_store(ctx.native_handle(), store);
    }
#endif

    enum { max_length = 8192 };

    class base_client
    {
    protected:
        base_client(const std::string& request)
                : request(request), code(-1), chunked(false),
                  in_content(false), content_length(0),
                  received_length(0), last_progress_report(0),
                  response_file(nullptr), response_handler(nullptr),
                  fail_handler(nullptr), progress_handler(nullptr)
        {
        }

    public:
        virtual ~base_client()
        {
        }

        void set_response_handler(response_callback cb)
        {
            response_handler = cb;
        }

        void set_fail_handler(fail_callback cb)
        {
            fail_handler = cb;
        }

        void set_progress_handler(progress_callback cb)
        {
            progress_handler = cb;
        }

        int get_error_code() const
        {
            return code;
        }

        const std::string& get_redirect() const
        {
            return location;
        }

        void set_response_file(FILE* f)
        {
            response_file = f;
        }

        virtual void stop() = 0;

    protected:
        template<typename AsyncWriteStream>
        void send_request(AsyncWriteStream& socket_)
        {
            size_t requestLength = request.length();
            asio::async_write(
                socket_,
                asio::buffer(request.data(), requestLength),
                [this, requestLength, &socket_](const asio::error_code& error, std::size_t written)
                {
                    // TODO: handle written < requestLength
                    if (!error && written == requestLength) {
                        receive_response(socket_);
                    } else {
                        std::cout << "HTTP: Write failed: " << error.message() << "\n";
                        if (fail_handler)
                            fail_handler();
                    }
                }
            );
        }

        template<typename AsyncWriteStream>
        void receive_response(AsyncWriteStream& socket_)
        {
            asio::async_read(socket_, asio::buffer(buffer, max_length),
                    [this, &socket_](const asio::error_code& error, std::size_t length)
            {
                if (!error || error == asio::error::eof)
                {
                    data.append(buffer, length);
                    bool done = parse_http();
                    if (!done && error == asio::error::eof) {
                        code = -1;
                        fail_handler();
                    } else if (done) {
                        //if (error != asio::error::eof)
                        //    socket_.shutdown();
                        if (code < 100 && fail_handler)
                            fail_handler();
                        else if (code == 302 && response_handler)
                            response_handler(code, location, headers);
                        else if (code >= 100 && response_handler)
                            response_handler(code, content, headers);
                    } else {
                        receive_response(socket_);
                    }
                }
                else
                {
                    std::cout << "HTTP: Read failed: " << error.message() << "\n";
                    if (fail_handler) fail_handler();
                }
            });
        }
        
        bool parse_header(const std::string& in, std::string& name, std::string& value)
        {
            std::string::size_type pos = in.find(":");
            if (pos == in.npos) return false;
            name = in.substr(0, pos);
            value = in.substr(in.find_first_not_of(" ", pos+1));
            return true;
        }

        bool parse_http()
        {
            std::string::size_type last_pos = 0;
            while (true) {
                if (code == -1) {
                    // handle status line
                    auto pos = data.find("\r\n");
                    if (pos != data.npos) {
                        http_debug << "HTTP: Status: " << data.substr(0, pos) << "\n";
                        if (strncasecmp(data.c_str(), "HTTP/1.", 7) == 0) {
                            char* next = nullptr;
                            code = (int)strtol(data.c_str() + 9, &next, 10);
                            if (next && *next) {
                                auto p = next-data.c_str();
                                status = data.substr(p+1, pos-p-1);
                            }
                        }
                        if (code<100 || code>999) {
                            std::cout << "HTTP: bad status " << code << "\n";
                            content = "bad status";
                            code = -1;
                            return true;
                        } else if (code<200 || code>=300) {
                            content = status;
                        }
                        last_pos = pos + 2;
                    } else {
                        // more data required
                        break;
                    }
                }
                else if (!in_content) {
                    // handle headers
                    auto pos = data.find("\r\n", last_pos);
                    if (pos != data.npos) {
                        // check if headers are done
                        if (pos == last_pos) {
                            in_content = true;
                            last_pos += 2;
                            continue;
                        }
                        // parse headers
                        std::string name, value;
                        if (!parse_header(data.substr(last_pos, pos-last_pos), name, value)) {
                            code = -1;
                            return true;
                        }
                        http_debug << "HTTP: HDR: " << name << ": " << value << "\n";
                        headers[name] = value;
                        if (strcasecmp(name.c_str(), "Transfer-Encoding") == 0) {
                            if (strcasecmp(value.c_str(), "chunked") == 0) {
                                chunked = true;
                            } else if (strcasecmp(value.c_str(), "identity") == 0) {
                                chunked = false;
                            } else {
                                // unsupported Transfer-Encoding
                                code = -1;
                                return true;
                            }
                        }
                        else if (strcasecmp(name.c_str(), "Content-Length") == 0) {
                            content_length = (size_t)std::stoll(value);
                        }
                        else if (strcasecmp(name.c_str(), "Location") == 0) {
                            location = value;
                        }
                        // next line
                        last_pos = pos + 2;
                    } else {
                        // more data required
                        break;
                    }
                }
                else if (chunked) {
                    // handle chunked content
                    size_t pos = data.find("\r\n", last_pos);
                    if (pos == data.npos) {
                        if (data.length() - last_pos >= 18) {
                            // invalid chunk header
                            code = -1;
                            return true;
                        }
                        // incomplete chunk header
                        break;
                    }
                    char* next = nullptr;
                    size_t chunk_len = (size_t)strtoll(data.c_str()+last_pos, &next, 16);
                    if (next != data.c_str()+pos) {
                        // bad chunk header
                        code = -1;
                        return true;
                    }
                    if (data.length() - pos < chunk_len + 4) {
                        // chunk incomplete
                        break;
                    }
                    if (chunk_len == 0) {
                        // last chunk
                        return true;
                    }
                    if (response_file) {
                        if (fwrite(data.c_str()+pos+2, 1, chunk_len, response_file) != chunk_len) {
                            code = INTERNAL_ERROR;
                            content = strerror(errno);
                            if (fail_handler) fail_handler();
                            return true; // error -> done
                        }
                    } else {
                        content += data.substr(pos+2, chunk_len);
                    }
                    received_length += chunk_len;
                    data = data.substr(pos+2+chunk_len+2);
                    last_pos = 0;
                }
                else {
                    // handle regular content
                    size_t chunk_len = data.length() - last_pos;
                    if (response_file) {
                        if (fwrite(data.c_str()+last_pos, 1, chunk_len, response_file) != chunk_len) {
                            code = INTERNAL_ERROR;
                            content = strerror(errno);
                            if (fail_handler) fail_handler();
                            return true; // error -> done
                        }
                    } else {
                        content += data.substr(last_pos);
                    }
                    received_length += chunk_len;
                    data.clear();
                    last_pos = 0;
                    break;
                }
            }
            if (last_pos) data = data.substr(last_pos);
            if (in_content && progress_handler && received_length && (received_length-last_progress_report>=2048 || received_length == content_length)) {
                last_progress_report = received_length;
                progress_handler((int)received_length, (int)content_length);
            }
            if (in_content && !chunked) return received_length >= content_length;
            return false;
        }

    private:
        std::string location;
        std::string request;
        Headers headers;
        std::string content;
        int code;
        bool chunked;
        bool in_content;
        size_t content_length;
        size_t received_length;
        size_t last_progress_report;
        std::string status;
        FILE* response_file;
        std::string data;
        char buffer[max_length];
        response_callback response_handler;

    protected:
        fail_callback fail_handler;
        progress_callback progress_handler;
    };

    template <typename V>
    class verbose_verification
    {
    public:
        verbose_verification(V verifier)
          : verifier_(verifier)
        {}

        bool operator()(bool preverified, asio::ssl::verify_context& ctx)
        {
            char subject_name[256];
            X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
            X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
            bool verified = verifier_(preverified, ctx);
            std::cout << "HTTP: Verifying: " << subject_name << " ... " << verified << "\n";
            return verified;
        }
    private:
        V verifier_;
    };

    class tcp_client : public base_client
    {
    public:
        tcp_client(asio::io_context& io_context,
                const tcp::resolver::results_type& endpoints,
                const std::string& request)
                : base_client(request), socket_(io_context)
        {
            connect(endpoints);
        }

        virtual ~tcp_client()
        {
        }

        virtual void stop() {
            socket_.close();
        }

    private:
        void connect(const tcp::resolver::results_type& endpoints)
        {
            asio::async_connect(socket_.lowest_layer(), endpoints,
                    [this](const asio::error_code& error,
                           const tcp::endpoint& /*endpoint*/)
            {
                if (!error)
                {
                    http_debug << "HTTP: Connected" << "\n";
                    send_request(socket_);
                }
                else
                {
                    std::cout << "HTTP: Connect failed: " << error.message() << "\n";
                    if (fail_handler) fail_handler();
                }
            });
        }

        tcp::socket socket_;
    };
    
    class ssl_client : public base_client
    {
    public:
        ssl_client(asio::io_context& io_context,
                asio::ssl::context& context,
                const tcp::resolver::results_type& endpoints,
                const std::string& request, const std::string& host)
                : base_client(request), socket_(io_context, context)
        {
#ifdef HTTP_DEBUG
            socket_.set_verify_callback(verbose_verification(asio::ssl::rfc2818_verification(host)));
#else
            socket_.set_verify_callback(asio::ssl::rfc2818_verification(host));
#endif

#ifndef HTTP_NO_SNI
            if (!SSL_set_tlsext_host_name(socket_.native_handle(), host.c_str()))
            {
                asio::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
                asio::detail::throw_error(ec, "set_tlsext_host_name");
            }
#endif
            connect(endpoints);
        }

        virtual ~ssl_client()
        {
        }

        virtual void stop() {
            asio::error_code ec;
            socket_.shutdown(ec);
            // if we get an error here, SSL connection probably just failed
            if (ec) {
                http_debug << "HTTP: ssl shutdown failed: " << ec.message() << "\n";
                // if SSL was not up, try to close the underlying socket
                socket_.lowest_layer().close(ec);
            }
        }

    private:
        void connect(const tcp::resolver::results_type& endpoints)
        {
            asio::async_connect(socket_.lowest_layer(), endpoints,
                    [this](const asio::error_code& error,
                           const tcp::endpoint& /*endpoint*/)
            {
                if (!error)
                {
                    http_debug << "HTTP: Connected" << "\n";
                    handshake();
                }
                else
                {
                    std::cout << "HTTP: Connect failed: " << error.message() << "\n";
                    if (fail_handler) fail_handler();
                }
            });
        }

        void handshake()
        {
            try {
                socket_.async_handshake(asio::ssl::stream_base::client,
                        [this](const asio::error_code& error)
                {
                    if (!error)
                    {
                        send_request(socket_);
                    }
                    else
                    {
                        std::cout << "HTTP: SSL handshake failed: " << error.message() << "\n";
                        if (fail_handler) fail_handler();
                    }
                });
            } catch (std::exception& ex) {
                std::cout << "HTTP: Error initializing SSL handshake: " << ex.what() << "\n";
                if (fail_handler) fail_handler();
            }
        }

        asio::ssl::stream<tcp::socket> socket_;
    };
};

#endif // _HTTP_HTTP_H
