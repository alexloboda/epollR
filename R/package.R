#' @useDynLib epollPromise
#' @importFrom Rcpp sourceCpp
NULL

#' @export
socket <- function(hostname, port) {
  structure(socket_create(hostname, port), class = "epoll_socket")
}

#' @export
epoll <- function(delay = 0.1) {
  obj <- list()
  obj$fd <- epollImpl()
  obj$env <- new.env()
  obj$env$subscriptions <- c()
  obj$env$open <- TRUE
  f <- function() {
    if (!obj$env$open) {
      return()
    }
    socks <- epoll_wait(obj$fd)
    for (s in socks) {
      subscribe(obj$fd, s, unsubscribe = TRUE)
      fd <- as.character(s)
      fs <- obj$env$subscriptions[[fd]]
      obj$env$subscriptions[[fd]] <- NULL
      line <- readLine(s)
      fs$resolve(line)
    }
    later::later(f, delay)
  }
  later::later(f, delay)
  structure(obj, class = "epoll")
}

#' @export
close.epoll <- function(obj) {
  obj$env$open <- FALSE
  close_socket(obj$fd)
}

#' @export
close.epoll_socket <- function(s) {
  close_socket(s)
}

#' @export
readLinePromise <- function(e, s) {
  f <- function(resolve, reject) {
    subscribe(e$fd, s, FALSE)
    e$env$subscriptions[[as.character(s)]] <- list(resolve = resolve,
                                                   reject = reject)
  }
  promises::promise(f)
}
