#include <Rcpp.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <string>
#include <netdb.h>
#include <unistd.h>

using namespace Rcpp;

namespace {
    const int buf_size = 4096;
    const int max_events = 10;
    struct epoll_event events[max_events];
}

// [[Rcpp::export]]
IntegerVector epollImpl() {
    int efd = epoll_create(1);
    if (efd < 0) {
        Rcpp::stop("Network error");
    }
    return IntegerVector::create(efd);
}

//' @export
// [[Rcpp::export]]
void close_socket(IntegerVector fileno) {
    int fd = fileno[0];
    close(fd);
}

// [[Rcpp::export]]
IntegerVector socket_create(CharacterVector addr, CharacterVector port) {
    std::string p(port[0]);
    std::string address(addr[0]);

    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(address.c_str(), p.c_str(), &hints, &servinfo) != 0) {
        Rcpp::stop("Network error");
    }

    int sock;
    if ((sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0)
    {
        Rcpp::stop("Network error");
    }

    if(connect(sock, servinfo->ai_addr, servinfo->ai_addrlen) != 0){
        Rcpp::stop("Network error");
    }

    freeaddrinfo(servinfo);
    return IntegerVector::create(sock);
}

//' @export
// [[Rcpp::export]]
CharacterVector readLine(IntegerVector fileno) {
    std::string result;
    int fd = fileno[0];
    char buf[buf_size];
    while(true) {
        ssize_t read = recv(fd, &buf, buf_size - 1, 0);
        buf[read] = '\0';
        if (read == 0) {
            Rcpp::stop("Network error");
        }
        result += std::string(buf);
        if (result[result.length() - 1] == '\n') {
            return CharacterVector::create(result);
        }
    }
}

//' @export
// [[Rcpp::export]]
void writeLine(IntegerVector fileno, CharacterVector str) {
    int fd = fileno[0];
    std::string msg(str[0]);
    msg += "\n";
    const char* buf = msg.c_str();
    size_t len = msg.length();
    while(true) {
        ssize_t sent = send(fd, buf, len, 0);
        if (sent == 0) {
            Rcpp::stop("Network error");
        }
        len -= sent;
        buf += sent;
        if (len == 0) {
            return;
        }
    }
}

// [[Rcpp::export]]
IntegerVector epoll_wait(IntegerVector epollfd) {
    int efd = epollfd[0];
    int ndfs = epoll_wait(efd, events, max_events, 0);
    if (ndfs < 0) {
        Rcpp::stop("Network error");
    }
    IntegerVector result(ndfs);
    for (int i = 0; i < ndfs; i++) {
        result[i] = events[i].data.fd;
    }
    return result;
}

// [[Rcpp::export]]
void subscribe(IntegerVector epollfd, IntegerVector fileno, LogicalVector unsubscribe){
    int fd = fileno[0];
    int efd = epollfd[0];
    bool unsub = unsubscribe[0];
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    uint32_t op = unsub ? EPOLL_CTL_DEL : EPOLL_CTL_ADD;
    if (epoll_ctl(efd, op, fd, &ev) == -1) {
        Rcpp::stop("Network error");
    }
}
