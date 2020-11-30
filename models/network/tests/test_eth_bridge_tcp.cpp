
#include <iostream>
#include <systemc.h>
#include "tlm.h"
#include "models/network/ethbridge.hpp"
#include "models/network/mock_gmii.hpp"
#include "commons/assertions.hpp"
#include <sys/socket.h>
#include <netinet/in.h> 
#include <unistd.h>
#include <thread>

// For async
#include <future>

using namespace sc_dt;
using namespace std;

struct sockaddr_in clientaddr;
struct sockaddr_in serveraddr; 

#define CLK_PERIOD 1

const EthBridge::Configuration config {
      EthBridge::PROTOCOL_TYPE::TCP,
      EthBridge::BRIDGE_IP_T("127.0.0.1"),
      EthBridge::REMOTE_IP_T("127.0.0.1"),
      EthBridge::BRIDGE_PORT_T(5002),
      EthBridge::REMOTE_PORT_T(5000)
  };


const uint8_t TCP_PKT [] = {0x45, 0x00, 0x00, 0x3c, 0x91, 0xce, 0x40, 0x00, 0x40, 0x06, 0xaa, 0xeb, 0x7f, 
                        0x00, 0x00, 0x01, 0x7f, 0x00, 0x00, 0x01, 0xdf, 0x18, 0x1f, 0x90, 0xaa, 0x45, 
                        0x3f, 0x61, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0xaa, 0xaa, 0xfe, 0x30, 0x00, 
                        0x00, 0x02, 0x04, 0xff, 0xd7, 0x04, 0x02, 0x08, 0x0a, 0xb8, 0xd1, 0x41, 0xcb, 
                        0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x07};

int init_server(){ 

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        SC_REPORT_ERROR("Testbench", "TestBench - Socket creation Failed");
    }
    /* setsockopt: Handy debugging trick that lets 
	 * us rerun the server immediately after we kill it; 
	 * otherwise we have to wait about 20 secs. 
	 * Eliminates "ERROR on binding: Address already in use" error. 
	 */
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		   (const void *)&optval, sizeof(int));

    // Create a UDP Socket 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons(config.remote_port); 
    serveraddr.sin_family = AF_INET;  

    /* 
	 * bind: associate the parent socket with a port 
	 */
	if (bind(sockfd, (struct sockaddr *)&serveraddr,
		 sizeof(serveraddr)) < 0){
        std::string emsg { "TestBench - Binding error - errno: "};
        emsg += std::to_string(errno);
        SC_REPORT_ERROR("Testbench", emsg.c_str());
    }

    if (listen(sockfd, 3) < 0) 
    { 
        std::string emsg { "TestBench - listen error - errno: " + std::to_string(errno)};
        SC_REPORT_ERROR("Testbench", emsg.c_str());
    } 

    int connected_socket;
    if ((connected_socket = accept(server_fd, (struct sockaddr *)&address,  
                       (socklen_t*)&addrlen))<0) 
    { 
        std::string emsg { "TestBench - accept error - errno: " + std::to_string(errno)};
        SC_REPORT_ERROR("Testbench", emsg.c_str());
    } 

    clientaddr.sin_addr.s_addr = inet_addr(config.bridge_ip.c_str()); 
    clientaddr.sin_port = htons(config.bridge_port); 
    clientaddr.sin_family = AF_INET;  

    return connected_socket;
}

void send_hello(int sock){
    const char *hello = "Hello from server";
    int len = send(sock , hello , strlen(hello) , 0);
    if (len < 0){
        SC_REPORT_ERROR("Testbench", "TestBench - send_hello failed");
    } 
    cout << sc_time_stamp() << " - Testbench: Hello message sent"  << endl;
}

bool wait_for_hello(int sock){
   std::cout << sc_time_stamp() << " - Testbench: entering waiting phase of wait_for_hello()" << std::endl;
    char *buffer = (char *) malloc(MAX_BUF_SIZE * sizeof(char));
    int n = read(sock, buffer, MAX_BUF_SIZE, 
            0); //receive message from server 
    if (n < 0){
         SC_REPORT_ERROR("Testbench", "TestBench - wait_for_hello failed");
        }
    
    cout << sc_time_stamp() << " - Testbench: wait_for_hello() completed"  << endl;
    return true;
    }



int sc_main(int argc, char** argv) {

   sc_trace_file *Tf = sc_create_vcd_trace_file("/workdir/trace_ethbridge");
   Tf->set_time_unit(1,SC_PS);
   
   // Let'i"s start first the server
   std::future<int> server_socket = std::async(std::launch::async, init_server);
  
   sc_clock clk("clk", sc_time(1, SC_NS)); 
   sc_signal<bool> reset;
   sc_signal<bool>      rx_new_pkt;
   sc_signal<sc_bv<8>>  rx_pkt;
   sc_signal<bool>       tx_new_pkt;
   sc_signal<sc_bv<8>>   tx_pkt;

  sc_signal<bool>  start_tx;

   EthBridge bridge = EthBridge("ethbridge", config);

   bridge.clk(clk);
   bridge.reset(reset);
   bridge.rx_new_pkt(rx_new_pkt);
   bridge.rx_pkt(rx_pkt);
   bridge.tx_new_pkt(tx_new_pkt);
   bridge.tx_pkt(tx_pkt);
   
   sc_trace(Tf, clk, "clk");  
   sc_trace(Tf, rx_new_pkt, "rx_new_pkt");
   sc_trace(Tf, rx_pkt,     "rx_pkt");
   sc_trace(Tf, tx_new_pkt, "tx_new_pkt");
   sc_trace(Tf, tx_pkt,     "tx_pkt");

   MockGMII gmii = MockGMII("mockgmii", UDP_PKT, sizeof(UDP_PKT) / sizeof(uint8_t));
   gmii.clk(clk);
   gmii.start(start_tx);
   gmii.tx_new_pkt(tx_new_pkt);
   gmii.tx_pkt(tx_pkt);
   sc_trace(Tf, start_tx, "start_tx");

   reset.write(0);
   sc_start(10, SC_NS);

   // retrieve the future value
   int sock = server_socket.get();

   send_hello(sock);
   sc_start(10, SC_NS);

   // We start listening (in a separate thread) and then generate the pkt
   std::future<bool> tx_on_wire = std::async(std::launch::async, wait_for_hello, sock);
  
   start_tx.write(1);
   sc_start(10, SC_NS);
   start_tx.write(0);
   sc_start(100, SC_NS);
  
   bool completed = tx_on_wire.get();
   checkValuesMatch<bool>(completed, true, "tx_completed");

   std::cout << "Closing traces" << std::endl;
   sc_close_vcd_trace_file(Tf);
  
   //sc_close_vcd_trace_file(Tf);
   // Assertion phase. 
   checkValuesMatch<int>(bridge.rx_pkt_cnt, 1, "num_of_rx_packets");
   checkValuesMatch<int>(bridge.tx_pkt_cnt, 1, "num_of_tx_packets");
   return 0;
}
    
