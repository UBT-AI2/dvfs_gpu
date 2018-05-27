//
// Created by alex on 16.05.18.
//

#include "profit_optimization.h"

#include "../script_running/mining.h"
#include "../freq_nelder_mead/freq_nelder_mead.h"
#include "profit_calculation.h"

namespace frequency_scaling {

    void mine_most_profitable_currency(const std::string &user_info, const device_clock_info &dci,
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
                                  energy_hash_info(currency_type::ZEC, m_zec.hashrate_, m_zec.power_));
        energy_hash_infos.emplace(currency_type::ETH,
                                  energy_hash_info(currency_type::ETH, m_eth.hashrate_, m_eth.power_));
        energy_hash_infos.emplace(currency_type::XMR,
                                  energy_hash_info(currency_type::XMR, m_xmr.hashrate_, m_xmr.power_));
        const std::map<currency_type, currency_info> &currency_infos = get_currency_infos_nanopool(energy_hash_infos);

        profit_calculator pc(currency_infos, energy_hash_infos, 0.3);
        const std::pair<currency_type, double>& best_currency = pc.calc_best_currency();

        switch (best_currency.first) {
            case currency_type::ETH:
                change_clocks_nvml_nvapi(dci, m_eth.mem_oc, m_eth.nvml_graph_clock_idx);
                start_mining_script(miner_script::ETHMINER, dci, user_info);
                break;
            case currency_type::ZEC:
                change_clocks_nvml_nvapi(dci, m_zec.mem_oc, m_zec.nvml_graph_clock_idx);
                start_mining_script(miner_script::EXCAVATOR, dci, user_info);
                break;
            case currency_type::XMR:
                change_clocks_nvml_nvapi(dci, m_xmr.mem_oc, m_xmr.nvml_graph_clock_idx);
                start_mining_script(miner_script::XMRSTAK, dci, user_info);
                break;
            default:
                throw std::runtime_error("Invalid enum value");
        }
    }

}