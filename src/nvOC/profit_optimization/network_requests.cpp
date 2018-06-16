#include "network_requests.h"

#include <sstream>
#include <iostream>
#include <thread>
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
        curl_easy_setopt(curl, CURLOPT_URL, request_url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        std::string response_string;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw network_error("CURL perform failed: " +
                                std::string(curl_easy_strerror(res)) + " (" + request_url + ")");
        }

        char *url;
        long response_code;
        double elapsed;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
        std::cout << "Network request URL : " << url << std::endl;
        std::cout << "Network request elapsed time: " << elapsed << std::endl;

        curl_easy_cleanup(curl);

        if (response_code != 200)
            throw network_error("CURL invalid response code: " +
                                std::to_string(response_code) + " (" + url + ")");
        return response_string;
    }

    static std::string get_nanopool_url(currency_type type) {
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


    static double __get_approximated_earnings_per_hour_nanopool(currency_type type, double hashrate_hs) {
        double hashrate_arg = (type == currency_type::ETH) ? hashrate_hs / 1e6 : hashrate_hs;
        const std::string &json_response = curl_https_get(
                get_nanopool_url(type) + "/approximated_earnings/" + std::to_string(hashrate_arg));
        //
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        std::string status = root.get<std::string>("status", "false");
        if (status != "true")
            throw network_error("Nanopool REST API error: " + root.get<std::string>("data"));
        return root.get<double>("data.hour.euros");
    }


    static std::map<std::string, double>
    __get_avg_hashrate_per_worker_nanopool(currency_type ct, const std::string &wallet_address, double period_hours) {
        const std::string &json_response = curl_https_get(
                get_nanopool_url(ct) + "/avghashrateworkers/" + wallet_address + "/" +
                std::to_string(period_hours));
        //
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        std::string status = root.get<std::string>("status", "false");
        if (status != "true")
            throw network_error("Nanopool REST API error: " + root.get<std::string>("data"));
        std::map<std::string, double> res;
        for (const boost::property_tree::ptree::value_type &array_elem : root.get_child("data")) {
            const boost::property_tree::ptree &subtree = array_elem.second;
            double worker_hashrate_mhs = subtree.get<double>("hashrate");
            res.emplace(subtree.get<std::string>("worker"),
                        ((ct == currency_type::ETH) ? worker_hashrate_mhs * 1e6 : worker_hashrate_mhs));
        }
        return res;
    };


    static double __get_current_stock_price_nanopool(currency_type ct) {
        const std::string &json_response = curl_https_get(
                get_nanopool_url(ct) + "/prices");
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        std::string status = root.get<std::string>("status", "false");
        if (status != "true")
            throw network_error("Nanopool REST API error: " + root.get<std::string>("data"));
        return root.get<double>("data.price_eur");
    }


    static double __get_energy_cost_stromdao(int plz) {
        const std::string &json_response = curl_https_get(
                "https://stromdao.de/crm/service/gsi/?plz=" + std::to_string(plz));
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        return root.get<double>("tarif.centPerKWh") / 100.0;
    }


    template<typename R, typename ...Args>
    static R safe_network_proxycall(int trials, int trial_timeout_ms, R (*mf)(Args...), Args &&... args) {
        while (true) {
            trials--;
            try {
                return (*mf)(std::forward<Args>(args)...);
            } catch (const network_error &ex) {
                if (trials <= 0)
                    throw;
            } catch (const boost::property_tree::ptree_error &ex) {
                if (trials <= 0)
                    throw network_error("Boost error while processing json: " + std::string(ex.what()));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(trial_timeout_ms));
        }
    }


    double get_approximated_earnings_per_hour_nanopool(currency_type ct, double hashrate_hs, int trials,
                                                       int trial_timeout_ms) {
        return safe_network_proxycall<double, currency_type, double>(
                trials, trial_timeout_ms, &__get_approximated_earnings_per_hour_nanopool, std::move(ct),
                std::move(hashrate_hs));
    }


    std::map<std::string, double>
    get_avg_hashrate_per_worker_nanopool(currency_type ct, const std::string &wallet_address, double period_hours,
                                         int trials, int trial_timeout_ms) {
        return safe_network_proxycall<std::map<std::string, double>, currency_type, const std::string &, double>(
                trials, trial_timeout_ms, &__get_avg_hashrate_per_worker_nanopool,
                std::move(ct), std::move(wallet_address), std::move(period_hours));
    };


    double get_current_stock_price_nanopool(currency_type ct, int trials, int trial_timeout_ms) {
        return safe_network_proxycall<double, currency_type>(
                trials, trial_timeout_ms, &__get_current_stock_price_nanopool, std::move(ct));
    }


    double get_energy_cost_stromdao(int plz, int trials, int trial_timeout_ms) {
        return safe_network_proxycall<double, int>(
                trials, trial_timeout_ms, &__get_energy_cost_stromdao, std::move(plz));
    }

}

