
#include <curl/curl.h>
#include <map>
#include <sstream>
#include <iostream>
#include <cstdlib>

namespace ThorsAnvil
{
    namespace Socket
    {

template<std::size_t I = 0, typename... Args>
int print(std::ostream& s, Args... args)
{
    using Expander = int[];
    return Expander{ 0, ((s << std::forward<Args>(args)), 0)...}[0];
}

template<typename... Args>
std::string buildErrorMessage(Args const&... args)
{
    std::stringstream msg;
    print(msg, args...);
    return msg.str();
}

class CurlGlobal
{
    public:
        CurlGlobal()
        {
            if (curl_global_init(CURL_GLOBAL_ALL) != 0)
            {
                throw std::runtime_error(buildErrorMessage("CurlGlobal::", __func__, ": curl_global_init: fail"));
            }
        }
        ~CurlGlobal()
        {
            curl_global_cleanup();
        }
};

enum RequestType {Get, Head, Put, Post, Delete};
class CurlConnector
{
    CURL*       curl;
    std::string host;
    int         port;
    public:
        CurlConnector(std::string const& host, int port)
            : curl(curl_easy_init( ))
            , host(host)
            , port(port)
        {
            if (curl == NULL)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::", __func__, ": curl_easy_init: fail"));
            }
        }
        ~CurlConnector()
        {
            curl_easy_cleanup(curl);
        }

        virtual RequestType getRequestType() const {return Post;}

        void sendMessage(std::string const& urlPath, std::string const& message)
        {
            std::stringstream url;
            url << "http://" << host;
            if (port != 80)
            {
                url << ":" << port;
            }
            url << urlPath;

            CURLcode res;
            auto sListDeleter = [](struct curl_slist* headers){curl_slist_free_all(headers);};
            std::unique_ptr<struct curl_slist, decltype(sListDeleter)> headers(nullptr, sListDeleter);
            headers = std::unique_ptr<struct curl_slist, decltype(sListDeleter)>(curl_slist_append(headers.get(), "Content-Type: text/text"), sListDeleter);

            if ((res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers.get())) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::", __func__, ": curl_easy_setopt CURLOPT_HTTPHEADER:", curl_easy_strerror(res)));
            }
            if ((res = curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "*/*")) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::", __func__, ": curl_easy_setopt CURLOPT_ACCEPT_ENCODING:", curl_easy_strerror(res)));
            }
            if ((res = curl_easy_setopt(curl, CURLOPT_USERAGENT, "ThorsExperimental-Client/0.1")) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::", __func__, ": curl_easy_setopt CURLOPT_USERAGENT:", curl_easy_strerror(res)));
            }
            if ((res = curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str())) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::", __func__, ": curl_easy_setopt CURLOPT_URL:", curl_easy_strerror(res)));
            }
            if ((res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, message.size())) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::", __func__, ": curl_easy_setopt CURLOPT_POSTFIELDSIZE:", curl_easy_strerror(res)));
            }
            if ((res = curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, message.data())) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::", __func__, ": curl_easy_setopt CURLOPT_COPYPOSTFIELDS:", curl_easy_strerror(res)));
            }

            switch(getRequestType())
            {
                case Get:       res = CURLE_OK; /* The default is GET. So do nothing.*/         break;
                case Head:      res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");    break;
                case Put:       res = curl_easy_setopt(curl, CURLOPT_PUT, 1);                   break;
                case Post:      res = curl_easy_setopt(curl, CURLOPT_POST, 1);                  break;
                case Delete:    res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");  break;
                default:
                    throw std::domain_error(buildErrorMessage("CurlConnector::", __func__, ": invalid method: ", static_cast<int>(getRequestType())));
            }
            if (res != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::", __func__, ": curl_easy_setopt CURL_METHOD:", curl_easy_strerror(res)));
            }
            if ((res = curl_easy_perform(curl)) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::", __func__, ": curl_easy_perform:", curl_easy_strerror(res)));
            }
        }
};


    }
}

int main(int argc, char* argv[])
{
    namespace Sock = ThorsAnvil::Socket;
    if (argc != 3)
    {
        std::cerr << "Usage: client <host> <Message>\n";
        std::exit(1);
    }

    Sock::CurlGlobal    curlInit;
    Sock::CurlConnector connect(argv[1], 8080);

    connect.sendMessage("/message", argv[2]);
}

