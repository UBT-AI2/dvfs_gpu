#include "network_requests.h"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <curl/curl.h>
#include "../exceptions.h"

namespace frequency_scaling {

    static size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string *data) {
        data->append((char *) ptr, size * nmemb);
        return size * nmemb;
    }

    static std::string curl_https_get(const std::string &request_url) {
        CURL *curl = curl_easy_init();
        if (!curl)
            throw network_error("CURL initialization failed");
        curl_easy_setopt(curl, CURLOPT_URL, request_url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        std::string response_string;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        char *url;
        long response_code;
        double elapsed;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (response_code != 200)
            throw network_error("Response code: " + std::to_string(response_code));
        return response_string;
    }

    double get_current_price_eur(currency_type type) {
        std::string json_response;
        switch (type) {
            case currency_type::ZEC:
                json_response = curl_https_get("https://min-api.cryptocompare.com/data/price?fsym=ZEC&tsyms=EUR");
                break;
            case currency_type::ETH:
                json_response = curl_https_get("https://min-api.cryptocompare.com/data/price?fsym=ETH&tsyms=EUR");
                break;
            case currency_type::XMR:
                json_response = curl_https_get("https://min-api.cryptocompare.com/data/price?fsym=XMR&tsyms=EUR");
                break;
        }
        //
        std::istringstream is(json_response);
        boost::property_tree::ptree ptree;
        boost::property_tree::json_parser::read_json(is, ptree);
        return ptree.get<double>("EUR");
    }

}

