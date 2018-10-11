#' @useDynLib epollPromise
#' @importFrom Rcpp sourceCpp
NULL

#' @export
epoll <- function(delay = 0.1) {
  obj <- list()
  obj$fd <- epollImpl()
  obj$env <- new.env()
  obj$env$subscriptions <- c()
  structure(obj, class = "epoll")
  f <- function() {
    socks <- epoll_wait(obj$fd)
    for (s in socks) {
      subscribe(obj$fd, s, unsubscribe = TRUE)
      fd <- as.character(s)
      fs <- obj$env$subscriptions[[fd]]
      obj$env$subscriptions[[fd]] <- NULL
      line <- readLine(s)
      later::later(function(){
        fs$resolve(line)
      })
    }
    later::later(f, delay)
  }
  later::later(f, delay)
  obj
}

readLinePromise <- function(e, s) {
  f <- function(resolve, reject) {
    subscribe(e$fd, s, FALSE)
    e$env$subscriptions[[as.character(s)]] <- list(resolve = resolve,
                                                   reject = reject)
  }
  promises::promise(f)
}
