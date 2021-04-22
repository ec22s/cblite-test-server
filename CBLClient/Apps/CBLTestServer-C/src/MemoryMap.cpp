#include "MemoryMap.h"
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <sstream>

#ifdef _MSC_VER
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h> // Needed for its define for the next header
#include <iphlpapi.h>
#pragma comment( lib, "Iphlpapi.lib" )
#pragma comment( lib, "Ws2_32.lib" )
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#endif

using namespace std;
using namespace nlohmann;

struct MemoryMapEntry {
    void* item;
    memory_map::cleanup_func cleanup;
};

static int64_t NextID = 0;
static string LocalIPAddr;
static unordered_map<string, MemoryMapEntry> Map;

static mutex Mutex;

#define LOCK() unique_lock<mutex> l(Mutex)

#ifdef _MSC_VER
static void get_ip_addr() {
    ULONG bufferSize = 15;
    PIP_ADAPTER_ADDRESSES addrs = nullptr ,current = nullptr;
    DWORD result;
    int attempts = 3;
    do {
        free(addrs);
        addrs = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(bufferSize));
        result = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_GATEWAYS,
            nullptr, addrs, &bufferSize);
    } while(result == ERROR_BUFFER_OVERFLOW && attempts--);

    if(result != S_OK) {
        throw domain_error("Unable to retrieve IP address");
    }

    current = addrs;
    while(current) {
        if(current->OperStatus == IfOperStatusUp && current->FirstGatewayAddress) {
            char address[255];
            DWORD size = 255;
            WSAAddressToStringA(current[0].FirstUnicastAddress->Address.lpSockaddr, current[0].FirstUnicastAddress->Address.iSockaddrLength, 
                nullptr, address, &size);
            LocalIPAddr = address;
            break;
        }

        current = current->Next;
    }

    free(addrs);
}
#else
static void get_ip_addr() {
    ifaddrs* ifaddr, *ifa;
    char host[NI_MAXHOST];
    if(getifaddrs(&ifaddr) != 0) {
        throw domain_error("Unable to get network interfaces");
    }

    for(ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        if(!(ifa->ifa_flags & IFF_UP) || (ifa->ifa_flags & IFF_LOOPBACK)) {
            continue;
        }

        if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST,
            nullptr, 0, NI_NUMERICHOST) != 0) {
            throw domain_error("Unable to get inteface info");
        }

        LocalIPAddr = host;
        break;
    }

    freeifaddrs(ifaddr);
}
#endif

namespace memory_map {
    void init() {
        if(LocalIPAddr.empty()) {
            get_ip_addr();
        }
    }

    void clear() {
        LOCK();
        NextID = 0;
        for(const auto& entry : Map) {
            entry.second.cleanup(entry.second.item);
        }

        Map.clear();
    }

    void* get(const string &id) {
        LOCK();
        const auto val = Map.find(id);
        if(val == Map.end()) {
            const string s = "Can't find object with id " + id;
            throw out_of_range(s);
        }

        return val->second.item;
    }

    string store(void* item, cleanup_func cleanup) {
        LOCK();
        stringstream ss;
        ss << "@" << NextID++ << "_" << LocalIPAddr << "_C";
        auto id = ss.str();
        Map[id] = { item, cleanup };
        return id;
    }

    string store(const void* item, cleanup_func cleanup) {
        return store(const_cast<void *>(item), cleanup);
    }

    void release(const string& id) {
        LOCK();
        const auto val = Map.find(id);
        if(val != Map.end()) {
            if(val->second.cleanup) {
                val->second.cleanup(val->second.item);
            }

            Map.erase(id);
        }
    }
}