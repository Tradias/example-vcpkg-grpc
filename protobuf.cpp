#include "helloworld.pb.h"

int main() {
  helloworld::HelloRequest request;
  helloworld::HelloReply response;
  response.set_message("Hello " + request.name());
}