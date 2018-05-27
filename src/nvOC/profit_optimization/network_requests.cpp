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

    static std::string get_nanopool_url(currency_type type){
        switch (type) {
            case currency_type::ZEC:
                return "https://api.nanopool.org/v1/zec";
            case currency_type::ETH:
                return "https://api.nanopool.org/v1/eth";
            case currency_type::XMR:
                return "https://api.nanopool.org/v1/xmr";
            default:
                return "";
        }
    }

    double get_approximated_earnings_per_hour_nanopool(currency_type type, double hashrate_hs) {
        double hashrate_arg = (type == currency_type::ETH)?hashrate_hs/1e6:hashrate_hs;
        const std::string& json_response = curl_https_get(
                get_nanopool_url(type)+ "/approximated_earnings/" + std::to_string(hashrate_arg));
        //
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        std::string status = root.get<std::string>("status", "false");
        if (status != "true")
            throw network_error("API error: " + root.get<std::string>("data"));
        return root.get<double>("data.hour.euros");
    }

    std::map<std::string, double>
    get_avg_hashrate_per_worker_nanopool(currency_type type, const std::string &wallet_address, double period_hours) {
        const std::string& json_response = curl_https_get(
                get_nanopool_url(type) + "/avghashrateworkers/" + wallet_address + "/" +
                std::to_string(period_hours));
        //
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        std::string status = root.get<std::string>("status", "false");
        if (status != "true")
            throw network_error("API error: " + root.get<std::string>("data"));
        std::map<std::string, double> res;
        for (const boost::property_tree::ptree::value_type &array_elem : root.get_child("data")) {
            const boost::property_tree::ptree &subtree = array_elem.second;
            res.emplace(subtree.get<std::string>("worker"), subtree.get<double>("hashrate"));
        }
        return res;
    };


    double get_current_stock_price_nanopool(currency_type type) {
        const std::string& json_response = curl_https_get(
                get_nanopool_url(type) + "/prices");
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        std::string status = root.get<std::string>("status", "false");
        if (status != "true")
            throw network_error("API error: " + root.get<std::string>("data"));
        return root.get<double>("data.price_eur");
    }


    double get_energy_cost_stromdao(int plz) {
        const std::string &json_response = curl_https_get(
                "https://stromdao.de/crm/service/gsi/?plz=" + std::to_string(plz));
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        return root.get<double>("tarif.centPerKWh") / 100.0;
    }


}

