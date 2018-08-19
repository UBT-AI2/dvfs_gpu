#include "network_requests.h"

#include <sstream>
#include <thread>
#include <glog/logging.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <curl/curl.h>
#include "../common_header/exceptions.h"
#include "../common_header/constants.h"

namespace frequency_scaling {

    double currency_stats::calc_approximated_earnings_eur_hour(double user_hashrate_hs) const {
        return (user_hashrate_hs / nethash_) * (1 / block_time_sec_) *
               block_reward_ * stock_price_eur_ * 3600;
    }

    //#######################################################################################################

    static size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string *data) {
        data->append((char *) ptr, size * nmemb);
        return size * nmemb;
    }

    static std::string curl_https_get(const std::string &request_url) {
        CURL *curl = curl_easy_init();
        if (!curl)
            THROW_NETWORK_ERROR("CURL initialization failed");
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
            THROW_NETWORK_ERROR("CURL perform failed: " +
                                std::string(curl_easy_strerror(res)) + " (" + request_url + ")");
        }

        char *url;
        long response_code;
        double elapsed;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
        VLOG(2) << "Network request URL : " << url << std::endl;
        VLOG(2) << "Network request elapsed time: " << elapsed << std::endl;

        curl_easy_cleanup(curl);

        if (response_code != 200)
            THROW_NETWORK_ERROR("CURL invalid response code: " +
                                std::to_string(response_code) + " (" + url + ")");
        return response_string;
    }

    //####################################################################################################

    static double get_current_stock_price_cryptocompare(const currency_type &ct) {
        const std::string &json_response = curl_https_get(
                "https://min-api.cryptocompare.com/data/price?fsym=" +
                ct.cryptocompare_fsym_ + "&tsyms=BTC,USD,EUR");
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        return root.get<double>("EUR");
    }

    static void get_currency_stats_whattomine(const currency_type &ct, currency_stats &cs) {
        const std::string &json_response = curl_https_get(
                "https://whattomine.com/coins/" + std::to_string(ct.whattomine_coin_id_) + ".json");
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        cs.block_reward_ = root.get<double>("block_reward");
        cs.block_time_sec_ = root.get<double>("block_time");
        cs.difficulty_ = root.get<double>("difficulty");
        cs.nethash_ = root.get<double>("nethash");
        //cs.nethash_ = cs.difficulty_ / cs.block_time_sec_;
    }

    //#######################################################################################################


    static double __get_approximated_earnings_per_hour(const currency_type &ct, double hashrate_hs) {
        char api_address[BUFFER_SIZE];
        snprintf(api_address, BUFFER_SIZE, ct.pool_approximated_earnings_api_address_.c_str(),
                 std::to_string(hashrate_hs / ct.pool_approximated_earnings_api_unit_factor_hashrate_).c_str());
        const std::string &json_response = curl_https_get(api_address);
        //
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        //api return unit: coins of mined currency
        return root.get<double>(ct.pool_approximated_earnings_json_path_) *
               ct.pool_approximated_earnings_api_unit_factor_period_ * get_current_stock_price_cryptocompare(ct);
    }


    static double __get_avg_worker_hashrate(const currency_type &ct, const std::string &wallet_address,
                                            const std::string &worker_name, int period_ms) {
        char api_address[BUFFER_SIZE];
        snprintf(api_address, BUFFER_SIZE, ct.pool_avg_hashrate_api_address_.c_str(),
                 wallet_address.c_str(), worker_name.c_str(),
                 std::to_string(period_ms / ct.pool_avg_hashrate_api_unit_factor_period_).c_str());
        const std::string &json_response = curl_https_get(api_address);
        //
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        return root.get<double>(ct.pool_avg_hashrate_json_path_) * ct.pool_avg_hashrate_api_unit_factor_hashrate_;
    };

    static double __get_current_worker_hashrate(const currency_type &ct, const std::string &wallet_address,
                                                const std::string &worker_name) {
        char api_address[BUFFER_SIZE];
        snprintf(api_address, BUFFER_SIZE, ct.pool_current_hashrate_api_address_.c_str(),
                 wallet_address.c_str(), worker_name.c_str());
        const std::string &json_response = curl_https_get(api_address);
        //
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        return root.get<double>(ct.pool_current_hashrate_json_path_) *
               ct.pool_current_hashrate_api_unit_factor_hashrate_;
    };

    static currency_stats __get_currency_stats(const currency_type &ct) {
        currency_stats cs;
        cs.stock_price_eur_ = get_current_stock_price_cryptocompare(ct);
        get_currency_stats_whattomine(ct, cs);
        return cs;
    }


    static double __get_energy_cost_stromdao(int plz) {
        const std::string &json_response = curl_https_get(
                "https://stromdao.de/crm/service/gsi/?plz=" + std::to_string(plz));
        std::istringstream is(json_response);
        boost::property_tree::ptree root;
        boost::property_tree::json_parser::read_json(is, root);
        return root.get<double>("tarif.centPerKWh") / 100.0;
    }

    //#######################################################################################################

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
                    THROW_NETWORK_ERROR("Boost error while processing json: " + std::string(ex.what()));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(trial_timeout_ms));
        }
    }


    double get_approximated_earnings_per_hour(const currency_type &ct, double hashrate_hs, int trials,
                                              int trial_timeout_ms) {
        return safe_network_proxycall<double, const currency_type &, double>(
                trials, trial_timeout_ms, &__get_approximated_earnings_per_hour, std::move(ct),
                std::move(hashrate_hs));
    }


    double get_avg_worker_hashrate(const currency_type &ct, const std::string &wallet_address,
                                   const std::string &worker_name, int period_ms, int trials, int trial_timeout_ms) {
        return safe_network_proxycall<double, const currency_type &, const std::string &, const std::string &, int>(
                trials, trial_timeout_ms, &__get_avg_worker_hashrate,
                std::move(ct), std::move(wallet_address), std::move(worker_name), std::move(period_ms));
    };

    double get_current_worker_hashrate(const currency_type &ct, const std::string &wallet_address,
                                       const std::string &worker_name, int trials, int trial_timeout_ms) {
        return safe_network_proxycall<double, const currency_type &, const std::string &, const std::string &>(
                trials, trial_timeout_ms, &__get_current_worker_hashrate,
                std::move(ct), std::move(wallet_address), std::move(worker_name));
    };

    currency_stats get_currency_stats(const currency_type &ct, int trials, int trial_timeout_ms) {
        return safe_network_proxycall<currency_stats, const currency_type &>(
                trials, trial_timeout_ms, &__get_currency_stats, std::move(ct));
    }


    double get_energy_cost_stromdao(int plz, int trials, int trial_timeout_ms) {
        return safe_network_proxycall<double, int>(
                trials, trial_timeout_ms, &__get_energy_cost_stromdao, std::move(plz));
    }

}

