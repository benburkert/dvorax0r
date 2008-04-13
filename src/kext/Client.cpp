extern "C" {
#include <sys/systm.h>
#include <sys/un.h>
  errno_t sock_nointerrupt(socket_t so, int on);
}
#include "Client.hpp"

namespace {
  void
  releaseSocket(socket_t socket)
  {
    sock_shutdown(socket, SHUT_RDWR);
    sock_close(socket);
  }

  bool
  makeSocket(socket_t *socket)
  {
    int error = sock_socket(PF_LOCAL, SOCK_STREAM, 0, NULL, NULL, socket);
    if (error) {
      printf("KeyRemap4MacBook_client sock_socket failed(%d)\n", error);
      return false;
    }

    // ----------------------------------------
    struct timeval tv;
    tv.tv_sec = KeyRemap4MacBook_client::Bridge::BRIDGE_TIMEOUT;
    tv.tv_usec = 0;

    error = sock_setsockopt(*socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
    if (error) {
      releaseSocket(*socket);
      printf("KeyRemap4MacBook_client sock_setsockopt failed(%d)\n", error);
      return false;
    }

    return true;
  }

  bool
  connectSocket(socket_t socket)
  {
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_len = sizeof(addr);
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, KeyRemap4MacBook_server::Bridge::socketPath, sizeof(addr.sun_path) - 1);

    int error = sock_connect(socket, reinterpret_cast<const sockaddr *>(&addr), 0);
    if (error && error != EINPROGRESS) {
      if (error != ENOENT) {
        printf("KeyRemap4MacBook_client sock_connect failed(%d)\n", error);
      }
      return false;
    }
    error = sock_nointerrupt(socket, TRUE);
    if (error) {
      printf("KeyRemap4MacBook_client sock_nointerrupt(%d)\n", error);
      return false;
    }

    return true;
  }
}

int
KeyRemap4MacBook_client::Bridge::sendmsg(KeyRemap4MacBook_server::Bridge::RequestType type, void *request, size_t requestsize, void *reply, size_t replysize)
{
  socket_t socket;
  if (! makeSocket(&socket)) {
    return EIO;
  }

  if (! connectSocket(socket)) {
    releaseSocket(socket);
    return EIO;
  }

  // ----------------------------------------
  struct msghdr msg;
  memset(&msg, 0, sizeof(msg));

  struct iovec aiov[3];
  size_t iolen;
  aiov[0].iov_base = reinterpret_cast<caddr_t>(&type);
  aiov[0].iov_len = sizeof(type);
  if (requestsize <= 0) {
    msg.msg_iovlen = 1;
  } else {
    aiov[1].iov_base = reinterpret_cast<caddr_t>(&requestsize);
    aiov[1].iov_len = sizeof(requestsize);
    aiov[2].iov_base = reinterpret_cast<caddr_t>(request);
    aiov[2].iov_len = requestsize;
    msg.msg_iovlen = 3;
  }
  msg.msg_iov = aiov;

  int error = sock_send(socket, &msg, 0, &iolen);
  if (error) {
    printf("KeyRemap4MacBook_client::sendmsg sock_send failed(%d)\n", error);
    releaseSocket(socket);
    return error;
  }

  // ----------------------------------------
  memset(&msg, 0, sizeof(msg));

  int result;
  aiov[0].iov_base = reinterpret_cast<caddr_t>(&result);
  aiov[0].iov_len = sizeof(result);
  aiov[1].iov_base = reinterpret_cast<caddr_t>(reply);
  aiov[1].iov_len = replysize;
  msg.msg_iov = aiov;
  msg.msg_iovlen = (replysize == 0 ? 1 : 2);

  for (;;) {
    error = sock_receive(socket, &msg, MSG_WAITALL, &iolen);
    if (error == EWOULDBLOCK) continue;

    break;
  }

  releaseSocket(socket);
  return error;
}
