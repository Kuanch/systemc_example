#include <systemc>
#include <scml2.h>
#include <tlm>
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

#include <iostream>
#include <cstdlib> // For rand()
#include <ctime>   // For seeding rand()

using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

SC_MODULE(MemoryModule) {
    tlm_utils::simple_target_socket<MemoryModule> target_socket;  // Target socket
    scml2::memory<unsigned char> memory;                         // SCML memory (1 KB)

    // Variables for bandwidth calculation
    sc_time total_simulation_time;
    uint64_t total_bytes_transferred;
    const unsigned int memory_size;        // Memory size (bytes)
    const unsigned int max_burst_length;   // Maximum burst length (bytes)
    const sc_time max_allowed_latency;     // Timeout threshold

    SC_CTOR(MemoryModule)
        : target_socket("target_socket"),
          memory("memory", 1024 * 1024), // Set memory to 1MB
          total_bytes_transferred(0),
          memory_size(1024 * 16),     // Initialize memory size
          max_burst_length(16),         // Initialize max burst length
          max_allowed_latency(sc_time(100, SC_NS)) { // Initialize timeout latency
        // Register callback for b_transport
        target_socket.register_b_transport(this, &MemoryModule::b_transport);

        // Configure memory latencies (optional)
        memory.set_default_read_latency(sc_time(10, SC_NS));
        memory.set_default_write_latency(sc_time(20, SC_NS));
    }

    // Callback for b_transport
    void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
        tlm::tlm_command cmd = trans.get_command();
        uint64_t addr = trans.get_address();
        unsigned char* data_ptr = trans.get_data_ptr();
        unsigned int length = trans.get_data_length();

        // Address range check
        if (addr + length > memory.get_size()) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        // Apply memory operation and latency
        if (cmd == tlm::TLM_WRITE_COMMAND) {
            for (unsigned int i = 0; i < length; ++i) {
                memory.write(addr + i, data_ptr, 1);
            }
            total_bytes_transferred += length;
            delay += memory.get_default_write_latency(); // Add write latency
        } else if (cmd == tlm::TLM_READ_COMMAND) {
            for (unsigned int i = 0; i < length; ++i) {
                memory.read(addr + i, data_ptr, 1);
            }
            total_bytes_transferred += length;
            delay += memory.get_default_read_latency(); // Add read latency
        } else {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }

        // Check if the latency exceeds the maximum allowed threshold
        if (delay > max_allowed_latency) {
            std::cerr << "Transaction timeout: addr=" << addr << ", delay=" << delay << std::endl;
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }

        // Successful transaction
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    // Method to print bandwidth utilization at the end of simulation
    void end_of_simulation() override {
        double total_simulation_time = sc_time_stamp().to_seconds();
        double bandwidth_mb = total_bytes_transferred / total_simulation_time / 1024 / 1024;
        std::cout << "Total MB transferred: " << total_bytes_transferred << std::endl;
        std::cout << "Total simulation time: " << sc_time_stamp() << std::endl;
        std::cout << "Bandwidth Utilization: " << bandwidth_mb << " MB/second" << std::endl;
    }
};

SC_MODULE(TrafficGenerator) {
    tlm_utils::simple_initiator_socket<TrafficGenerator> initiator_socket;
    sc_in<bool> clock; // Clock input

    const sc_time cycle_period = sc_time(10, SC_NS); // Clock period
    const unsigned int memory_size = 1024;       // Memory size (bytes)
    const unsigned int max_burst_length = 16;   // Maximum burst length (bytes)

    SC_CTOR(TrafficGenerator) {
        SC_THREAD(generate_traffic); // Thread to generate traffic
        srand(static_cast<unsigned>(time(nullptr))); // Seed random number generator
        sensitive << clock.pos();
    }

    // Constructor with parameter to set max cycles
    TrafficGenerator(sc_module_name name, unsigned int cycles)
        : sc_module(name) {
        SC_THREAD(generate_traffic);
        srand(static_cast<unsigned>(time(nullptr)));
    }

    void generate_traffic() {
        while (true) {
            // Wait for clock edge
            wait();
            // Randomly decide between read and write
            bool is_write = rand() % 2;

            // Random address and data
            uint64_t addr = rand() % (memory_size - max_burst_length); // Random address
            unsigned int length = (rand() % max_burst_length) + 1; // Random burst length (1 to 16 bytes)
            unsigned char data[max_burst_length]; // Buffer for read/write data

            tlm::tlm_generic_payload trans;
            sc_time delay = sc_time(rand() % 10, SC_NS); // Random delay between transactions
            delay += cycle_period; // Add clock cycle delay

            if (is_write) {
                // Generate random write data
                for (unsigned int i = 0; i < length; ++i) {
                    data[i] = static_cast<unsigned char>(rand() % 256);
                }

                trans.set_command(tlm::TLM_WRITE_COMMAND);
                trans.set_address(addr);
                trans.set_data_ptr(data);
                trans.set_data_length(length);
            } else {
                trans.set_command(tlm::TLM_READ_COMMAND);
                trans.set_address(addr);
                trans.set_data_ptr(data);
                trans.set_data_length(length);
            }

            // Perform the transaction
            initiator_socket->b_transport(trans, delay);

            // Check the response status
            switch (trans.get_response_status()) {
            case tlm::TLM_OK_RESPONSE:
                break;
            case tlm::TLM_INCOMPLETE_RESPONSE:
                std::cerr << "TrafficGenerator:" << trans.get_command() << "incomplete at address 0x"
                          << std::hex << addr << std::endl;
                break;
            case tlm::TLM_ADDRESS_ERROR_RESPONSE:
                std::cerr << "TrafficGenerator: Address out of range at address 0x"
                          << std::hex << addr << std::endl;
                break;
            case tlm::TLM_COMMAND_ERROR_RESPONSE:
                std::cerr << "TrafficGenerator: Invalid command at address 0x"
                          << std::hex << addr << std::endl;
                break;
            case tlm::TLM_GENERIC_ERROR_RESPONSE:
                std::cerr << "TrafficGenerator: Generic error at address 0x"
                          << std::hex << addr << std::endl;
                break;
            case tlm::TLM_BURST_ERROR_RESPONSE:
                std::cerr << "TrafficGenerator: Burst error at address 0x"
                          << std::hex << addr << std::endl;
                break;
            default:
                break;
            }

        }
    }
};


SC_MODULE(TopModule) {
    sc_clock clk;
    TrafficGenerator traffic_gen;  // Traffic Generator
    MemoryModule memory;           // Memory module

    SC_CTOR(TopModule)
        : clk("Clock", sc_time(10, SC_NS)), // 10 ns clock period
          traffic_gen("TrafficGenerator"),
          memory("Memory") {
        traffic_gen.initiator_socket.bind(memory.target_socket);
        traffic_gen.clock(clk); // Connect clock to the traffic generator
    }
};

int sc_main(int argc, char* argv[]) {
    TopModule top("TopModule");
    const sc_time sim_period = sc_time(1, SC_MS); // Clock period
    sc_start(sim_period); // Run simulation for the required number of cycles

    sc_stop();   // Stop simulation
    return 0;
}