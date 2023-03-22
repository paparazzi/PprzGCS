#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "pprzgcs.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using pprzgcs::GCSService;
using pprzgcs::CenterOnRequest;
using pprzgcs::CenterOnResponse;

class PprzgcsClient {
 public:
  PprzgcsClient(std::shared_ptr<Channel> channel): stub_(GCSService::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  void CenterOn(double lat, double lon) {
    // Data we are sending to the server.
    CenterOnRequest request;
    auto coor = request.mutable_coordinate();
    coor->set_crs("WGS84");
    coor->set_lat(lat);
    coor->set_lon(lon);
    //request.mutable_aircraft()->set_ac_name("JP");

    // Container for the data we expect from the server.
    CenterOnResponse reply;
    
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->CenterOn(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      std::cout << reply.status() << "    " << reply.msg() << std::endl;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()<< std::endl;
    }
  }

 private:
  std::unique_ptr<GCSService::Stub> stub_;
};

int main(int argc, char** argv) {
  std::string target_str = "localhost:50051";

  PprzgcsClient greeter(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  
  greeter.CenterOn(43.5, 1.2);

  return 0;
}

