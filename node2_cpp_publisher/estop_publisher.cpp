#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <thread>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

static void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[Node2] WSAStartup failed\n";
        return 1;
    }
#endif

    const int UDP_PORT = 1111;
    const char* BROADCAST_IP = "255.255.255.255";

#ifdef _WIN32
    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "[Node2] Failed to create socket\n";
        WSACleanup();
        return 1;
    }
#else
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "[Node2] Failed to create socket\n";
        return 1;
    }
#endif

    int enable = 1;
#ifdef _WIN32
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*)&enable, sizeof(enable));
#else
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, BROADCAST_IP, &addr.sin_addr);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> estop_dist(0, 1);
    std::uniform_int_distribution<int> sleep_dist_ms(1500, 4500); // 1.5–4.5 seconds

    std::cout << "[Node2] Publishing 'estop' to 255.255.255.255:1111 (Ctrl+C to stop)\n";

    while (true) {
        int estop = estop_dist(gen);
        std::string msg = "estop," + std::to_string(estop);

#ifdef _WIN32
        int sent = sendto(sockfd, msg.c_str(), (int)msg.size(), 0,
                          (sockaddr*)&addr, sizeof(addr));
#else
        ssize_t sent = sendto(sockfd, msg.c_str(), msg.size(), 0,
                              (sockaddr*)&addr, sizeof(addr));
#endif

        if (sent < 0) std::cerr << "[Node2] sendto failed\n";
        else std::cout << "[Node2] Sent: " << msg << "\n";

        sleep_ms(sleep_dist_ms(gen));
    }

#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif
    return 0;
}
