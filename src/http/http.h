#include <asio.hpp>
#include <asio/ssl.hpp>
#include <string>
#include <iostream>
#include <functional>
#include <string.h>

#ifdef _MSC_VER 
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#ifdef _WIN32
#include <wincrypt.h>
#endif

//#define HTTP_DEBUG
#ifndef HTTP_DEBUG
#define debug \
    if (true) {} \
    else std::cout
#else
#define debug std::cout
#endif

using asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

class HTTP {
public:
    typedef std::function<void(int code, const std::string&)> response_callback;
    typedef std::function<void(void)> fail_callback;
    
    static std::string certfile; // https://curl.se/docs/caextract.html
    
    static bool Get(const std::string& uri, std::string& response)
    {
        asio::io_context io_context;
        
        bool res = false;
        if (!GetAsync(io_context, uri, [&response, &res](int code, const std::string& r) {
            response = r;
            res = (code == 200);
        })) return false;
        
        io_context.run();
        return res;
    }

    static bool GetAsync(asio::io_context& io_context, const std::string& uri, response_callback cb, fail_callback fail = nullptr)
    {
        std::string proto, host, port, path;
        if (!parse_uri(uri, proto, host, port, path)) return false;
        if (path.empty()) path = "/";
        
        std::string request = 
                "GET " + path + " HTTP/1.1\r\n"
                //"Accept: */*\r\n"
                "Connection: close\r\n"
                "DNT: 1\r\n"
                "Host: " + host + "\r\n"
                "User-Agent: PopTracker\r\n"
                "\r\n";
        
        std::cout << "HTTP: connecting to " << proto << "," << host << "," << port << " for " << path << "\n";
        
        base_client *c;
        tcp::resolver *resolver;
        if (strcasecmp(proto.c_str(), "https") == 0) {
            if (port.empty()) port = "443";
            resolver = new tcp::resolver(io_context);
            auto endpoints = resolver->resolve(host, port);

            asio::ssl::context ctx(asio::ssl::context::tlsv12_client);
            ctx.set_options(asio::ssl::context::default_workarounds
                            | asio::ssl::context::no_sslv2
                            | asio::ssl::context::no_sslv3
                            | asio::ssl::context::tlsv12_client);

            bool load_system_certs = true;
            if (!certfile.empty()) {
                debug << "HTTP: loading " << certfile << "\n";
                asio::error_code ec;
                ctx.load_verify_file(certfile, ec);
                load_system_certs = !!ec;
            }
            if (load_system_certs) {
#ifdef _WIN32
                debug << "HTTP: loading windows cert store\n";
                add_windows_root_certs(ctx);
#else
                debug << "HTTP: adding system certs\n";
                ctx.set_default_verify_paths();
#endif
            }

            ctx.set_verify_mode(asio::ssl::verify_peer
                                | asio::ssl::verify_fail_if_no_peer_cert
                                | asio::ssl::verify_client_once);
            ctx.set_verify_callback(asio::ssl::rfc2818_verification(host));
            c = new ssl_client(io_context, ctx, endpoints, request, host);
        }
        else if (strcasecmp(proto.c_str(), "http") == 0) {
            if (port.empty()) port = "80";
            resolver = new tcp::resolver(io_context);
            auto endpoints = resolver->resolve(host, port);
            c = new tcp_client(io_context, endpoints, request);
        }
        else {
            // unsupported protocol
            return false;
        }

        c->set_fail_handler([fail,resolver,c](...) {
            c->stop();
            if (fail) fail();
            delete resolver;
            delete c;
        });
        c->set_response_handler([cb,resolver,c](int code, const std::string& r) {
            c->stop();
            if (cb) cb(code, r);
            delete resolver;
            delete c;
        });

        return true;
    }
    
private:
    static bool parse_uri(const std::string& uri, std::string& proto, std::string& host, std::string& port, std::string& path)
    {
        std::string::size_type pos = uri.find("://");
        if (pos == uri.npos) return false;
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

    enum { max_length = 1024 };

    class base_client
    {
    protected:
        base_client(const std::string& request)
                : request(request), code(-1), chunked(false),
                  in_content(false), content_length(0),
                  response_handler(nullptr), fail_handler(nullptr)
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
        
        int get_error_code() const
        {
            return code;
        }

        virtual void stop() = 0;

    protected:
        template<typename AsyncWriteStream>
        void send_request(AsyncWriteStream& socket_)
        {
            asio::async_write(socket_,
            asio::buffer(request.data(), request.length()),
                    [this, &socket_](const asio::error_code& error, std::size_t length)
            {
                if (!error)
                {
                    receive_response(socket_);
                }
                else
                {
                    std::cout << "HTTP: Write failed: " << error.message() << "\n";
                    if (fail_handler) fail_handler();
                }
            });
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
                        if (code >= 100 && response_handler)
                            response_handler(code, content);
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
                        debug << "HTTP: Status: " << data.substr(0, pos) << "\n";
                        if (strncasecmp(data.c_str(), "HTTP/1.", 7) == 0)
                            code = atoi(data.c_str() + 9);
                        if (code<100 || code>999) {
                            std::cout << "HTTP: bad status " << code << "\n";
                            code = -1;
                            return true;
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
                        debug << "HTTP: HDR: " << name << ": " << value << "\n";
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
                    content += data.substr(pos+2, chunk_len);
                    data = data.substr(pos+2+chunk_len+2);
                    last_pos = 0;
                }
                else {
                    // handle regular content
                    content += data.substr(last_pos);
                    data.clear();
                    last_pos = 0;
                    break;
                }
            }
            if (last_pos) data = data.substr(last_pos);
            if (in_content && !chunked) return content.length() >= content_length;
            return false;
        }

    private:
        std::string request;
        char buffer[max_length];
        std::string data;
        std::string content;
        int code;
        bool chunked;
        bool in_content;
        size_t content_length;
        response_callback response_handler;

    protected:
        fail_callback fail_handler;
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
            socket_.set_verify_callback(asio::ssl::rfc2818_verification(host));
            connect(endpoints);
        }

        virtual ~ssl_client()
        {
        }

        virtual void stop() {
            socket_.shutdown();
            //socket_.lowest_layer().close();
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
        }

        asio::ssl::stream<tcp::socket> socket_;
    };
};
