#include "profit_calculation.h"
#include "network_requests.h"

namespace frequency_scaling {

    currency_info::currency_info(currency_type type, double difficulty, double block_reward, double stock_price)
            : type_(type), difficulty_(difficulty), block_reward_(block_reward), stock_price_usd_(stock_price) {}


    energy_hash_info::energy_hash_info(currency_type type_, double hashrate_hs_, double energy_consumption_js_) : type_(
            type_), hashrate_hs_(hashrate_hs_), energy_consumption_js_(energy_consumption_js_) {}


    profit_calculator::profit_calculator(const std::map<currency_type, currency_info> &currency_info,
                                         const std::map<currency_type, energy_hash_info> &energy_hash_info,
                                         double power_cost_kwh,
                                         double pool_fee_percentage) : currency_info_(currency_info),
                                                                       energy_hash_info_(energy_hash_info),
                                                                       power_cost_kwh_(power_cost_kwh),
                                                                       pool_fee_percentage_(pool_fee_percentage) {}


    std::pair<currency_type, double>
    profit_calculator::calc_best_currency() const {
        return std::pair<currency_type, double>(currency_type::ZEC, 0.03);
    }


    std::map<currency_type, currency_info> get_currency_infos() {
        std::map<currency_type, currency_info> currency_infos;
        currency_infos.emplace(currency_type::ZEC,
                               currency_info(currency_type::ZEC, 8310910.23802672, 10.00288114,
                                             get_current_price_eur(currency_type::ZEC)));
        currency_infos.emplace(currency_type::ETH,
                               currency_info(currency_type::ETH, 3289492680541800, 3.00000000,
                                             get_current_price_eur(currency_type::ETH)));
        currency_infos.emplace(currency_type::XMR,
                               currency_info(currency_type::XMR, 58874879759, 4.86930501,
                                             get_current_price_eur(currency_type::XMR)));
        return currency_infos;
    }

}
