//
// Created by alex on 16.05.18.
//

#include "profit_optimization.h"

#include <thread>
#include <iostream>
#include "../script_running/mining.h"
#include "../freq_nelder_mead/freq_nelder_mead.h"
#include "profit_calculation.h"
#include "network_requests.h"

namespace frequency_scaling {

    static void start_mining_best_currency(const profit_calculator &profit_calc, const miner_user_info &user_info) {
        const std::pair<currency_type, double> &best_currency = profit_calc.calc_best_currency();
        const energy_hash_info &ehi = profit_calc.getEnergy_hash_info_().at(best_currency.first);
        miner_script ms = get_miner_for_currency(best_currency.first);
        //
        change_clocks_nvml_nvapi(ehi.dci_, ehi.optimal_configuration_.mem_oc,
                                 ehi.optimal_configuration_.nvml_graph_clock_idx);
        start_mining_script(ms, ehi.dci_, user_info);
    }

    static void
    start_profit_monitoring(profit_calculator &profit_calc, const miner_user_info &user_info, int update_interval_sec) {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(update_interval_sec));
            try {
                const std::pair<currency_type, double> &old_best_currency = profit_calc.calc_best_currency();
                profit_calc.update_currency_info_nanopool();
                const std::pair<currency_type, double> &new_best_currency = profit_calc.calc_best_currency();
                if (old_best_currency.first != new_best_currency.first) {
                    stop_mining_script(get_miner_for_currency(old_best_currency.first));
                    start_mining_best_currency(profit_calc, user_info);
                }
            } catch (const std::exception &ex) {
                std::cout << "Exception: " << ex.what() << std::endl;
            }
        }
    }

    void mine_most_profitable_currency(const miner_user_info &user_info, const device_clock_info &dci,
                                       int max_iterations, int mem_step, int graph_idx_step) {
        //
        //start power monitoring
        start_power_monitoring_script(dci.device_id_nvml);
        const measurement &m_eth = freq_nelder_mead(miner_script::ETHMINER, dci, 1, max_iterations, mem_step,
                                                    graph_idx_step);
        const measurement &m_zec = freq_nelder_mead(miner_script::EXCAVATOR, dci, 1, max_iterations, mem_step,
                                                    graph_idx_step);
        const measurement &m_xmr = freq_nelder_mead(miner_script::XMRSTAK, dci, 1, max_iterations, mem_step,
                                                    graph_idx_step);
        printf("Best energy-hash value ETH: %f\n", m_eth.energy_hash_);
        printf("Best energy-hash value ZEC: %f\n", m_zec.energy_hash_);
        printf("Best energy-hash value XMR: %f\n", m_xmr.energy_hash_);
        stop_power_monitoring_script(dci.device_id_nvml);


        std::map<currency_type, energy_hash_info> energy_hash_infos;
        energy_hash_infos.emplace(currency_type::ZEC,
                                  energy_hash_info(currency_type::ZEC, dci, m_zec));
        energy_hash_infos.emplace(currency_type::ETH,
                                  energy_hash_info(currency_type::ETH, dci, m_eth));
        energy_hash_infos.emplace(currency_type::XMR,
                                  energy_hash_info(currency_type::XMR, dci, m_xmr));
        const std::map<currency_type, currency_info> &currency_infos = get_currency_infos_nanopool(energy_hash_infos);

        profit_calculator pc(currency_infos, energy_hash_infos, get_energy_cost_stromdao(95440));
        start_mining_best_currency(pc, user_info);
        start_profit_monitoring(pc, user_info, 300);
    }

}