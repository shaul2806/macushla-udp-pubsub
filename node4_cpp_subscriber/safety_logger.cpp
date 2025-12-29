#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cctype>

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

static long long ms_since_boot(const std::chrono::steady_clock::time_point& boot) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - boot
    ).count();
}

static void trim(std::string& s) {
    while (!s.empty() && (s.back()=='\n' || s.back()=='\r' || s.back()==' ' || s.back()=='\t')) s.pop_back();
    size_t i = 0;
    while (i < s.size() && (s[i]==' ' || s[i]=='\t')) i++;
    if (i) s.erase(0, i);
}

static bool parse_estop(const std::string& msg, bool& estop_out) {
    auto comma = msg.find(',');
    if (comma == std::string::npos) return false;

    std::string topic = msg.substr(0, comma);
    std::string data  = msg.substr(comma + 1);
    trim(topic); trim(data);

    for (char& c : topic) c = (char)std::tolower((unsigned char)c);

    if (topic != "estop") return false;
    if (data != "0" && data != "1") return false;

    estop_out = (data == "1");
    return true;
}

int main() {
    auto boot = std::chrono::steady_clock::now();

    std::ofstream log("safety_log.txt", std::ios::out | std::ios::app);
    if (!log) {
        std::cerr << "[Node4] Cannot open safety_log.txt\n";
        return 1;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "[Node4] WSAStartup failed\n";
        return 1;
    }
#endif

#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[Node4] socket() failed\n";
        WSACleanup();
        return 1;
    }
#else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "[Node4] socket() failed\n";
        return 1;
    }
#endif

    // ✅ Allow multiple listeners on the same UDP port (needed because Node3 also binds to 1111)
    int reuse = 1;
#ifdef _WIN32
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) != 0) {
        std::cerr << "[Node4] setsockopt(SO_REUSEADDR) failed\n";
    }
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(1111);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (sockaddr*)&local, sizeof(local)) != 0) {
        std::cerr << "[Node4] bind() failed\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }

    enum class State { UNKNOWN, SAFE, STOP };
    State state = State::UNKNOWN;

    std::cout << "[Node4] Listening for estop on UDP port 1111\n";
    std::cout << "[Node4] Logging transitions to safety_log.txt\n";

    char buf[2048];

    while (true) {
        sockaddr_in from{};
#ifdef _WIN32
        int fromlen = sizeof(from);
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0, (sockaddr*)&from, &fromlen);
        if (n == SOCKET_ERROR) continue;
#else
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(sock, buf, sizeof(buf) - 1, 0, (sockaddr*)&from, &fromlen);
        if (n < 0) continue;
#endif
        buf[n] = '\0';
        std::string msg(buf);

        bool estop = false;
        if (!parse_estop(msg, estop)) continue;

        State new_state = estop ? State::STOP : State::SAFE;
        if (new_state != state) {
            long long t = ms_since_boot(boot);

            auto s2 = [](State s) {
                if (s == State::SAFE) return "SAFE";
                if (s == State::STOP) return "STOP";
                return "UNKNOWN";
            };

            std::string line = "[" + std::to_string(t) + " ms] " +
                               s2(state) + std::string(" -> ") + s2(new_state);

            log << line << "\n";
            log.flush();

            std::cout << "[Node4] " << line << "\n";
            state = new_state;
        }
    }

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return 0;
}
