#include "grpcconnector.h"
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
//#include "pprzgcs.grpc.pb.h"
#include <QDebug>
#include <QThread>
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include "pprzmain.h"

using grpc::Server;
using grpc::ServerBuilder;


void GRPCConnector::runServer() {
  std::string server_address("0.0.0.0:50051");
//  GreeterServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  //builder.RegisterService(&service);
  builder.RegisterService(this);
  // Finally assemble the server.
  server = builder.BuildAndStart();
  qDebug() << "GRPC server listening on " << server_address.c_str();

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}


GRPCConnector::GRPCConnector(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{

}

void GRPCConnector::setToolbox(PprzToolbox* toolbox) {
    PprzTool::setToolbox(toolbox);

    auto th = QThread::create([=] {
        this->runServer();
    });
    th->start();
}


Status GRPCConnector::LoadSRTM([[maybe_unused]] ServerContext* context, [[maybe_unused]] const p::SRTMRequest* request,
                [[maybe_unused]] p::SRTMReply* reply) {
  emit dl_srtm();
  return Status::OK;
}

Status GRPCConnector::CenterOn([[maybe_unused]] ServerContext *context, [[maybe_unused]] const p::CenterOnRequest *request,
                               [[maybe_unused]] p::CenterOnResponse *response) {
    if(request->has_aircraft()) {
        // center on AC
        auto ac_id = QString::number(request->aircraft().ac_id());
        auto ac_name = QString::fromStdString(request->aircraft().ac_name());
        auto ac = AircraftManager::get()->getAircraftByName(ac_name);

        if(AircraftManager::get()->aircraftExists(ac_id)) {
            // by ID
            auto ac = AircraftManager::get()->getAircraft(ac_id);
            emit DispatcherUi::get()->centerMap(ac->getPosition());
        } else if(ac.has_value()) {
            // by name
            emit DispatcherUi::get()->centerMap(ac.value()->getPosition());
        } else {
            response->set_status(p::CenterOnResponse::CENTER_FAILED);
            response->set_msg("AC ID and name unknown");
        }
    } else if (request->has_coordinate()) {
        // center on position
        auto crs = request->coordinate().crs();
        if(crs == "WGS84" || crs == "EPSG:4326") {
            auto pt = Point2DLatLon(request->coordinate().lat(),request->coordinate().lon());
            emit DispatcherUi::get()->centerMap(pt);
        } else {
            //
            response->set_status(p::CenterOnResponse::CENTER_FAILED);
            response->set_msg("CRS not handled");
        }
    }

    return Status::OK;
}


QVariant fromProto(::google::protobuf::Any any) {
    QVariant variant;
    if(any.Is<p::String>()) {
        p::String s;
        any.UnpackTo(&s);
        variant.setValue(QString::fromStdString(s.value()));
    } else if(any.Is<p::Color>()) {
        p::Color c;
        any.UnpackTo(&c);
        auto color = QColor(c.red(), c.green(), c.blue(), c.alpha());
        variant.setValue(color);
    }
    return variant;
}

Status GRPCConnector::SetWidgetProperty([[maybe_unused]] grpc::ServerContext *context,
                                        const pprzgcs::WidgetProperty *request,
                                        [[maybe_unused]] ::google::protobuf::Empty *response) {
    auto widget_name = QString::fromStdString(request->widget_name());
    auto widget = pprzApp()->mainWindow()->findChild<QWidget*>(widget_name);
    if(widget != nullptr) {
        if(request->has_visible()) {
            widget->setVisible(request->visible());
        }
        else if(request->has_property()){
            QVariant value = fromProto(request->property().value());
            if(value.isValid()) {
                widget->setProperty(request->property().property_name().c_str(), value);
            }
        }
    }

    return Status::OK;
}
