#ifndef GRPCCONNECTOR_H
#define GRPCCONNECTOR_H

#include <QObject>
#include "PprzToolbox.h"
#include "PprzApplication.h"
#include <memory>
#include <grpcpp/grpcpp.h>
#include "pprzgcs.grpc.pb.h"

using grpc::Server;
using grpc::Status;
using grpc::ServerContext;

namespace p = pprzgcs;


class GRPCConnector : public PprzTool, public p::GCSService::Service
{
    Q_OBJECT
public:
    explicit GRPCConnector(PprzApplication* app, PprzToolbox* toolbox);
    static GRPCConnector* get() {
        return pprzApp()->toolbox()->connector();
    }

    virtual void setToolbox(PprzToolbox* toolbox) override;

    Status LoadSRTM([[maybe_unused]] ServerContext* context,
                    const p::SRTMRequest* request, p::SRTMReply* reply) override;
    Status CenterOn(grpc::ServerContext *context,
                    const p::CenterOnRequest *request,p::CenterOnResponse *response) override;
    Status SetWidgetProperty(grpc::ServerContext *context,
                    const p::WidgetProperty *request, ::google::protobuf::Empty *response) override;

signals:
    void dl_srtm();

private:
    void runServer();

    std::unique_ptr<Server> server;

};

#endif // GRPCCONNECTOR_H
