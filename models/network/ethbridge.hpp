/**
 * @file ethbridge.hpp
 * @author Riverlane, 2020
 *
 */
#ifndef __ETHBRIDGE_H__
#define __ETHBRIDGE_H__

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <systemc>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <fcntl.h> /* Added for the nonblocking socket */
#include "models/network/network_helpers.hpp"

using namespace std;
using namespace sc_dt;

#define MAX_BUF_SIZE 1024

/**
 * Implementation of an ethernet to socket component
 *
 * This blocks represents a simple memory exposed to a single `DWIDTH`-wide wishbone bus
 *  SIZE defines the size of the Memory.
 */
struct EthBridge: sc_module
{
    sc_in_clk         clk;
    sc_in<bool>       reset;
    sc_out<sc_bv<1>>  rx_new_pkt;
    sc_out<sc_bv<8>>  rx_pkt;
    sc_in<sc_bv<1>>   tx_new_pkt;
    sc_in<sc_bv<8>>   tx_pkt;

    // Supported Protocols
    enum PROTOCOL_TYPE {UDP, TCP};
    typedef std::string BRIDGE_IP_T;
    typedef std::string REMOTE_IP_T;
    typedef uint16_t BRIDGE_PORT_T;
    typedef uint16_t REMOTE_PORT_T;

    struct Configuration {
        PROTOCOL_TYPE protocol;
        BRIDGE_IP_T bridge_ip = "127.0.0.1";
        REMOTE_IP_T remote_ip = "127.0.0.1";
        BRIDGE_PORT_T bridge_port = 5002;
        REMOTE_PORT_T remote_port = 5000;
        bool trace_on = false;
    };

    // Configuration of ports, IPs etc
    Configuration config;

    // Sockets and addresse
    int sockfd {0};
    struct sockaddr_in sockstr;
    socklen_t add_size;

    // Tx
    char tx_buffer [MAX_BUF_SIZE] {0};
    int tx_pkt_cnt {0};
    enum state_t {IDLE, PROCESSING, FINALIZING};

    // RX
    char rx_buffer [MAX_BUF_SIZE] {0};
    int rx_pkt_cnt {0};

    // TX components
    state_t tx_state {IDLE};
    int tx_idx {0};

    // RX components
    int rec_len {0};
    std::vector<char *> rx_pkts;


    void linkup() {
        // create raw socket
        if (config.protocol == PROTOCOL_TYPE::UDP) {
            sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
        } else if (config.protocol == PROTOCOL_TYPE::TCP) {
            sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
        }

        if(sockfd<0)
        {
            SC_REPORT_ERROR("ETHBRIDGE", "Error in socket creation");
        }
        // This is required to keep the enable both read and write to the socket
        int on = 1;
        setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

        // Setting up other side address
        sockstr.sin_addr.s_addr = inet_addr(config.remote_ip.c_str());
        sockstr.sin_family = AF_INET;
        sockstr.sin_port = htons(config.remote_port);
        add_size = (socklen_t)sizeof(sockstr);

        std::cout << sc_time_stamp() << " - EthBridge: Socket created" << std::endl;
    }

    void transmit() {
        std::cout << sc_time_stamp() << " - EthBridge: transmit() - about to send packet of size : " << tx_idx << std::endl;
        if (config.trace_on)
            printIPPacket(tx_buffer, tx_idx);

        int send_len = sendto(sockfd, tx_buffer, tx_idx, 0, (const struct sockaddr*)&sockstr, add_size);
        if(send_len < 0)
        {
            std::string err {"Error in tx transmit - errno: "+std::to_string(errno)};
            SC_REPORT_ERROR("ETHBRIDGE", err.c_str());
        } else {
            std::cout << sc_time_stamp() << " - EthBridge: Packet sent! Total size was " << tx_idx << std::endl;
            tx_pkt_cnt++;
        }
    }


    void pack() {
        switch(tx_state) {
        case IDLE:
            if (tx_new_pkt.read() == 1) {
                tx_idx = 0;
                tx_buffer[tx_idx++] = (char) tx_pkt.read().to_int();
                tx_state = PROCESSING;
            }
            break;
        case PROCESSING:
            if (tx_new_pkt.read() == 1) {
                tx_buffer[tx_idx++] = (char) tx_pkt.read().to_int();
            } else {
                tx_state = FINALIZING;
            }
            break;
        case FINALIZING:
            sc_spawn( sc_bind(&EthBridge::transmit, this) );
            tx_state = IDLE;
            break;
        }
    }



    void receive() {
        sc_bv<8> _bus;
        while (true) {
            // Wait next clock cycle
            rec_len = recvfrom(sockfd, rx_buffer, MAX_BUF_SIZE,
                               MSG_DONTWAIT, (struct sockaddr*)&sockstr, &add_size);
            if(rec_len > 0) {
                if (!isEcho(rx_buffer, rec_len, config.bridge_port)) {
                    std::cout << sc_time_stamp() << " - EthBridge: Packet received! " << std::endl;
                    if (config.trace_on)
                        printIPPacket(rx_buffer, rec_len);
                    int rem = rec_len;
                    // Clearing new pkt flag
                    rx_new_pkt.write(0);
                    while (rem > 0) {
                        // Aligning to clk pos edges
                        wait();
                        rx_new_pkt.write(1);
                        // which way do they go in? LSB vs MSB?
                        _bus = rx_buffer[rem];
                        rx_pkt.write(_bus);
                        // one byte at the time!
                        rem--;
                    }
                    rx_pkt_cnt++;
                    rx_new_pkt.write(0);
                } else {
                    std::cout << sc_time_stamp() << " - EthBridge: Ignoring echo" << std::endl;
                }
            }
            wait();
        }
    }


    explicit EthBridge(sc_module_name name, const Configuration& config)
        : sc_module(name), config(config)
    {
        SC_THREAD(linkup);
        sensitive << reset.neg();

        SC_METHOD(pack);
        sensitive << clk.pos();
        dont_initialize();

        SC_THREAD(receive);
        sensitive << clk.pos();
    }

    SC_HAS_PROCESS(EthBridge);
};

#endif //__ETHBRIDGE_H__


